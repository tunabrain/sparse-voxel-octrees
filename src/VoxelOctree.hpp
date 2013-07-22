#ifndef VOXELOCTREE_HPP_
#define VOXELOCTREE_HPP_

#include <vector>
#include <stdint.h>

#include "math/Vec3.hpp"

class VoxelData;

class VoxelOctree {
	static const int32_t MaxScale = 23;

	std::vector<uint32_t> _octree;
	std::vector<uint32_t> _farPointers;

	VoxelData *_voxels;

	void buildOctree(int x, int y, int z, int size, uint32_t descriptorIndex);

public:
	void save(const char *path);
	bool raymarch(const Vec3 &o, const Vec3 &d, uint32_t &normal, float &t);

	VoxelOctree(const char *path);
	VoxelOctree(VoxelData *voxels);
};

#endif /* VOXELOCTREE_HPP_ */
