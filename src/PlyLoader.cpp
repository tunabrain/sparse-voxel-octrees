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
#include "third-party/ply.h"
#include "third-party/tribox3.h"
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

bool Triangle::barycentric(const Vec3 &p, float &lambda1, float &lambda2) const {
    Vec3 f1 = v1.pos - p;
    Vec3 f2 = v2.pos - p;
    Vec3 f3 = v3.pos - p;
    float area = (v1.pos - v2.pos).cross(v1.pos - v3.pos).length();
    lambda1 = f2.cross(f3).length()/area;
    lambda2 = f3.cross(f1).length()/area;

    return lambda1 >= 0.0f && lambda2 >= 0.0f && lambda1 + lambda2 <= 1.0f;
}

/* Used for sorting the right-most ends of triangles along the largest
 * dimensions for rudimentary triangle culling.
 */
static int largestDim;
static bool compareTriangles(const Triangle &a, const Triangle &b) {
	return a.upper.a[largestDim] < b.upper.a[largestDim];
}

PlyLoader::PlyLoader(const char *path) : _lower(1e30), _upper(-1e30) {
	PlyFile *file;

	openPly(path, file);
	readVertices(file);
	rescaleVertices();
	readTriangles(file);
	ply_close(file);

	vector<Vertex>().swap(_verts); /* Get rid of vertex data */
	sort(_tris.begin(), _tris.end(), &compareTriangles); /* Sort for triangle culling */
}

void PlyLoader::openPly(const char *path, PlyFile *&file) {
	int elemCount, fileType;
	char **elemNames;
	float version;
	file = ply_open_for_reading(path, &elemCount, &elemNames, &fileType, &version);

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
}

void PlyLoader::readVertices(PlyFile *file) {
	int vertCount, vertPropCount;
	PlyProperty **vertProps = ply_get_element_description(file, "vertex", &vertCount, &vertPropCount);

	_verts.reserve(vertCount);

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
}

void PlyLoader::rescaleVertices() {
	Vec3 diff = _upper - _lower;
	largestDim = 2;
	if (diff.x > diff.y && diff.x > diff.z)
		largestDim = 0;
	else if (diff.y > diff.z)
		largestDim = 1;
	float factor = 1.0/diff.a[largestDim];

	for (unsigned i = 0; i < _verts.size(); i++)
		_verts[i].pos = (_verts[i].pos - _lower)*factor;

	_upper *= factor;
	_lower *= factor;
}

void PlyLoader::readTriangles(PlyFile *file) {
	int triCount, triPropCount;
	PlyProperty **triProps = ply_get_element_description(file, "face", &triCount, &triPropCount);

	_tris.reserve(triCount);

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
}

void PlyLoader::writeTriangleCell(int x, int y, int z, float cx, float cy, float cz, const Triangle &t) {
	size_t idx = (x - _bufferX) + _bufferW*(y - _bufferY + _bufferH*(z - _bufferZ));

    float lambda1, lambda2, lambda3;
    if (!t.barycentric(Vec3(cx, cy, cz), lambda1, lambda2)) {
		lambda1 = min(max(lambda1, 0.0f), 1.0f);
		lambda2 = min(max(lambda2, 0.0f), 1.0f);
		float tau = lambda1 + lambda2;
		if (tau > 1.0f) {
			lambda1 /= tau;
			lambda2 /= tau;
		}
    }
	lambda3 = 1.0f - lambda1 - lambda2;

	_normals[idx] += (t.v1.normal*lambda1 + t.v2.normal*lambda2 + t.v3.normal*lambda3).normalize();

	/* Only store luminance - we only care about AO anyway */
    Vec3 color = t.v1.color*lambda1 + t.v2.color*lambda2 + t.v3.color*lambda3;
	_shade  [idx] += (int)color.dot(Vec3(0.2126f, 0.7152f, 0.0722f));
	_counts [idx]++;
}

