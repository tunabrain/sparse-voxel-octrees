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

#include "VoxelOctree.hpp"
#include "VoxelData.hpp"
#include "Debug.hpp"
#include "Util.hpp"

#include "third-party/lz4.h"

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <cmath>

static const uint32 BitCount[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static const size_t CompressionBlockSize = 64*1024*1024;

VoxelOctree::VoxelOctree(const char *path) : _voxels(0) {
    FILE *fp = fopen(path, "rb");

    if (fp) {

        fread(_center.a, sizeof(float), 3, fp);
        fread(&_octreeSize, sizeof(uint64), 1, fp);

        _octree.reset(new uint32[_octreeSize]);

        std::unique_ptr<char[]> buffer(new char[LZ4_compressBound(CompressionBlockSize)]);
        char *dst = reinterpret_cast<char *>(_octree.get());

        LZ4_streamDecode_t *stream = LZ4_createStreamDecode();
        LZ4_setStreamDecode(stream, dst, 0);

        uint64 compressedSize = 0;
        for (uint64 offset = 0; offset < _octreeSize*sizeof(uint32); offset += CompressionBlockSize) {
            uint64 compSize;
            fread(&compSize, sizeof(uint64), 1, fp);
            fread(buffer.get(), sizeof(char), size_t(compSize), fp);

            int outSize = std::min<int>(_octreeSize*sizeof(uint32) - offset, CompressionBlockSize);
            LZ4_decompress_fast_continue(stream, buffer.get(), dst + offset, outSize);
            compressedSize += compSize + 8;
        }
        LZ4_freeStreamDecode(stream);

        fclose(fp);

        std::cout << "Octree size: " << prettyPrintMemory(_octreeSize*sizeof(uint32))
                  << " Compressed size: " << prettyPrintMemory(compressedSize) << std::endl;
    }
}

void VoxelOctree::save(const char *path) {
    FILE *fp = fopen(path, "wb");

    if (fp) {
        fwrite(_center.a, sizeof(float), 3, fp);
        fwrite(&_octreeSize, sizeof(uint64), 1, fp);

        LZ4_stream_t *stream = LZ4_createStream();
        LZ4_resetStream(stream);

        std::unique_ptr<char[]> buffer(new char[LZ4_compressBound(CompressionBlockSize)]);
        const char *src = reinterpret_cast<char *>(_octree.get());

        uint64 compressedSize = 0;
        for (uint64 offset = 0; offset < _octreeSize*sizeof(uint32); offset += CompressionBlockSize) {
            int outSize = int(std::min(_octreeSize*sizeof(uint32) - offset, uint64(CompressionBlockSize)));
            uint64 compSize = LZ4_compress_continue(stream, src + offset, buffer.get(), outSize);

            fwrite(&compSize, sizeof(uint64), 1, fp);
            fwrite(buffer.get(), sizeof(char), size_t(compSize), fp);

            compressedSize += compSize + 8;
        }

        LZ4_freeStream(stream);

        fclose(fp);

        std::cout << "Octree size: " << prettyPrintMemory(_octreeSize*sizeof(uint32))
                  << " Compressed size: " << prettyPrintMemory(compressedSize) << std::endl;
    }
}

VoxelOctree::VoxelOctree(VoxelData *voxels)
: _voxels(voxels)
{
    std::unique_ptr<ChunkedAllocator<uint32>> octreeAllocator(new ChunkedAllocator<uint32>());
    octreeAllocator->pushBack(0);

    buildOctree(*octreeAllocator, 0, 0, 0, _voxels->sideLength(), 0);
    (*octreeAllocator)[0] |= 1 << 18;

    _octreeSize = octreeAllocator->size() + octreeAllocator->insertionCount();
    _octree = octreeAllocator->finalize();
    _center = _voxels->getCenter();
}

uint64 VoxelOctree::buildOctree(ChunkedAllocator<uint32> &allocator, int x, int y, int z, int size, uint64 descriptorIndex) {
    _voxels->prepareDataAccess(x, y, z, size);

    int halfSize = size >> 1;

    int posX[] = {x + halfSize, x, x + halfSize, x, x + halfSize, x, x + halfSize, x};
    int posY[] = {y + halfSize, y + halfSize, y, y, y + halfSize, y + halfSize, y, y};
    int posZ[] = {z + halfSize, z + halfSize, z + halfSize, z + halfSize, z, z, z, z};

    uint64 childOffset = uint64(allocator.size()) - descriptorIndex;

    int childCount = 0;
    int childIndices[8];
    uint32 childMask = 0;
    for (int i = 0; i < 8; i++) {
        if (_voxels->cubeContainsVoxelsDestructive(posX[i], posY[i], posZ[i], halfSize)) {
            childMask |= 128 >> i;
            childIndices[childCount++] = i;
        }
    }

    bool hasLargeChildren = false;
    uint32 leafMask;
    if (halfSize == 1) {
        leafMask = 0;

        for (int i = 0; i < childCount; i++) {
            int idx = childIndices[childCount - i - 1];
            allocator.pushBack(_voxels->getVoxelDestructive(posX[idx], posY[idx], posZ[idx]));
        }
    } else {
        leafMask = childMask;
        for (int i = 0; i < childCount; i++)
            allocator.pushBack(0);

        uint64 grandChildOffsets[8];
        uint64 delta = 0;
        uint64 insertionCount = allocator.insertionCount();
        for (int i = 0; i < childCount; i++) {
            int idx = childIndices[childCount - i - 1];
            grandChildOffsets[i] = delta + buildOctree(allocator, posX[idx], posY[idx], posZ[idx],
                halfSize, descriptorIndex + childOffset + i);
            delta += allocator.insertionCount() - insertionCount;
            insertionCount = allocator.insertionCount();
            if (grandChildOffsets[i] > 0x3FFF)
                hasLargeChildren = true;
        }

        for (int i = 0; i < childCount; i++) {
            uint64 childIndex = descriptorIndex + childOffset + i;
            uint64 offset = grandChildOffsets[i];
            if (hasLargeChildren) {
                offset += childCount - i;
                allocator.insert(childIndex + 1, uint32(offset));
                allocator[childIndex] |= 0x20000;
                offset >>= 32;
            }
            allocator[childIndex] |= uint32(offset << 18);
        }
    }

    allocator[descriptorIndex] = (childMask << 8) | leafMask;
    if (hasLargeChildren)
        allocator[descriptorIndex] |= 0x10000;

    return childOffset;
}

bool VoxelOctree::raymarch(const Vec3 &o, const Vec3 &d, float rayScale, uint32 &normal, float &t) {
    struct StackEntry {
        uint64 offset;
        float maxT;
    };
    StackEntry rayStack[MaxScale + 1];

    float ox = o.x, oy = o.y, oz = o.z;
    float dx = d.x, dy = d.y, dz = d.z;

    if (std::fabs(dx) < 1e-4f) dx = 1e-4f;
    if (std::fabs(dy) < 1e-4f) dy = 1e-4f;
    if (std::fabs(dz) < 1e-4f) dz = 1e-4f;

    float dTx = 1.0f/-std::fabs(dx);
    float dTy = 1.0f/-std::fabs(dy);
    float dTz = 1.0f/-std::fabs(dz);

    float bTx = dTx*ox;
    float bTy = dTy*oy;
    float bTz = dTz*oz;

    uint8 octantMask = 7;
    if (dx > 0.0f) octantMask ^= 1, bTx = 3.0f*dTx - bTx;
    if (dy > 0.0f) octantMask ^= 2, bTy = 3.0f*dTy - bTy;
    if (dz > 0.0f) octantMask ^= 4, bTz = 3.0f*dTz - bTz;

    float minT = std::max(2.0f*dTx - bTx, std::max(2.0f*dTy - bTy, 2.0f*dTz - bTz));
    float maxT = std::min(     dTx - bTx, std::min(     dTy - bTy,      dTz - bTz));
    minT = std::max(minT, 0.0f);

    uint32 current = 0;
    uint64 parent  = 0;
    int idx     = 0;
    float posX  = 1.0f;
    float posY  = 1.0f;
    float posZ  = 1.0f;
    int scale   = MaxScale - 1;

    float scaleExp2 = 0.5f;

    if (1.5f*dTx - bTx > minT) idx ^= 1, posX = 1.5f;
    if (1.5f*dTy - bTy > minT) idx ^= 2, posY = 1.5f;
    if (1.5f*dTz - bTz > minT) idx ^= 4, posZ = 1.5f;

    while (scale < MaxScale) {
        if (current == 0)
            current = _octree[parent];

        float cornerTX = posX*dTx - bTx;
        float cornerTY = posY*dTy - bTy;
        float cornerTZ = posZ*dTz - bTz;
        float maxTC = std::min(cornerTX, std::min(cornerTY, cornerTZ));

        int childShift = idx ^ octantMask;
        uint32 childMasks = current << childShift;

        if ((childMasks & 0x8000) && minT <= maxT) {
            if (maxTC*rayScale >= scaleExp2) {
                t = maxTC;
                return true;
            }

            float maxTV = std::min(maxT, maxTC);
            float half = scaleExp2*0.5f;
            float centerTX = half*dTx + cornerTX;
            float centerTY = half*dTy + cornerTY;
            float centerTZ = half*dTz + cornerTZ;

            if (minT <= maxTV) {
                uint64 childOffset = current >> 18;
                if (current & 0x20000)
                    childOffset = (childOffset << 32) | uint64(_octree[parent + 1]);

                if (!(childMasks & 0x80)) {
                    normal = _octree[childOffset + parent + BitCount[((childMasks >> (8 + childShift)) << childShift) & 127]];

                    break;
                }

                rayStack[scale].offset = parent;
                rayStack[scale].maxT = maxT;

                uint32 siblingCount = BitCount[childMasks & 127];
                parent += childOffset + siblingCount;
                if (current & 0x10000)
                    parent += siblingCount;

                idx = 0;
                scale--;
                scaleExp2 = half;

                if (centerTX > minT) idx ^= 1, posX += scaleExp2;
                if (centerTY > minT) idx ^= 2, posY += scaleExp2;
                if (centerTZ > minT) idx ^= 4, posZ += scaleExp2;

                maxT = maxTV;
                current = 0;

                continue;
            }
        }

        int stepMask = 0;
        if (cornerTX <= maxTC) stepMask ^= 1, posX -= scaleExp2;
        if (cornerTY <= maxTC) stepMask ^= 2, posY -= scaleExp2;
        if (cornerTZ <= maxTC) stepMask ^= 4, posZ -= scaleExp2;

        minT = maxTC;
        idx ^= stepMask;

        if ((idx & stepMask) != 0) {
            int differingBits = 0;
            if (stepMask & 1) differingBits |= floatBitsToUint(posX) ^ floatBitsToUint(posX + scaleExp2);
            if (stepMask & 2) differingBits |= floatBitsToUint(posY) ^ floatBitsToUint(posY + scaleExp2);
            if (stepMask & 4) differingBits |= floatBitsToUint(posZ) ^ floatBitsToUint(posZ + scaleExp2);
            scale = (floatBitsToUint((float)differingBits) >> 23) - 127;
            scaleExp2 = uintBitsToFloat((scale - MaxScale + 127) << 23);

            parent = rayStack[scale].offset;
            maxT   = rayStack[scale].maxT;

            int shX = floatBitsToUint(posX) >> scale;
            int shY = floatBitsToUint(posY) >> scale;
            int shZ = floatBitsToUint(posZ) >> scale;
            posX = uintBitsToFloat(shX << scale);
            posY = uintBitsToFloat(shY << scale);
            posZ = uintBitsToFloat(shZ << scale);
            idx = (shX & 1) | ((shY & 1) << 1) | ((shZ & 1) << 2);

            current = 0;
        }
    }

    if (scale >= MaxScale)
        return false;

    t = minT;
    return true;
}
