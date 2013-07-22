#include <stdio.h>
#include <string.h>

#include "PlyLoader.hpp"
#include "Debug.hpp"
#include "ply/ply.h"

PlyLoader::PlyLoader(const char *path) {
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
	}

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

		//free(face.elems);
	}

	ply_close(file);
}