void PlyLoader::triangleToVolume(const Triangle &t) {
	int lx = max((int)(t.lower.x*_sideLength), _bufferX);
	int ly = max((int)(t.lower.y*_sideLength), _bufferY);
	int lz = max((int)(t.lower.z*_sideLength), _bufferZ);
	int ux = min((int)(t.upper.x*_sideLength), _bufferX + _bufferW - 1);
	int uy = min((int)(t.upper.y*_sideLength), _bufferY + _bufferH - 1);
	int uz = min((int)(t.upper.z*_sideLength), _bufferZ + _bufferD - 1);

	if (lx > ux || ly > uy || lz > uz)
		return;

	float hx = 1.0f/_sideLength;
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

size_t PlyLoader::blockMemRequirement(int w, int h, int d) {
	size_t elementCost = sizeof(Vec3) + sizeof(uint32_t) + sizeof(uint16_t);
	return ((w*elementCost)*h)*d;
}

void PlyLoader::setupBlockProcessing(size_t elementCount, int sideLength) {
	_normals = new Vec3    [elementCount];
	_shade   = new uint32_t[elementCount];
	_counts  = new uint16_t[elementCount];

	_sideLength = sideLength;
}

void PlyLoader::processBlock(uint32_t *data, int x, int y, int z, int w, int h, int d) {
	static Triangle query;

	_bufferX = x;
	_bufferY = y;
	_bufferZ = z;
	_bufferW = w;
	_bufferH = h;
	_bufferD = d;

	size_t elemCount = _bufferW*(_bufferH*(size_t)_bufferD);
	memset(_normals, 0, elemCount*sizeof(Vec3));
	memset(_counts,  0, elemCount*sizeof(uint16_t));
	memset(_shade,   0, elemCount*sizeof(uint32_t));
	memset(data,     0, elemCount*sizeof(uint32_t));

	query.upper = Vec3(x, y, z)*1.0f/_sideLength;
	unsigned begin =
		lower_bound(_tris.begin(), _tris.end(), query, &compareTriangles) - _tris.begin();
	unsigned end = _tris.size();
	for (unsigned i = begin; i < end; i++)
		triangleToVolume(_tris[i]);

	for (size_t i = 0; i < elemCount; i++) {
		if (_counts[i])
			data[i] = compressMaterial(_normals[i], _shade[i]/(256.0f*_counts[i]));
	}
}

void PlyLoader::teardownBlockProcessing() {
	delete[] _normals;
	delete[] _shade;
	delete[] _counts;
}

void PlyLoader::suggestedDimensions(int sideLength, int &w, int &h, int &d) {
	Vec3 sizes = (_upper - _lower)*sideLength;
	w = (int)sizes.x;
	h = (int)sizes.y;
	d = (int)sizes.z;
}

void PlyLoader::convertToVolume(const char *path, int maxSize, size_t memoryBudget) {
	FILE *fp = fopen(path, "wb");
	if (!fp)
		return;

	int w, h, d;
	suggestedDimensions(maxSize, w, h, d);

	size_t sliceCost = blockMemRequirement(w, h, 1);
	int sliceZ = min((int)(memoryBudget/sliceCost), d);
	ASSERT(sliceZ != 0, "Not enough memory available for single slice");

	uint32_t *data = new uint32_t[w*(h*(size_t)sliceZ)];
	setupBlockProcessing(w*(h*(size_t)sliceZ), maxSize);

	fwrite(&w, 4, 1, fp);
	fwrite(&h, 4, 1, fp);
	fwrite(&d, 4, 1, fp);

	for (int z = 0; z < d; z += sliceZ) {
		processBlock(data, 0, 0, z, w, h, sliceZ);
		fwrite(data, sizeof(uint32_t), w*(h*(size_t)min(sliceZ, d - z)), fp);
	}
	fclose(fp);

	teardownBlockProcessing();
	delete[] data;
}
