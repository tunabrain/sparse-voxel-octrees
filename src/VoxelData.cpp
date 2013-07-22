#include <algorithm>
#include <string.h>

#include "VoxelData.hpp"
#include "Debug.hpp"
#include "Util.hpp"

using namespace std;

#include <windows.h>
static LARGE_INTEGER timeStart, timeStop;
static void startTimer() {
    QueryPerformanceCounter(&timeStart);
}

static double stopTimer() {
    QueryPerformanceCounter(&timeStop);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return (timeStop.QuadPart - timeStart.QuadPart)/(double)frequency.QuadPart*1000.0;
}

VoxelData::VoxelData(const char* File) {
	_dataStream = fopen(File, "rb");

	if (_dataStream != NULL) {
		fread(&_dataD, 4, 1, _dataStream);
		fread(&_dataH, 4, 1, _dataStream);
		fread(&_dataW, 4, 1, _dataStream);

		virtualDataW = roundToPow2(_dataW);
		virtualDataH = roundToPow2(_dataH);
		virtualDataD = roundToPow2(_dataD);
		printf("%d %d %d\n", virtualDataD, virtualDataH, virtualDataW);
		_highestVirtualBit = findHighestBit(max(virtualDataW, max(virtualDataH, virtualDataD)));

		_lowerThreshold = 17000;
		_upperThreshold = 0xFFFF;

		_maxDataMemory = 512*1024*1024;
		_maxLutMemory  = 512*1024*1024;

		_maxCacheableSize = 1;
		while ((_maxCacheableSize + 2)*(_maxCacheableSize + 2)*(_maxCacheableSize + 2)*8ULL*sizeof(uint8_t) < _maxDataMemory)
			_maxCacheableSize <<= 1;

		_lut = new uint8_t[_maxLutMemory];
		memset(_lut, 0, _maxLutMemory);

		_lutLevels = 0;

		_bufferedData = new uint16_t[_maxDataMemory/sizeof(uint16_t)];
		memset(_bufferedData, 0, _maxDataMemory);

		_bufferX = _bufferY = _bufferZ = _bufferW = _bufferH = _bufferD = 0;

		precalcLut();
	}
}

VoxelData::~VoxelData() {
	delete[] _lut;
	delete[] _bufferedData;

	fclose(_dataStream);
}

void VoxelData::prepareDataAccess(int x, int y, int z, int size) {
	x = max(x - 1, 0);
	y = max(y - 1, 0);
	z = max(z - 1, 0);

	int64_t width  = min(size + 2, _dataW - x);
	int64_t height = min(size + 2, _dataH - y);
	int64_t depth  = min(size + 2, _dataD - z);

	if (width <= 0 || height <= 0 || depth <= 0)
		return;

	if (x >= _bufferX &&
		y >= _bufferY &&
		z >= _bufferZ &&
		x + width  <= _bufferX + _bufferW &&
		y + height <= _bufferY + _bufferH &&
		z + depth  <= _bufferZ + _bufferD)
			return;

	uint64_t dataSize = width*height*depth;
	if (dataSize*sizeof(uint16_t) <= _maxDataMemory) {
		cacheData(x, y, z, width, height, depth);

		_bufferX = x;
		_bufferY = y;
		_bufferZ = z;
		_bufferW = width;
		_bufferH = height;
		_bufferD = depth;
	}
}

uint8_t &VoxelData::getLut(int l, int x, int y, int z) {
	return _levelTable[l][x + ((size_t)y << l) + ((size_t)z << 2*l)];
}

