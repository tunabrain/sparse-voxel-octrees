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

#include "PlyLoader.hpp"
#include "Debug.hpp"
#include "Timer.hpp"
#include "Util.hpp"

#include "thread/ThreadUtils.hpp"
#include "thread/ThreadPool.hpp"

#include "third-party/tribox3.h"
#include "third-party/ply.h"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <stdio.h>

Triangle::Triangle(const Vertex &_v1, const Vertex &_v2, const Vertex &_v3) :
    v1(_v1), v2(_v2), v3(_v3) {

    lower = Vec3(
        std::min(v1.pos.x, std::min(v2.pos.x, v3.pos.x)),
        std::min(v1.pos.y, std::min(v2.pos.y, v3.pos.y)),
        std::min(v1.pos.z, std::min(v2.pos.z, v3.pos.z))
    );
    upper = Vec3(
        std::max(v1.pos.x, std::max(v2.pos.x, v3.pos.x)),
        std::max(v1.pos.y, std::max(v2.pos.y, v3.pos.y)),
        std::max(v1.pos.z, std::max(v2.pos.z, v3.pos.z))
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

PlyLoader::PlyLoader(const char *path) : _isBigEndian(false), _lower(1e30f), _upper(-1e30f) {
    PlyFile *file;

    openPly(path, file);
    readVertices(file);
    rescaleVertices();
    readTriangles(file);
    ply_close(file);

    std::cout << "Triangle count: " << _tris.size() << ", taking up "
              << prettyPrintMemory(_tris.size()*sizeof(Triangle)) << " of memory" << std::endl;

    std::vector<Vertex>().swap(_verts); /* Get rid of vertex data */
}

void PlyLoader::openPly(const char *path, PlyFile *&file) {
    int elemCount, fileType;
    char **elemNames;
    float version;
    file = ply_open_for_reading(path, &elemCount, &elemNames, &fileType, &version);

    ASSERT(file != 0, "Failed to open PLY at %s\n", path);
    
    _isBigEndian = (file->file_type == PLY_BINARY_BE);

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

template<typename T>
T toHostOrder(T src, bool isBigEndian)
{
    if (!isBigEndian)
        return src;

    union {
        char bytes[sizeof(T)];
        T type;
    } srcBytes, dstBytes;
    
    srcBytes.type = src;
    for (size_t i = 0; i < sizeof(T); ++i)
        dstBytes.bytes[i] = srcBytes.bytes[sizeof(T) - i - 1];
    
    return dstBytes.type;
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
                vertProps[i]->offset = t*sizeof(float);
                ply_get_property(file, "vertex", vertProps[i]);
                vpAvail[t] = true;
                break;
            }
        }
    }
    
    _hasNormals = vpAvail[3] && vpAvail[4] && vpAvail[5];

    float vertData[9], elemData[9];
    float vertDefault[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 255.0f, 255.0f, 255.0f
    };
    for (int i = 0; i < vertCount; i++) {
        ply_get_element(file, (void *)elemData);

        for (int t = 0; t < 9; t++)
            vertData[t] = (vpAvail[t] ? toHostOrder(elemData[t], _isBigEndian) : vertDefault[t]);

        _verts.push_back(Vertex(
            Vec3(vertData[0], vertData[1], vertData[2]),
            Vec3(vertData[3], vertData[4], vertData[5]),
            Vec3(vertData[6], vertData[7], vertData[8])
        ));

        for (int t = 0; t < 3; t++) {
            _lower.a[t] = std::min(_lower.a[t], vertData[t]);
            _upper.a[t] = std::max(_upper.a[t], vertData[t]);
        }
    }
}

void PlyLoader::rescaleVertices() {
    Vec3 diff = _upper - _lower;
    int largestDim = 2;
    if (diff.x > diff.y && diff.x > diff.z)
        largestDim = 0;
    else if (diff.y > diff.z)
        largestDim = 1;
    float factor = 1.0f/diff.a[largestDim];

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
            triProps[i]->offset = 0;
            triProps[i]->count_offset = sizeof(int*);
            ply_get_property(file, "face", triProps[i]);
            break;
        } else if (i == triPropCount - 1)
            FAIL("No face information found\n");
    }

    struct { int *elems; int count; } face;
    for (int i = 0; i < triCount; i++) {
        ply_get_element(file, (void *)&face);

        if (!face.count)
            continue;

        int v0 = toHostOrder(face.elems[0], _isBigEndian);
        int v1 = toHostOrder(face.elems[1], _isBigEndian);
        for (int t = 2; t < face.count; t++) {
            int v2 = toHostOrder(face.elems[t], _isBigEndian);
            _tris.push_back(Triangle(_verts[v0], _verts[v1], _verts[v2]));
            if (!_hasNormals) {
                Vec3 n = (_verts[v1].pos - _verts[v0].pos).cross(_verts[v2].pos - _verts[v0].pos).normalize();
                _tris.back().v1.normal = n;
                _tris.back().v2.normal = n;
                _tris.back().v3.normal = n;
            }
            v1 = v2;
        }

        free(face.elems);
    }
}

