/*
Copyright (c) 2013 Benedikt Bitterli

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "VoxelData.hpp"
#include "PlyLoader.hpp"
#include "Debug.hpp"

#include "thread/ThreadUtils.hpp"
#include "thread/ThreadPool.hpp"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <limits>

VoxelData::VoxelData(const char *path, size_t mem) : _loader(0) {
    _dataStream = fopen(path, "rb");

    if (_dataStream != NULL) {
        fread(&_dataW, 4, 1, _dataStream);
        fread(&_dataH, 4, 1, _dataStream);
        fread(&_dataD, 4, 1, _dataStream);

        init(mem);
        buildTopLut();
    }
}

VoxelData::VoxelData(PlyLoader *loader, int sideLength, size_t mem) : _dataStream(0), _loader(loader) {
    loader->suggestedDimensions(sideLength, _dataW, _dataH, _dataD);
    init(mem);
    int size = int(_maxCacheableSize);
    loader->setupBlockProcessing(sideLength, size, size, size, _dataW, _dataH, _dataD);
    buildTopLut();
}

VoxelData::~VoxelData() {
    if (_dataStream)
        fclose(_dataStream);
    else if (_loader)
        _loader->teardownBlockProcessing();
}

static uint64 countCellsInHiarchicalGrid(int numLevels)
{
    uint64 result = 0;
    uint64 size = 1;
    for (int i = 0; i < numLevels; ++i) {
        result += size;
        size *= 8;
    }
    return result;
}

static void buildHierarchicalGridLinks(uint8 *base, int numLevels, std::vector<uint8 *> &dst)
{
    size_t offset = 0;
    size_t size = 1;
    for (int i = 0; i < numLevels; ++i) {
        dst.push_back(base + offset);
        offset += size;
        size *= 8;
    }
}

template<bool isTop>
void VoxelData::upsampleLutLevel(int l) {
    int size = 2 << l;

    auto innerBody = [&](int z) {
        for (int y = 0; y < size; y += 2) {
            for (int x = 0; x < size; x += 2) {
                int value =
                    getLut<isTop>(l + 1, x, y + 0, z + 0) | getLut<isTop>(l + 1, x + 1, y + 0, z + 0) |
                    getLut<isTop>(l + 1, x, y + 1, z + 0) | getLut<isTop>(l + 1, x + 1, y + 1, z + 0) |
                    getLut<isTop>(l + 1, x, y + 0, z + 1) | getLut<isTop>(l + 1, x + 1, y + 0, z + 1) |
                    getLut<isTop>(l + 1, x, y + 1, z + 1) | getLut<isTop>(l + 1, x + 1, y + 1, z + 1);

                getLut<isTop>(l, x >> 1, y >> 1, z >> 1) = (value != 0);
            }
        }
    };

    if (size < 128) {
        for (int z = 0; z < size; z += 2)
            innerBody(z);
    } else {
        int threadCount = ThreadUtils::pool->threadCount();
        ThreadUtils::pool->enqueue([&](uint32 id, uint32, uint32) {
            int start = (((size/2)*id)/threadCount)*2;
            int   end = (((size/2)*(id + 1))/threadCount)*2;

            for (int z = start; z < end; z += 2)
                innerBody(z);
        }, threadCount)->wait();
    }
}

void VoxelData::buildTopLutBlock(int cx, int cy, int cz) {
    int size = int(_maxCacheableSize);
    int bufferW = std::min(size, _dataW - cx);
    int bufferH = std::min(size, _dataH - cy);
    int bufferD = std::min(size, _dataD - cz);
    if (bufferW <= 0 || bufferH <= 0 || bufferD <= 0)
        return;

    bool empty;
    if (_loader) {
        empty = _loader->isBlockEmpty(cx, cy, cz);
    } else {
        cacheData(cx, cy, cz, bufferW, bufferH, bufferD);

        empty = true;
        for (size_t i = 0; i < size_t(bufferW)*size_t(bufferH)*size_t(bufferD); ++i) {
            if (_bufferedData[i]) {
                empty = false;
                break;
            }
        }
    }

    if (!empty)
        getTopLut(_topLutLevels - 1, cx >> _lowLutLevels, cy >> _lowLutLevels, cz >> _lowLutLevels) = 1;
}

void VoxelData::buildTopLut() {
    if (_topLutLevels == 0)
        return;

    for (int z = 0; z < _virtualDataD; z += int(_maxCacheableSize))
        for (int y = 0; y < _virtualDataH; y += int(_maxCacheableSize))
            for (int x = 0; x < _virtualDataW; x += int(_maxCacheableSize))
                buildTopLutBlock(x, y, z);

    for (int i = _topLutLevels - 2; i >= 0; i--)
        upsampleLutLevel<true>(i);
}

void VoxelData::buildLowLut() {
    int threadCount = ThreadUtils::pool->threadCount();
    ThreadUtils::pool->enqueue([&](uint32 id, uint32, uint32) {
        int start = (((_bufferD/2)*id)/threadCount)*2;
        int   end = (((_bufferD/2)*(id + 1))/threadCount)*2;

        for (int z = start; z < end; ++z)
            for (int y = 0; y < _bufferH; ++y)
                for (int x = 0; x < _bufferW; ++x)
                    if (_bufferedData[x + size_t(_bufferW)*(y + size_t(_bufferH)*z)])
                        getLowLut(_lowLutLevels - 1, x/2, y/2, z/2) = 1;
    }, threadCount)->wait();

    for (int i = _lowLutLevels - 2; i >= 0; i--)
        upsampleLutLevel<false>(i);
}

void VoxelData::cacheData(int x, int y, int z, int w, int h, int d) {
    if (_loader)  {
        _loader->processBlock(_bufferedData.get(), x, y, z, w, h, d);
        return;
    }

    uint64_t zStride = _dataH*(uint64_t)_dataW;
    uint64_t yStride = (uint64_t)_dataW;
    uint64_t offsetZ = z*zStride;
    uint64_t offsetY = y*yStride;
    uint64_t offsetX = x;
    uint64_t baseOffset = (offsetX + offsetY + offsetZ)*sizeof(uint32) + 3*sizeof(uint32);
    uint64_t offset = 0;

    for (uint64_t voxelZ = 0; voxelZ < (unsigned)d; voxelZ++) {
        for (uint64_t voxelY = 0; voxelY < (unsigned)h; voxelY++) {
#ifdef _MSC_VER
            _fseeki64(_dataStream, baseOffset + offset, SEEK_SET);
#elif __APPLE__
            fseeko(_dataStream, baseOffset + offset, SEEK_SET);
#else
            fseeko64(_dataStream, baseOffset + offset, SEEK_SET);
#endif

            fread(_bufferedData.get() + (voxelY + voxelZ*h)*w, sizeof(uint32), w, _dataStream);

            offset += yStride*sizeof(uint32);
        }

        baseOffset += zStride*sizeof(uint32);
        offset = 0;
    }
}

void VoxelData::init(size_t mem) {
    _virtualDataW = roundToPow2(_dataW);
    _virtualDataH = roundToPow2(_dataH);
    _virtualDataD = roundToPow2(_dataD);
    _highestVirtualBit = findHighestBit(std::max(_virtualDataW, std::max(_virtualDataH, _virtualDataD)));

    _cellCost = sizeof(uint32);
    if (_loader)
        _cellCost += _loader->blockMemRequirement(1, 1, 1);

    auto partitionCost = [&](int level) {
        int lowBit = _highestVirtualBit - level;
        uint64 topLutCost = countCellsInHiarchicalGrid(level + 1);
        uint64 lowLutCost = countCellsInHiarchicalGrid(lowBit);
        uint64 gridCost = (uint64(1) << uint64(lowBit*3))*_cellCost;
        return topLutCost + lowLutCost + gridCost;
    };

    int largestLowerLevel = -1;
    uint64 smallestSize = std::numeric_limits<uint64>::max();
    for (int i = _highestVirtualBit; i >= 0; --i) {
        if (partitionCost(i) < mem)
            largestLowerLevel = _highestVirtualBit - i;
        smallestSize = std::min(smallestSize, partitionCost(i));
    }
    if (largestLowerLevel == -1) {
        std::cout << "Not enough memory to convert voxel volume. Require at least "
                  << prettyPrintMemory(smallestSize) << std::endl;
        std::exit(-1);
    }

    _lowLutLevels = largestLowerLevel;
    _topLutLevels = _highestVirtualBit - largestLowerLevel + 1;
    _maxCacheableSize = size_t(1) << largestLowerLevel;

    _topLut.reset(new uint8[countCellsInHiarchicalGrid(_topLutLevels)]());
    _lowLut.reset(new uint8[countCellsInHiarchicalGrid(_lowLutLevels)]());
    _bufferedData.reset(new uint32[size_t(1) << size_t(largestLowerLevel*3)]());

    std::cout << "Using a cache block of size " << _maxCacheableSize << "^3, taking up "
              << prettyPrintMemory(partitionCost(_topLutLevels - 1)) << " in memory.";
    if (largestLowerLevel < _highestVirtualBit) {
        std::cout << " For the next size of " << _maxCacheableSize*2 << "^3, you would need "
                  << prettyPrintMemory(partitionCost(_topLutLevels - 2)) << " of memory";
    }
    std::cout << std::endl;

    buildHierarchicalGridLinks(_topLut.get(), _topLutLevels, _topTable);
    buildHierarchicalGridLinks(_lowLut.get(), _lowLutLevels, _lowTable);

    _bufferX = _bufferY = _bufferZ = _bufferW = _bufferH = _bufferD = 0;
}

void VoxelData::prepareDataAccess(int x, int y, int z, int size) {
    int width  = std::min(size, _dataW - x);
    int height = std::min(size, _dataH - y);
    int depth  = std::min(size, _dataD - z);

    if (width <= 0 || height <= 0 || depth <= 0)
        return;

    if (x >= _bufferX &&
        y >= _bufferY &&
        z >= _bufferZ &&
        x + width  <= _bufferX + _bufferW &&
        y + height <= _bufferY + _bufferH &&
        z + depth  <= _bufferZ + _bufferD)
            return;

    if (size_t(size) <= _maxCacheableSize) {
        _bufferX = x;
        _bufferY = y;
        _bufferZ = z;
        _bufferW = width;
        _bufferH = height;
        _bufferD = depth;

        cacheData(x, y, z, width, height, depth);
        buildLowLut();
    }
}

int VoxelData::sideLength() const {
    return std::max(_virtualDataW, std::max(_virtualDataH, _virtualDataD));
}

Vec3 VoxelData::getCenter() const {
    return Vec3(
        _dataW*0.5f/sideLength(),
        _dataH*0.5f/sideLength(),
        _dataD*0.5f/sideLength()
    );
}
