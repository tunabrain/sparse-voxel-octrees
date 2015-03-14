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

#include <algorithm>
#include <string.h>

#include "VoxelData.hpp"
#include "PlyLoader.hpp"
#include "Debug.hpp"
#include "Util.hpp"

using namespace std;

VoxelData::VoxelData(const char *path, size_t lutMem, size_t dataMem) : _loader(0) {
    _dataStream = fopen(path, "rb");

    if (_dataStream != NULL) {
        fread(&_dataW, 4, 1, _dataStream);
        fread(&_dataH, 4, 1, _dataStream);
        fread(&_dataD, 4, 1, _dataStream);

        init(lutMem, dataMem);
        precalcLut();
    }
}

VoxelData::VoxelData(PlyLoader *loader, int sideLength, size_t lutMem, size_t dataMem) :
        _dataStream(0), _loader(loader) {

    loader->suggestedDimensions(sideLength, _dataW, _dataH, _dataD);
    init(lutMem, dataMem);
    loader->setupBlockProcessing(_maxCacheableSize*_maxCacheableSize*_maxCacheableSize, sideLength);
    precalcLut();
}

VoxelData::~VoxelData() {
    delete[] _lut;
    delete[] _bufferedData;

    if (_dataStream)
        fclose(_dataStream);
    else if (_loader)
        _loader->teardownBlockProcessing();
}

void VoxelData::init(size_t lutMem, size_t dataMem) {
    _virtualDataW = roundToPow2(_dataW);
    _virtualDataH = roundToPow2(_dataH);
    _virtualDataD = roundToPow2(_dataD);
    _highestVirtualBit = findHighestBit(max(_virtualDataW, max(_virtualDataH, _virtualDataD)));

    _maxDataMemory = dataMem;
    _maxLutMemory  = lutMem;

    _cellCost = sizeof(uint32_t);
    if (_loader)
        _cellCost += _loader->blockMemRequirement(1, 1, 1);

    _maxCacheableSize = 1;
    while (_maxCacheableSize*_maxCacheableSize*_maxCacheableSize*8*_cellCost < _maxDataMemory)
        _maxCacheableSize <<= 1;
    _maxCacheableSize = min(_maxCacheableSize, size_t(1 << _highestVirtualBit));

    allocateLut();

    _bufferedData = new uint32_t[_maxDataMemory/sizeof(uint32_t)];
    memset(_bufferedData, 0, _maxDataMemory);

    _bufferX = _bufferY = _bufferZ = _bufferW = _bufferH = _bufferD = 0;
}

int VoxelData::sideLength() const {
    return max(_virtualDataW, max(_virtualDataH, _virtualDataD));
}

void VoxelData::prepareDataAccess(int x, int y, int z, int size) {
    int width  = min(size, _dataW - x);
    int height = min(size, _dataH - y);
    int depth  = min(size, _dataD - z);

    if (width <= 0 || height <= 0 || depth <= 0)
        return;

    if (x >= _bufferX &&
        y >= _bufferY &&
        z >= _bufferZ &&
        x + width  <= _bufferX + _bufferW &&
        y + height <= _bufferY + _bufferH &&
        z + depth  <= _bufferZ + _bufferD)
            return;

    uint64_t dataSize = uint64_t(width)*uint64_t(height)*uint64_t(depth);
    if (dataSize*_cellCost <= _maxDataMemory) {
        cacheData(x, y, z, width, height, depth);

        _bufferX = x;
        _bufferY = y;
        _bufferZ = z;
        _bufferW = width;
        _bufferH = height;
        _bufferD = depth;
    }
}

void VoxelData::allocateLut() {
    size_t currentReq =  1;
    size_t totalReq   =  0;
    _lutLevels        = -1;

    vector<size_t> indices;
    while (totalReq + currentReq <= _maxLutMemory && _lutLevels < _highestVirtualBit) {
        _lutLevels++;
        indices.push_back(totalReq);
        totalReq += currentReq;
        currentReq *= 8;
    }

    _minLutStep = max(_virtualDataW, max(_virtualDataH, _virtualDataD)) >> _lutLevels;

    _lut = new uint8_t[totalReq];
    memset(_lut, 0, totalReq);

    for (unsigned i = 0; i < indices.size(); i++)
        _levelTable.push_back(&_lut[indices[i]]);
}

void VoxelData::buildBlockLut(int cx, int cy, int cz) {
    prepareDataAccess(cx, cy, cz, int(_maxCacheableSize));

    int boundX = cx + int(_maxCacheableSize), lutX = cx/_minLutStep;
    int boundY = cy + int(_maxCacheableSize), lutY = cy/_minLutStep;
    int boundZ = cz + int(_maxCacheableSize), lutZ = cz/_minLutStep;

    for (int z = cz, lz = lutZ; z < boundZ; z += _minLutStep, lz++)
        for (int y = cy, ly = lutY; y < boundY; y += _minLutStep, ly++)
            for (int x = cx, lx = lutX; x < boundX; x += _minLutStep, lx++)
                getLut(_lutLevels, lx, ly, lz) = cubeContainsVoxelsRaw(x, y, z, _minLutStep);
}