void PlyLoader::pointToGrid(const Vec3 &p, int &x, int &y, int &z)
{
    x = (int)(p.x*(_sideLength - 2) + 1.0f);
    y = (int)(p.y*(_sideLength - 2) + 1.0f);
    z = (int)(p.z*(_sideLength - 2) + 1.0f);
}

template<typename LoopBody>
void PlyLoader::iterateOverlappingBlocks(const Triangle &t, LoopBody body)
{
    int lx, ly, lz;
    int ux, uy, uz;
    pointToGrid(t.lower, lx, ly, lz);
    pointToGrid(t.upper, ux, uy, uz);
    int lgridX = (lx + 0)/_subBlockW, lgridY = (ly + 0)/_subBlockH, lgridZ = (lz + 0)/_subBlockD;
    int ugridX = (ux + 1)/_subBlockW, ugridY = (uy + 1)/_subBlockH, ugridZ = (uz + 1)/_subBlockD;

    int maxSide = std::max(ugridX - lgridX, std::max(ugridY - lgridY, ugridZ - lgridZ));

    if (maxSide > 0) {
        float hx = _subBlockW/float(_sideLength - 2);
        float hy = _subBlockH/float(_sideLength - 2);
        float hz = _subBlockD/float(_sideLength - 2);
        float triVs[][3] = {
            {t.v1.pos.x, t.v1.pos.y, t.v1.pos.z},
            {t.v2.pos.x, t.v2.pos.y, t.v2.pos.z},
            {t.v3.pos.x, t.v3.pos.y, t.v3.pos.z}
        };
        float halfSize[] = {0.5f*hx, 0.5f*hy, 0.5f*hz};
        float center[3];

        center[2] = (lgridZ + 0.5f)*hz;
        for (int z = lgridZ; z <= ugridZ; ++z, center[2] += hz) {
            center[1] = (lgridY + 0.5f)*hy;
            for (int y = lgridY; y <= ugridY; ++y, center[1] += hy) {
                center[0] = (lgridX + 0.5f)*hx;
                for (int x = lgridX; x <= ugridX; ++x, center[0] += hx)
                    if (triBoxOverlap(center, halfSize, triVs))
                        body(x + _gridW*(y + _gridH*z));
            }
        }
    } else {
        body(lgridX + _gridW*(lgridY + _gridH*lgridZ));
    }
}

void PlyLoader::buildBlockLists()
{
    _blockOffsets.resize(_gridW*_gridH*_gridD + 1, 0);

    for (size_t i = 0; i < _tris.size(); ++i)
        iterateOverlappingBlocks(_tris[i], [&](size_t idx) { _blockOffsets[1 + idx]++; });

    for (size_t i = 1; i < _blockOffsets.size(); ++i)
        _blockOffsets[i] += _blockOffsets[i - 1];

    _blockLists.reset(new uint32[_blockOffsets.back()]);

    for (uint32 i = 0; i < _tris.size(); ++i)
        iterateOverlappingBlocks(_tris[i], [&](size_t idx) { _blockLists[_blockOffsets[idx]++] = i; });

    for (int i = int(_blockOffsets.size() - 1); i >= 1; --i)
        _blockOffsets[i] = _blockOffsets[i - 1];
    _blockOffsets[0] = 0;

    std::cout << "PlyLoader block lists take up an additional "
              << prettyPrintMemory((_blockOffsets.size() + _blockOffsets.back())*sizeof(uint32))
              << " of memory" << std::endl;
}

