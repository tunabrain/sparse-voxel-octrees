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

#ifndef PLYLOADER_HPP_
#define PLYLOADER_HPP_

#include "math/Vec3.hpp"

#include "IntTypes.hpp"
#include "Timer.hpp"

#include <memory>
#include <vector>

struct PlyFile;

struct Vertex {
    Vec3 pos, normal, color;

    Vertex() {};
    Vertex(const Vec3 &p, const Vec3 &n, const Vec3 &c) :
        pos(p), normal(n), color(c) {}
};

struct Triangle {
    Vertex v1, v2, v3;
    Vec3 lower, upper;

    bool barycentric(const Vec3 &p, float &lambda1, float &lambda2) const;

    Triangle() {};
    Triangle(const Vertex &_v1, const Vertex &_v2, const Vertex &_v3);
};

class PlyLoader {
    bool _hasNormals;
    bool _isBigEndian;

    std::vector<Vertex> _verts;
    std::vector<Triangle> _tris;
    std::vector<uint32> _blockOffsets;
    std::unique_ptr<uint32[]> _blockLists;

    Timer _conversionTimer;
    int _processedBlocks;
    int _numNonZeroBlocks;
    Vec3 _lower, _upper;

    std::unique_ptr<uint8[]> _counts;
    int _sideLength;
    int _volumeW, _volumeH, _volumeD;
    int _blockW, _blockH, _blockD;
    int _subBlockW, _subBlockH, _subBlockD;
    int _partitionW, _partitionH, _partitionD;
    int _numPartitions;
    int _gridW, _gridH, _gridD;
    int _bufferX, _bufferY, _bufferZ;
    int _bufferW, _bufferH, _bufferD;

    void writeTriangleCell(uint32 *data, int x, int y, int z,
            float cx, float cy, float cz, const Triangle &t);
    void triangleToVolume(uint32 *data, const Triangle &t, int offX, int offY, int offZ);

    void openPly(const char *path, PlyFile *&file);
    void readVertices(PlyFile *file);
    void rescaleVertices();
    void readTriangles(PlyFile *file);

    void pointToGrid(const Vec3 &p, int &x, int &y, int &z);

    template<typename LoopBody>
    void iterateOverlappingBlocks(const Triangle &t, LoopBody body);

    void buildBlockLists();

public:
    PlyLoader(const char *path);

    void suggestedDimensions(int sideLength, int &w, int &h, int &d);

    size_t blockMemRequirement(int w, int h, int d);
    void setupBlockProcessing(int sideLength, int blockW, int blockH, int blockD,
            int volumeW, int volumeH, int volumeD);
    void processBlock(uint32 *data, int x, int y, int z, int w, int h, int d);
    bool isBlockEmpty(int x, int y, int z);
    void teardownBlockProcessing();

    void convertToVolume(const char *path, int maxSize, size_t memoryBudget);

    const std::vector<Triangle> &tris() const {
        return _tris;
    }
};

#endif /* OBJLOADER_HPP_ */
