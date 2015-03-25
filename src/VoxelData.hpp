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

#ifndef VOXELDATA_HPP_
#define VOXELDATA_HPP_

#include "math/Vec3.hpp"

#include "IntTypes.hpp"
#include "Util.hpp"

#include <stdio.h>
#include <memory>
#include <vector>

class PlyLoader;

class VoxelData {
    FILE *_dataStream;
    PlyLoader *_loader;

    int _dataW;
    int _dataH;
    int _dataD;

    int _virtualDataW;
    int _virtualDataH;
    int _virtualDataD;

    int _highestVirtualBit;
    int _lowLutLevels;

    size_t _maxCacheableSize;

    size_t _cellCost;

    std::unique_ptr<uint8[]> _topLut, _lowLut;
    std::vector<uint8 *> _topTable;
    std::vector<uint8 *> _lowTable;
    int _minLutStep;
    int _topLutLevels;

    std::unique_ptr<uint32[]> _bufferedData;

    int _bufferX, _bufferY, _bufferZ;
    int _bufferW, _bufferH, _bufferD;

    template<bool isTop>
    void upsampleLutLevel(int l);

    void buildTopLutBlock(int x, int y, int z);

    void buildTopLut();
    void buildLowLut();

    void cacheData(int x, int y, int z, int w, int h, int d);

    void init(size_t mem);

    inline size_t lutIdx(int l, int x, int y, int z) const {
        return x + ((size_t)y << l) + ((size_t)z << 2*l);
    }

    inline uint8 &getTopLut(int l, int x, int y, int z) {
        return _topTable[l][lutIdx(l, x, y, z)];
    }

    inline uint8 &getLowLut(int l, int x, int y, int z) {
        return _lowTable[l][lutIdx(l, x, y, z)];
    }

    template<bool isTop>
    inline uint8 &getLut(int l, int x, int y, int z)
    {
        if (isTop)
            return getTopLut(l, x, y, z);
        else
            return getLowLut(l, x, y, z);
    }

public:
    VoxelData(const char *path, size_t mem);
    VoxelData(PlyLoader *loader, int sideLength, size_t mem);
    ~VoxelData();

    inline uint32 getVoxel(int x, int y, int z) const {
        if (x >= _dataW || y >= _dataH || z >= _dataD)
            return 0;
        size_t idx = size_t(x - _bufferX) + _bufferW*(size_t(y - _bufferY) + _bufferH*size_t(z - _bufferZ));
        return _bufferedData[idx];
    }

    inline uint32 getVoxelDestructive(int x, int y, int z) {
        if (x >= _dataW || y >= _dataH || z >= _dataD)
            return 0;
        size_t idx = size_t(x - _bufferX) + _bufferW*(size_t(y - _bufferY) + _bufferH*size_t(z - _bufferZ));
        uint32 value = _bufferedData[idx];
        if (value)
            _bufferedData[idx] = 0;
        return value;
    }

    inline bool cubeContainsVoxelsDestructive(int x, int y, int z, int size) {
        if (x >= _dataW || y >= _dataH || z >= _dataD)
            return false;

        int bit = findHighestBit(size);
        if (size == 1) {
            return getVoxel(x, y, z) != 0;
        } else if (bit < _lowLutLevels) {
            uint8 value = getLowLut(_lowLutLevels - bit, (x - _bufferX) >> bit, (y - _bufferY) >> bit, (z - _bufferZ) >> bit);
            if (value != 0)
                getLowLut(_lowLutLevels - bit, (x - _bufferX) >> bit, (y - _bufferY) >> bit, (z - _bufferZ) >> bit) = 0;
            return value != 0;
        } else {
            return getTopLut(_highestVirtualBit - bit, x >> bit, y >> bit, z >> bit) != 0;
        }
    }

    void prepareDataAccess(int x, int y, int z, int size);

    int sideLength() const;
    Vec3 getCenter() const;
};


#endif /* VOXELDATA_H_ */
