#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "VoxelOctree.hpp"
#include "VoxelData.hpp"
#include "Util.hpp"

using namespace std;

static const uint32_t BitCount[] = {
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

VoxelOctree::VoxelOctree(const char *path) : _voxels(0) {
	FILE* fp = fopen(path, "rb");

	if (fp) {
		int pointerCount;
		fread(&pointerCount, sizeof(uint32_t), 1, fp);
		_farPointers.resize(pointerCount);
		fread(&_farPointers[0], sizeof(uint32_t), pointerCount, fp);

		int dataSize;
		fread(&dataSize, sizeof(uint32_t), 1, fp);
		_octree.resize(dataSize/4);
		fread(&_octree[0], 1, dataSize, fp);

		fclose(fp);
	}
}

VoxelOctree::VoxelOctree(VoxelData *voxels) : _voxels(voxels) {
	int size = max(_voxels->virtualDataW, max(_voxels->virtualDataH, _voxels->virtualDataD));

	_octree.push_back(0);
	buildOctree(0, 0, 0, size, 0);
}

void VoxelOctree::buildOctree(int x, int y, int z, int size, uint32_t descriptorIndex) {
	_voxels->prepareDataAccess(x, y, z, size);

	int halfSize = size >> 1;

	int posX[] = {x + halfSize, x, x + halfSize, x, x + halfSize, x, x + halfSize, x};
	int posY[] = {y + halfSize, y + halfSize, y, y, y + halfSize, y + halfSize, y, y};
	int posZ[] = {z + halfSize, z + halfSize, z + halfSize, z + halfSize, z, z, z, z};

	uint8_t children = 0;
	for (int I = 0; I < 8; I++)
		if (_voxels->cubeContainsVoxels(posX[I], posY[I], posZ[I], halfSize))
			children |= 128 >> I;

	uint32_t childIndex = _octree.size() - descriptorIndex;

	uint8_t leafs;
	if (halfSize == 1) {
		leafs = 0;

		for (int I = 0; I < 8; I++)
			if (children & (1 << I))
				_octree.push_back(_voxels->GetNormal(posX[7 - I], posY[7 - I], posZ[7 - I]));
	} else {
		leafs = children;
		uint32_t childDescriptor = _octree.size();;
		for (unsigned i = 0; i < BitCount[children]; i++)
			_octree.push_back(0);

		for (int I = 0; I < 8; I++)
			if (children & (1 << I))
				buildOctree(posX[7 - I], posY[7 - I], posZ[7 - I], halfSize, childDescriptor++);
	}

	if (childIndex > 0x7FFF) {
		_octree[descriptorIndex] = (_farPointers.size() << 17) | 0x10000 | (children << 8) | leafs;
		_farPointers.push_back(childIndex);
	} else
		_octree[descriptorIndex] = (childIndex << 17) | (children << 8) | leafs;
}

bool VoxelOctree::raymarch(const Vec3 &o, const Vec3 &d, uint32_t &normal, float &t) {
	uint32_t rayStack[MaxScale + 1][2];

	float ox = o.x + 1.5f; //A small hack to center the volume around the origin
	float oy = o.y + 1.5f;
	float oz = o.z + 1.5f;
	float dx = d.x, dy = d.y, dz = d.z;

	if (fabsf(dx) < 1e-4) dx = 1e-4;
	if (fabsf(dy) < 1e-4) dy = 1e-4;
	if (fabsf(dz) < 1e-4) dz = 1e-4;

	float dTx = 1.0f/-fabsf(dx);
	float dTy = 1.0f/-fabsf(dy);
	float dTz = 1.0f/-fabsf(dz);

	float bTx = dTx*ox;
	float bTy = dTy*oy;
	float bTz = dTz*oz;

	uint8_t octantMask = 7;
	if (dx > 0.0f) octantMask ^= 1, bTx = 3.0f*dTx - bTx;
	if (dy > 0.0f) octantMask ^= 2, bTy = 3.0f*dTy - bTy;
	if (dz > 0.0f) octantMask ^= 4, bTz = 3.0f*dTz - bTz;

	float minT = max(2.0f*dTx - bTx, max(2.0f*dTy - bTy, 2.0f*dTz - bTz));
	float maxT = min(     dTx - bTx, min(     dTy - bTy,      dTz - bTz));
	float h = maxT;
	minT = max(minT, 0.0f);

	uint32_t current = 0;
	uint32_t parent  = 0;
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
		float maxTC = min(cornerTX, min(cornerTY, cornerTZ));

		int childShift = idx ^ octantMask;
		uint32_t childMasks = current << childShift;

		if ((childMasks & 0x8000) && minT <= maxT) {
			float maxTV = min(maxT, maxTC);
			float half = scaleExp2*0.5f;
			float centerTX = half*dTx + cornerTX;
			float centerTY = half*dTy + cornerTY;
			float centerTZ = half*dTz + cornerTZ;

			if (minT <= maxTV) {
				uint32_t childOffset = current >> 17;
				if (current & 0x10000)
					childOffset = _farPointers[childOffset];

				if (!(childMasks & 0x80)) {
					normal = _octree[childOffset + parent + BitCount[((childMasks >> (8 + childShift)) << childShift) & 127]];

					break;
				}

				if (maxTC < h) {
					rayStack[scale][0] = parent;
					rayStack[scale][1] = floatBitsToUint(maxT);
				}

				h = maxTC;

				childOffset += BitCount[childMasks & 127];
				parent      += childOffset;

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

			parent = rayStack[scale][0];
			maxT   = uintBitsToFloat(rayStack[scale][1]);

			int shX = floatBitsToUint(posX) >> scale;
			int shY = floatBitsToUint(posY) >> scale;
			int shZ = floatBitsToUint(posZ) >> scale;
			posX = uintBitsToFloat(shX << scale);
			posY = uintBitsToFloat(shY << scale);
			posZ = uintBitsToFloat(shZ << scale);
			idx = (shX & 1) | ((shY & 1) << 1) | ((shZ & 1) << 2);

			h = 0.0f;
			current = 0;
		}
	}

	if (scale >= MaxScale)
		return false;

	t = minT;
	return true;
}

void VoxelOctree::save(const char *path) {
	FILE* fp = fopen(path, "wb");

	if (fp) {
		uint32_t pointerCount = _farPointers.size();
		uint32_t dataSize = _octree.size()*sizeof(uint32_t);

		fwrite(&pointerCount, sizeof(uint32_t), 1, fp);
		fwrite(&_farPointers[0], sizeof(uint32_t), pointerCount, fp);

		fwrite(&dataSize, sizeof(uint32_t), 1, fp);
		fwrite(&_octree[0], 1, dataSize, fp);

//		uint32_t pointerCount = _farPointers.size();
//		uint32_t octreeSize   = _octree.size();
//
//		fwrite(&pointerCount, sizeof(uint32_t), 1, fp);
//		fwrite(&octreeSize,   sizeof(uint32_t), 1, fp);
//		fwrite(&_farPointers[0], sizeof(uint32_t), pointerCount, fp);
//		fwrite(&_octree[0],      sizeof(uint32_t), octreeSize,   fp);

		fclose(fp);
	}
}
