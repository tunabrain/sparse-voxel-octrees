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

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "math/Vec3.hpp"

class VoxelData {
	FILE *_dataStream;

	int _dataW;
	int _dataH;
	int _dataD;

	int _virtualDataW;
	int _virtualDataH;
	int _virtualDataD;

	int _highestVirtualBit;

	size_t _maxDataMemory;
	size_t _maxLutMemory;
	size_t _maxCacheableSize;

	uint8_t *_lut;
	std::vector<uint8_t *> _levelTable;
	int _minLutStep;
	int _lutLevels;

	uint32_t *_bufferedData;

	int _bufferX, _bufferY, _bufferZ;
	int _bufferW, _bufferH, _bufferD;

	uint8_t &getLut(int l, int x, int y, int z);
	void buildBlockLut(int x, int y, int z);
	void upsampleLutLevel(int l);
	void precalcLut();
	void cacheData(int x, int y, int z, int w, int h, int d);
	bool cubeContainsVoxelsRaw(int x, int y, int z, int size);

public:

	int sideLength() const;
	Vec3 getCenter() const;

	uint32_t getVoxel(int x, int y, int z);
	bool cubeContainsVoxels(int x, int y, int z, int size);
	void prepareDataAccess(int x, int y, int z, int size);

	VoxelData(const char *path, size_t lutMem, size_t dataMem);
	~VoxelData();
};


#endif /* VOXELDATA_H_ */
