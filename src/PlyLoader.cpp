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

#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "PlyLoader.hpp"
#include "Debug.hpp"
#include "ply/ply.h"
#include "tribox3.h"
#include "Util.hpp"

using namespace std;

Triangle::Triangle(const Vertex &_v1, const Vertex &_v2, const Vertex &_v3) :
	v1(_v1), v2(_v2), v3(_v3) {

	lower = Vec3(
		min(v1.pos.x, min(v2.pos.x, v3.pos.x)),
		min(v1.pos.y, min(v2.pos.y, v3.pos.y)),
		min(v1.pos.z, min(v2.pos.z, v3.pos.z))
	);
	upper = Vec3(
		max(v1.pos.x, max(v2.pos.x, v3.pos.x)),
		max(v1.pos.y, max(v2.pos.y, v3.pos.y)),
		max(v1.pos.z, max(v2.pos.z, v3.pos.z))
	);
}

PlyLoader::PlyLoader(const char *path) : _lower(1e30), _upper(-1e30) {
	int elemCount, fileType;
	char **elemNames;
	float version;
	PlyFile *file = ply_open_for_reading((char *)path, &elemCount, &elemNames, &fileType, &version);

	ASSERT(file != 0, "Failed to open PLY at %s\n", path);

	bool hasVerts = false, hasTris = false;
	for (int i = 0; i < elemCount; i++) {
		if (!strcmp(elemNames[i], "vertex"))
			hasVerts = true;
		else if (!strcmp(elemNames[i], "face"))
			hasTris = true;
		else
			DBG("PLY loader", WARN, "Ignoring unknown element %s\n", elemNames[i]);
	}

	ASSERT(hasVerts && hasTris, "PLY file has to have triangles and vertices\n");

	int vertCount, triCount, vertPropCount, triPropCount;
	PlyProperty **vertProps = ply_get_element_description(file, "vertex", &vertCount, &vertPropCount);
	PlyProperty ** triProps = ply_get_element_description(file, "face",    &triCount,  &triPropCount);

	printf("%d triangles, %d vertices\n", triCount, vertCount);

	_verts.reserve(vertCount);
	_tris.reserve(triCount);

	const char *vpNames[] = {"x", "y", "z", "nx", "ny", "nz", "red", "green", "blue"};
	bool vpAvail[9] = {false};
	for (int i = 0; i < vertPropCount; i++) {
		for (int t = 0; t < 9; t++) {
			if (!strcmp(vertProps[i]->name, vpNames[t])) {
				vertProps[i]->internal_type = PLY_FLOAT;
				vertProps[i]->offset = i*sizeof(float);
				ply_get_property(file, "vertex", vertProps[i]);
				vpAvail[t] = true;
				break;
			}
		}
	}

	float vertData[9], elemData[9];
	float vertDefault[] = {
		0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f
	};
	for (int i = 0; i < vertCount; i++) {
		ply_get_element(file, (void *)elemData);

		for (int t = 0; t < 9; t++)
			vertData[t] = (vpAvail[t] ? elemData[t] : vertDefault[t]);

		_verts.push_back(Vertex(
			Vec3(vertData[0], vertData[1], vertData[2]),
			Vec3(vertData[3], vertData[4], vertData[5]),
			Vec3(vertData[6], vertData[7], vertData[8])
		));

		for (int t = 0; t < 3; t++) {
			_lower.a[t] = min(_lower.a[t], vertData[t]);
			_upper.a[t] = max(_upper.a[t], vertData[t]);
		}
	}

	Vec3 diff = _upper - _lower;
	float factor = 1.0/max(diff.x, max(diff.y, diff.z));

	for (int i = 0; i < vertCount; i++)
		_verts[i].pos = (_verts[i].pos - _lower)*factor;

	_upper *= factor;
	_lower *= factor;

	for (int i = 0; i < triPropCount; i++) {
		if (!strcmp(triProps[i]->name, "vertex_indices")) {
			triProps[i]->count_internal = PLY_INT;
			triProps[i]->internal_type = PLY_INT;
			triProps[i]->offset = sizeof(int);
			triProps[i]->count_offset = 0;
			ply_get_property(file, "face", triProps[i]);
			break;
		} else if (i == triPropCount - 1)
			FAIL("No face information found\n");
	}

	struct { int count; int *elems; } face;
	for (int i = 0; i < triCount; i++) {
		ply_get_element(file, (void *)&face);

		if (!face.count)
			continue;

		int v0 = face.elems[0], v1 = face.elems[1];
		for (int i = 2; i < face.count; i++) {
			_tris.push_back(Triangle(_verts[v0], _verts[v1], _verts[face.elems[i]]));
			v1 = face.elems[i];
		}

		free(face.elems);
	}

	vector<Vertex>().swap(_verts); /* Clear out vertex data */

	ply_close(file);
}