void VoxelData::upsampleLutLevel(int l) {
    int size = 2 << l;

    for (int x = 0; x < size; x += 2) {
        for (int y = 0; y < size; y += 2) {
            for (int z = 0; z < size; z += 2) {
                int value =
                    getLut(l + 1, x, y + 0, z + 0) + getLut(l + 1, x + 1, y + 0, z + 0) +
                    getLut(l + 1, x, y + 1, z + 0) + getLut(l + 1, x + 1, y + 1, z + 0) +
                    getLut(l + 1, x, y + 0, z + 1) + getLut(l + 1, x + 1, y + 0, z + 1) +
                    getLut(l + 1, x, y + 1, z + 1) + getLut(l + 1, x + 1, y + 1, z + 1);

                getLut(l, x >> 1, y >> 1, z >> 1) = (value != 0);
            }
        }
    }
}

void VoxelData::precalcLut() {
    for (int z = 0; z < _virtualDataD; z += int(_maxCacheableSize))
        for (int y = 0; y < _virtualDataH; y += int(_maxCacheableSize))
            for (int x = 0; x < _virtualDataW; x += int(_maxCacheableSize))
                buildBlockLut(x, y, z);

    for (int i = _lutLevels - 1; i >= 0; i--)
        upsampleLutLevel(i);
}

uint8_t &VoxelData::getLut(int l, int x, int y, int z) {
    return _levelTable[l][x + ((size_t)y << l) + ((size_t)z << 2*l)];
}

bool VoxelData::cubeContainsVoxels(int x, int y, int z, int size) {
    if (x >= _dataW || y >= _dataH || z >= _dataD)
        return false;

    if (size < _minLutStep)
        return cubeContainsVoxelsRaw(x, y, z, size);
    else
        return getLut(_highestVirtualBit - findHighestBit(size), x/size, y/size, z/size);
}

bool VoxelData::cubeContainsVoxelsRaw(int x, int y, int z, int size) {
    int boundX = min(x + size, _dataW - 1);
    int boundY = min(y + size, _dataH - 1);
    int boundZ = min(z + size, _dataD - 1);

    for (int cz = z; cz < boundZ; cz++)
        for (int cy = y; cy < boundY; cy++)
            for (int cx = x; cx < boundX; cx++)
                if (getVoxel(cx, cy, cz) != 0)
                    return true;

    return false;
}

void VoxelData::cacheData(int x, int y, int z, int w, int h, int d) {
    if (_loader) {
        _loader->processBlock(_bufferedData, x, y, z, w, h, d);
        return;
    }

    uint64_t zStride = _dataH*(uint64_t)_dataW;
    uint64_t yStride = (uint64_t)_dataW;
    uint64_t offsetZ = z*zStride;
    uint64_t offsetY = y*yStride;
    uint64_t offsetX = x;
    uint64_t baseOffset = (offsetX + offsetY + offsetZ)*sizeof(uint32_t) + 3*sizeof(uint32_t);
    uint64_t offset = 0;

    for (uint64_t voxelZ = 0; voxelZ < (unsigned)d; voxelZ++) {
        for (uint64_t voxelY = 0; voxelY < (unsigned)h; voxelY++) {
#ifdef _MSC_VER
			_fseeki64(_dataStream, baseOffset + offset, SEEK_SET);
#else
            fseeko64(_dataStream, baseOffset + offset, SEEK_SET);
#endif
            fread(_bufferedData + (voxelY + voxelZ*h)*w, sizeof(uint32_t), w, _dataStream);

            offset += yStride*sizeof(uint32_t);
        }

        baseOffset += zStride*sizeof(uint32_t);
        offset = 0;
    }
}

uint32_t VoxelData::getVoxel(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= _dataW || y >= _dataH || z >= _dataD)
        return 0;

    ASSERT(
        x >= _bufferX && y >= _bufferY && z >= _bufferZ && x < _bufferW + _bufferX &&
        y < _bufferH + _bufferY && z < _bufferD + _bufferZ,
        "Non-cached buffer access not supported. "
        "Call PrepareDataAccess before using this function.\n"
    );

    size_t cubeX = x - _bufferX;
    size_t cubeY = y - _bufferY;
    size_t cubeZ = z - _bufferZ;

    cubeZ *= (size_t)_bufferH*(size_t)_bufferW;
    cubeY *= (size_t)_bufferW;

    return _bufferedData[cubeX + cubeY + cubeZ];
}

Vec3 VoxelData::getCenter() const {
    return Vec3(
        _dataW*0.5f/sideLength(),
        _dataH*0.5f/sideLength(),
        _dataD*0.5f/sideLength()
    );
}
