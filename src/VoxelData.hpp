#ifndef VOXELDATA_HPP_
#define VOXELDATA_HPP_

#include <stdio.h>
#include <stdint.h>
#include <vector>

class VoxelData {
	FILE *_dataStream;

	int _dataW;
	int _dataH;
	int _dataD;

	int _highestVirtualBit;

	int _lowerThreshold;
	int _upperThreshold;
	size_t _maxDataMemory;
	size_t _maxLutMemory;
	size_t _maxCacheableSize;

	uint8_t *_lut;
	std::vector<uint8_t *> _levelTable;
	int _minLutStep;
	int _lutLevels;

	uint16_t *_bufferedData;

	int _bufferX, _bufferY, _bufferZ;
	int _bufferW, _bufferH, _bufferD;

	uint8_t &getLut(int l, int x, int y, int z);
	void buildBlockLut(int x, int y, int z);
	void upsampleLutLevel(int l);
	void precalcLut();
	void cacheData(int x, int y, int z, int w, int h, int d);
	bool cubeContainsVoxelsRaw(int x, int y, int z, int size);

public:

	int virtualDataW;
	int virtualDataH;
	int virtualDataD;

	uint16_t GetVoxel(int X, int Y, int Z);
	bool     cubeContainsVoxels(int StartX, int StartY, int StartZ, int Size);
	bool     IsContourPoint(int X, int Y, int Z);
	uint32_t GetNormal(int X, int Y, int Z);
	void     prepareDataAccess(int X, int Y, int Z, int Size);

	VoxelData(const char* File);
	~VoxelData();
};


#endif /* VOXELDATA_H_ */