void VoxelData::buildBlockLut(int cx, int cy, int cz) {
	prepareDataAccess(cx, cy, cz, _maxCacheableSize);

	int boundX = cx + _maxCacheableSize, lutX = cx/_minLutStep;
	int boundY = cy + _maxCacheableSize, lutY = cy/_minLutStep;
	int boundZ = cz + _maxCacheableSize, lutZ = cz/_minLutStep;

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
	size_t currentReq =  1;
	size_t totalReq   =  0;
	_lutLevels        = -1;

	while (totalReq + currentReq <= _maxLutMemory && _lutLevels < _highestVirtualBit) {
		_lutLevels++;
		_levelTable.push_back(&_lut[totalReq]);
		totalReq += currentReq;
		currentReq *= 8;
	}

	_minLutStep = max(virtualDataW, max(virtualDataH, virtualDataD)) >> _lutLevels;

	startTimer();

	for (int CacheZ = 0; CacheZ < virtualDataD; CacheZ += _maxCacheableSize)
		for (int CacheY = 0; CacheY < virtualDataH; CacheY += _maxCacheableSize)
			for (int CacheX = 0; CacheX < virtualDataW; CacheX += _maxCacheableSize)
				buildBlockLut(CacheX, CacheY, CacheZ);

	printf("BlockLut built in %fms\n", stopTimer());
	startTimer();

	for (int i = _lutLevels - 1; i >= 0; i--)
		upsampleLutLevel(i);

	printf("Upsampled Lut built in %fms\n", stopTimer());
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
				if (IsContourPoint(cx, cy, cz))
					return true;

	return false;
}

void VoxelData::cacheData(int x, int y, int z, int w, int h, int d) {
	uint64_t zStride = _dataH*(uint64_t)_dataW;
	uint64_t yStride = (uint64_t)_dataW;
	uint64_t offsetZ = z*zStride;
	uint64_t offsetY = y*yStride;
	uint64_t offsetX = x;
	uint64_t baseOffset = ((offsetX + offsetY + offsetZ) << 1) + 4*sizeof(uint32_t);
	uint64_t offset = 0;

	for (uint64_t voxelZ = 0; voxelZ < d; voxelZ++) {
		for (uint64_t voxelY = 0; voxelY < h; voxelY++) {
			fseeko64(_dataStream, baseOffset + offset, SEEK_SET);
			fread(_bufferedData + (voxelY + voxelZ*h)*w, sizeof(uint16_t), w, _dataStream);

			offset += yStride*sizeof(uint16_t);
		}

		baseOffset += zStride*sizeof(uint16_t);
		offset = 0;
	}
}

uint16_t VoxelData::GetVoxel(int X, int Y, int Z) {
	if (X < 0 || Y < 0 || Z < 0 || X >= _dataW || Y >= _dataH || Z >= _dataD)
		return 0;

	ASSERT(
		X >= _bufferX && Y >= _bufferY && Z >= _bufferZ && X < _bufferW + _bufferX &&
		Y < _bufferH + _bufferY && Z < _bufferD + _bufferZ,
		"Non-cached buffer access not supported. "
		"Call PrepareDataAccess before using this function.\n"
	);

	uint64_t CubeX = X - _bufferX;
	uint64_t CubeY = Y - _bufferY;
	uint64_t CubeZ = Z - _bufferZ;

	CubeZ *= (uint64_t)_bufferH*(uint64_t)_bufferW;
	CubeY *= (uint64_t)_bufferW;

	return _bufferedData[CubeX + CubeY + CubeZ];
}

bool VoxelData::IsContourPoint(int X, int Y, int Z) {
	if (X >= _dataW - 1 || Y >= _dataH - 1 || Z >= _dataD - 1)
		return false;

	uint16_t D1 = GetVoxel(X, Y, Z);
	uint16_t D2 = GetVoxel(X + 1, Y, Z);
	uint16_t D3 = GetVoxel(X, Y + 1, Z);
	uint16_t D4 = GetVoxel(X, Y, Z + 1);

	bool B1 = (D1 >= _lowerThreshold && D1 <= _upperThreshold);
	bool B2 = (D2 >= _lowerThreshold && D2 <= _upperThreshold);
	bool B3 = (D3 >= _lowerThreshold && D3 <= _upperThreshold);
	bool B4 = (D4 >= _lowerThreshold && D4 <= _upperThreshold);

	return (B1 != B2 || B1 != B3 || B1 != B4);
}

uint32_t VoxelData::GetNormal(int X, int Y, int Z) {
	Vec3 n(
			(GetVoxel(X + 1, Y, Z) - GetVoxel(X - 1, Y, Z))*1.52590218e-5,
			(GetVoxel(X, Y + 1, Z) - GetVoxel(X, Y - 1, Z))*1.52590218e-5,
			(GetVoxel(X, Y, Z + 1) - GetVoxel(X, Y, Z - 1))*1.52590218e-5
	);
	fastNormalization(n);

	return compressNormal(n);
}