void PlyLoader::writeTriangleCell(uint32 *data, int x, int y, int z,
        float cx, float cy, float cz, const Triangle &t) {
    size_t idx = (x - _bufferX) + size_t(_bufferW)*(y - _bufferY + size_t(_bufferH)*(z - _bufferZ));

    float lambda1, lambda2, lambda3;
    if (!t.barycentric(Vec3(cx, cy, cz), lambda1, lambda2)) {
        lambda1 = std::min(std::max(lambda1, 0.0f), 1.0f);
        lambda2 = std::min(std::max(lambda2, 0.0f), 1.0f);
        float tau = lambda1 + lambda2;
        if (tau > 1.0f) {
            lambda1 /= tau;
            lambda2 /= tau;
        }
    }
    lambda3 = 1.0f - lambda1 - lambda2;

    Vec3 normal = (t.v1.normal*lambda1 + t.v2.normal*lambda2 + t.v3.normal*lambda3).normalize();
    Vec3 color = t.v1.color*lambda1 + t.v2.color*lambda2 + t.v3.color*lambda3;
    /* Only store luminance - we only care about AO anyway */
    float shade = color.dot(Vec3(0.2126f, 0.7152f, 0.0722f))*(1.0f/256.0f);

    if (data[idx] == 0) {
        _counts[idx] = 1;
        data[idx] = compressMaterial(normal, shade);
    } else {
        float currentRatio = _counts[idx]/(_counts[idx] + 1.0f);
        float newRatio = 1.0f - currentRatio;

        Vec3 currentNormal;
        float currentShade;
        decompressMaterial(data[idx], currentNormal, currentShade);

        Vec3 newNormal = currentNormal*currentRatio + normal*newRatio;
        float newShade = currentShade*currentRatio + shade*newRatio;
        if (newNormal.dot(newNormal) < 1e-3f)
            newNormal = currentNormal;

        data[idx] = compressMaterial(newNormal, newShade);
        _counts[idx] = std::min(int(_counts[idx]) + 1, 255);
    }
}

void PlyLoader::triangleToVolume(uint32 *data, const Triangle &t, int offX, int offY, int offZ) {
    int lx, ly, lz;
    int ux, uy, uz;
    pointToGrid(t.lower, lx, ly, lz);
    pointToGrid(t.upper, ux, uy, uz);

    lx = std::max(lx, _bufferX + offX);
    ly = std::max(ly, _bufferY + offY);
    lz = std::max(lz, _bufferZ + offZ);
    ux = std::min(ux, _bufferX + std::min(offX + _subBlockW, _bufferW) - 1);
    uy = std::min(uy, _bufferY + std::min(offY + _subBlockH, _bufferH) - 1);
    uz = std::min(uz, _bufferZ + std::min(offZ + _subBlockD, _bufferD) - 1);

    if (lx > ux || ly > uy || lz > uz)
        return;

    float hx = 1.0f/(_sideLength - 2);
    float triVs[][3] = {
        {t.v1.pos.x, t.v1.pos.y, t.v1.pos.z},
        {t.v2.pos.x, t.v2.pos.y, t.v2.pos.z},
        {t.v3.pos.x, t.v3.pos.y, t.v3.pos.z}
    };
    float halfSize[] = {0.5f*hx, 0.5f*hx, 0.5f*hx};
    float center[3];

    center[2] = (lz - 0.5f)*hx;
    for (int z = lz; z <= uz; z++, center[2] += hx) {
        center[1] = (ly - 0.5f)*hx;
        for (int y = ly; y <= uy; y++, center[1] += hx) {
            center[0] = (lx - 0.5f)*hx;
            for (int x = lx; x <= ux; x++, center[0] += hx) {
                if (triBoxOverlap(center, halfSize, triVs))
                    writeTriangleCell(data, x, y, z, center[0], center[1], center[2], t);
            }
        }
    }
}

size_t PlyLoader::blockMemRequirement(int w, int h, int d) {
    size_t elementCost = sizeof(uint8);
    return elementCost*size_t(w)*size_t(h)*size_t(d);
}

void findBestBlockPartition(int &w, int &h, int &d, int numThreads)
{
    auto maximum = [&]() -> int& {
        if (w > h && w > d) return w; else if (h > d) return h; else return d;
    };
    auto median = [&]() -> int& {
        int max = std::max(w, std::max(h, d));
        int min = std::min(w, std::min(h, d));
        if (w != min && w != max) return w;
        else if (h != min && h != max) return h;
        else return d;
    };
    auto minimum = [&]() -> int& {
        if (w < h && w < d) return w; else if (h < d) return h; else return d;
    };

    int usedThreads = 1;
    while (usedThreads < numThreads) {
             if ((maximum() % 2) == 0) maximum() /= 2;
        else if (( median() % 2) == 0)  median() /= 2;
        else if ((minimum() % 2) == 0) minimum() /= 2;
        else break;
        usedThreads *= 2;
    }
}