void PlyLoader::writeTriangleCell(int x, int y, int z, float cx, float cy, float cz, const Triangle &t) {
	size_t idx = (x - _bufferX) + _bufferW*(y - _bufferY + _bufferH*(z - _bufferZ));

	/* TODO: Interpolate within triangle */
	_normals[idx] += t.v1.normal;
	_shade  [idx] += (int)t.v1.color.dot(Vec3(0.2126f, 0.7152f, 0.0722f));
	_counts [idx]++;
}

void PlyLoader::triangleToVolume(const Triangle &t, int size) {
	int lx = max((int)(t.lower.x*size), _bufferX);
	int ly = max((int)(t.lower.y*size), _bufferY);
	int lz = max((int)(t.lower.z*size), _bufferZ);
	int ux = min((int)(t.upper.x*size), _bufferX + _bufferW - 1);
	int uy = min((int)(t.upper.y*size), _bufferY + _bufferH - 1);
	int uz = min((int)(t.upper.z*size), _bufferZ + _bufferD - 1);

	if (lx > ux || ly > uy || lz > uz)
		return;

	float hx = 1.0f/size;
	float triVs[][3] = {
		{t.v1.pos.x, t.v1.pos.y, t.v1.pos.z},
		{t.v2.pos.x, t.v2.pos.y, t.v2.pos.z},
		{t.v3.pos.x, t.v3.pos.y, t.v3.pos.z}
	};
	float halfSize[] = {0.5f*hx, 0.5f*hx, 0.5f*hx};
	float center[3];

	center[2] = (lz + 0.5f)*hx;
	for (int z = lz; z <= uz; z++, center[2] += hx) {
		center[1] = (ly + 0.5f)*hx;
		for (int y = ly; y <= uy; y++, center[1] += hx) {
			center[0] = (lx + 0.5)*hx;
			for (int x = lx; x <= ux; x++, center[0] += hx) {
				if (triBoxOverlap(center, halfSize, triVs))
					writeTriangleCell(x, y, z, center[0], center[1], center[2], t);
			}
		}
	}
}

void PlyLoader::convertToVolume(const char *path, int maxSize, size_t memoryBudget) {
	FILE *fp = fopen(path, "wb");
	if (!fp)
		return;

	Vec3 sizes = (_upper - _lower)*maxSize;
	int sizeX = (int)sizes.x;
	int sizeY = (int)sizes.y;
	int sizeZ = (int)sizes.z;

	printf("Sizes: %d %d %d\n", sizeX, sizeY, sizeZ);

	size_t elementCost = sizeof(Vec3) + sizeof(uint32_t)*2 + sizeof(uint16_t);
	int sliceZ = memoryBudget/(sizeX*sizeY*elementCost);
	sliceZ = min(sliceZ, sizeZ);

	ASSERT(sliceZ != 0, "Not enough memory available for single slice");
	printf("Slice size: %d  Passes: %d\n", sliceZ, (sizeZ - 1)/sliceZ + 1);

	uint32_t *data;
	_normals = new Vec3    [sizeX*sizeY*sliceZ];
	_shade   = new uint32_t[sizeX*sizeY*sliceZ];
	data     = new uint32_t[sizeX*sizeY*sliceZ];
	_counts  = new uint16_t[sizeX*sizeY*sliceZ];

	_bufferX = _bufferY = _bufferZ = 0;
	_bufferW = sizeX;
	_bufferH = sizeY;
	_bufferD = sliceZ;

	fwrite(&sizeX, 4, 1, fp);
	fwrite(&sizeY, 4, 1, fp);
	fwrite(&sizeZ, 4, 1, fp);

	for (_bufferZ = 0; _bufferZ < sizeZ; _bufferZ += sliceZ) {
		memset(_normals, 0, sizeX*sizeY*sliceZ*sizeof(Vec3));
		memset(_shade,   0, sizeX*sizeY*sliceZ*sizeof(uint32_t));
		memset(data,     0, sizeX*sizeY*sliceZ*sizeof(uint32_t));
		memset(_counts,  0, sizeX*sizeY*sliceZ*sizeof(uint16_t));

		for (unsigned i = 0; i < _tris.size(); i++)
			triangleToVolume(_tris[i], maxSize);

		for (int i = 0; i < sizeX*sizeY*sliceZ; i++) {
			if (_counts[i])
				data[i] = compressMaterial(_normals[i], _shade[i]/(256.0f*_counts[i]));
		}

		fwrite(data, sizeof(uint32_t), sizeX*sizeY*min(sliceZ, sizeZ - _bufferZ), fp);
	}

	fclose(fp);

	delete[] _normals;
	delete[] _shade;
	delete[] data;
	delete[] _counts;
}