void PlyLoader::setupBlockProcessing(int sideLength, int blockW, int blockH, int blockD,
        int volumeW, int volumeH, int volumeD) {
    _conversionTimer.start();

    size_t elementCount = size_t(blockW)*size_t(blockH)*size_t(blockD);
    _counts.reset(new uint8[elementCount]);

    _sideLength = sideLength - 2;
    _blockW = _subBlockW = blockW;
    _blockH = _subBlockH = blockH;
    _blockD = _subBlockD = blockD;
    findBestBlockPartition(_subBlockW, _subBlockH, _subBlockD, ThreadUtils::pool->threadCount());
    _partitionW = _blockW/_subBlockW;
    _partitionH = _blockH/_subBlockH;
    _partitionD = _blockD/_subBlockD;
    _numPartitions = _partitionW*_partitionH*_partitionD;
    std::cout << "Partitioning cache block into " << _partitionW << "x" << _partitionH
              << "x" << _partitionD << " over " << _numPartitions << " threads (per thread block is "
              << _subBlockW << "x" << _subBlockH << "x" << _subBlockD << ")" << std::endl;
    _volumeW = volumeW;
    _volumeH = volumeH;
    _volumeD = volumeD;
    _gridW = _partitionW*(_volumeW + _blockW - 1)/_blockW;
    _gridH = _partitionH*(_volumeH + _blockH - 1)/_blockH;
    _gridD = _partitionD*(_volumeD + _blockD - 1)/_blockD;

    _processedBlocks = 0;
    _numNonZeroBlocks = 0;

    buildBlockLists();
}

void PlyLoader::processBlock(uint32 *data, int x, int y, int z, int w, int h, int d) {
    _bufferX = x;
    _bufferY = y;
    _bufferZ = z;
    _bufferW = w;
    _bufferH = h;
    _bufferD = d;

    ThreadUtils::pool->enqueue([&](uint32 i, uint32, uint32){
        int px = i % _partitionW;
        int py = (i/_partitionW) % _partitionH;
        int pz = i/(_partitionW*_partitionH);

        int blockIdx = (x/_subBlockW + px) + _gridW*((y/_subBlockH + py) + _gridH*(z/_subBlockD + pz));
        int start = _blockOffsets[blockIdx];
        int end   = _blockOffsets[blockIdx + 1];

        for (int i = start; i < end; ++i)
            triangleToVolume(data, _tris[_blockLists[i]], px*_subBlockW, py*_subBlockH, pz*_subBlockD);
    }, _numPartitions)->wait();

    _processedBlocks++;

    _conversionTimer.stop();
    double elapsed = _conversionTimer.elapsed();

    std::cout << "Processed block " << _processedBlocks << "/" << _numNonZeroBlocks
              << " (" << (_processedBlocks*100)/_numNonZeroBlocks << "%) after "
              << int(elapsed) << " seconds. ";
    if (_processedBlocks < _numNonZeroBlocks)
        std::cout << "Approximate time to finish: " << int((_numNonZeroBlocks - _processedBlocks)*elapsed/_processedBlocks)
                  << " seconds.";
    else
        std::cout << "All blocks processed! Post processing...";
    std::cout << std::endl;
}

bool PlyLoader::isBlockEmpty(int x, int y, int z)
{
    for (int pz = 0; pz < _partitionD; ++pz) {
        for (int py = 0; py < _partitionH; ++py) {
            for (int px = 0; px < _partitionW; ++px) {
                int blockIdx = (x/_subBlockW + px) + _gridW*((y/_subBlockH + py) + _gridH*(z/_subBlockD + pz));
                int start = _blockOffsets[blockIdx];
                int end   = _blockOffsets[blockIdx + 1];

                if (start != end) {
                    _numNonZeroBlocks++;
                    return false;
                }
            }
        }
    }
    return true;
}

void PlyLoader::teardownBlockProcessing() {
    _counts.reset();
}

void PlyLoader::suggestedDimensions(int sideLength, int &w, int &h, int &d) {
    Vec3 sizes = (_upper - _lower)*float(sideLength - 2);
    w = int(sizes.x) + 2;
    h = int(sizes.y) + 2;
    d = int(sizes.z) + 2;
}

void PlyLoader::convertToVolume(const char *path, int maxSize, size_t memoryBudget) {
    FILE *fp = fopen(path, "wb");
    if (!fp)
        return;

    int w, h, d;
    suggestedDimensions(maxSize, w, h, d);

    size_t sliceCost = blockMemRequirement(w, h, 1);
    int sliceZ = std::min((int)(memoryBudget/sliceCost), d);
    ASSERT(sliceZ != 0, "Not enough memory available for single slice");

    uint32 *data = new uint32[size_t(w)*size_t(h)*size_t(sliceZ)];
    setupBlockProcessing(maxSize, w, h, sliceZ, w, h, d);

    fwrite(&w, 4, 1, fp);
    fwrite(&h, 4, 1, fp);
    fwrite(&d, 4, 1, fp);

    for (int z = 0; z < d; z += sliceZ) {
        processBlock(data, 0, 0, z, w, h, sliceZ);

        fwrite(data, sizeof(uint32), size_t(w)*size_t(h)*size_t(std::min(sliceZ, d - z)), fp);
    }
    fclose(fp);

    teardownBlockProcessing();
    delete[] data;
}
