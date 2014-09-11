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

#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <algorithm>

#include "math/MatrixStack.hpp"
#include "ThreadBarrier.hpp"
#include "VoxelOctree.hpp"
#include "math/Vec3.hpp"
#include "math/Vec4.hpp"
#include "math/Mat4.hpp"
#include "PlyLoader.hpp"
#include "VoxelData.hpp"
#include "Events.hpp"
#include "Util.hpp"

using namespace std;

/* Number of threads to use - adapt this to your platform for optimal results */
static const int NumThreads = 16;
/* Screen resolution */
static const int GWidth  = 1280;
static const int GHeight = 720;

static const float AspectRatio = GHeight/(float)GWidth;
static const int TileSize = 8;

static SDL_Surface *backBuffer;
static ThreadBarrier *barrier;

static volatile bool doTerminate;

struct BatchData {
    int id;
    int x0, y0;
    int x1, y1;

    int tilesX, tilesY;
    float *depthBuffer;
    VoxelOctree *tree;
};

Vec3 shade(int intNormal, const Vec3 &ray, const Vec3 &light) {
    Vec3 n;
    float c;
    decompressMaterial(intNormal, n, c);

    float d = max(light.dot(ray.reflect(n)), 0.0f);
    float specular = d*d;

    return c*0.9f*fabs(light.dot(n)) + specular*0.2f;
}

void renderTile(int x0, int y0, int x1, int y1, float scale, float zx, float zy, float zz, const Mat4 &tform, const Vec3 &light, VoxelOctree *tree, const Vec3 &pos, float minT) {
    uint8_t *buffer = (uint8_t *)backBuffer->pixels;
    int pitch       = backBuffer->pitch;

    float dy = AspectRatio - y0*scale;
    for (int y = y0; y < y1; y++, dy -= scale) {
        float dx = -1.0 + x0*scale;
        for (int x = x0; x < x1; x++, dx += scale) {
            Vec3 dir = Vec3(
                dx*tform.a11 + dy*tform.a12 + zx,
                dx*tform.a21 + dy*tform.a22 + zy,
                dx*tform.a31 + dy*tform.a32 + zz
            );
            dir *= invSqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);

            uint32_t intNormal;
            float t;
            Vec3 col;
            if (tree->raymarch(pos + dir*minT, dir, 0.0f, intNormal, t))
                col = shade(intNormal, dir, light);

            int idx = x*4 + y*pitch;
            buffer[idx + 0] = (uint8_t)(min(col.x, 1.0f)*255.0);
            buffer[idx + 1] = (uint8_t)(min(col.y, 1.0f)*255.0);
            buffer[idx + 2] = (uint8_t)(min(col.z, 1.0f)*255.0);
            buffer[idx + 3] = 0xFF;
        }
    }
}

void renderBatch(BatchData *data) {
    const float TreeMiss = 1e10;

    int x0 = data->x0, y0 = data->y0;
    int x1 = data->x1, y1 = data->y1;
    int tilesX = data->tilesX;
    int tilesY = data->tilesY;
    float *depthBuffer = data->depthBuffer;
    VoxelOctree *tree = data->tree;

    Mat4 tform;
    MatrixStack::get(INV_MODELVIEW_STACK, tform);

    Vec3 pos = tform*Vec3() + tree->center() + Vec3(1.0);

    tform.a14 = tform.a24 = tform.a34 = 0.0f;

    float scale = 2.0f/GWidth;
    float tileScale = TileSize*scale;
    float planeDist = 1.0/tan(M_PI/6.0f);
    float zx = planeDist*tform.a13, zy = planeDist*tform.a23, zz = planeDist*tform.a33;
    float coarseScale = 2.0*TileSize/(planeDist*GHeight);

    Vec3 light = (tform*Vec3(-1.0, 1.0, -1.0)).normalize();

    memset((uint8_t *)backBuffer->pixels + y0*backBuffer->pitch, 0, (y1 - y0)*backBuffer->pitch);

    float dy = AspectRatio - y0*scale;
    for (int y = 0, idx = 0; y < tilesY; y++, dy -= tileScale) {
        float dx = -1.0 + x0*scale;
        for (int x = 0; x < tilesX; x++, dx += tileScale, idx++) {
            Vec3 dir = Vec3(
                dx*tform.a11 + dy*tform.a12 + zx,
                dx*tform.a21 + dy*tform.a22 + zy,
                dx*tform.a31 + dy*tform.a32 + zz
            );
            dir *= invSqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);

            uint32_t intNormal;
            float t;
            Vec3 col;
            if (tree->raymarch(pos, dir, coarseScale, intNormal, t))
                depthBuffer[idx] = t;
            else
                depthBuffer[idx] = TreeMiss;

            if (x > 0 && y > 0) {
                float minT = min(min(depthBuffer[idx], depthBuffer[idx - 1]),
                    min(depthBuffer[idx - tilesX],
                    depthBuffer[idx - tilesX - 1]));

                if (minT != TreeMiss) {
                    int tx0 = (x - 1)*TileSize + x0;
                    int ty0 = (y - 1)*TileSize + y0;
                    int tx1 = min(tx0 + TileSize, x1);
                    int ty1 = min(ty0 + TileSize, y1);
                    renderTile(tx0, ty0, tx1, ty1, scale, zx, zy, zz, tform, light, tree, pos, max(minT - 0.03f, 0.0f));
                }
            }
        }
    }
}

int renderLoop(void *threadData) {
    BatchData *data = (BatchData *)threadData;

    while (!doTerminate) {
        barrier->waitPre();
        renderBatch(data);
        barrier->waitPost();

        if (data->id == 0) {
            if (SDL_MUSTLOCK(backBuffer))
                SDL_UnlockSurface(backBuffer);

            SDL_UpdateRect(backBuffer, 0, 0, 0, 0);

            int event;
            while ((event = WaitEvent()) != SDL_MOUSEBUTTONDOWN && event != SDL_KEYDOWN && !GetMouseDown(0) && !GetMouseDown(1));

            if (GetKeyDown(SDLK_ESCAPE)) {
                doTerminate = true;
                barrier->releaseAll();
            }

            float mx = GetMouseXSpeed();
            float my = GetMouseYSpeed();
            if (GetMouseDown(0) && (mx != 0 || my != 0)) {
                Mat4 tform;
                MatrixStack::get(INV_MODELVIEW_STACK, tform);
                Vec4 x = tform*Vec4(1.0, 0.0, 0.0, 0.0);
                Vec4 y = tform*Vec4(0.0, -1.0, 0.0, 0.0);
                Vec3 axis(mx*y.x - my*x.x, mx*y.y - my*x.y, mx*y.z - my*x.z);
                MatrixStack::mulR(MODEL_STACK, Mat4::rotAxis(axis.normalize(), sqrtf(mx*mx + my*my)));
            } else if (GetMouseDown(1))
                MatrixStack::mulR(VIEW_STACK, Mat4::scale(Vec3(1.0f, 1.0f, 1.0f + my*0.01f)));

            if (SDL_MUSTLOCK(backBuffer))
                SDL_LockSurface(backBuffer);
        }
    }

    return 0;
}

/* Set this to 1 to generate a voxel octree from a PLY mesh in-memory */
#define GENERATE_IN_MEMORY 0
/* Set this to 1 to do the same, only writing out a temporary full-sized voxel
 * file to disk and then converting that to an octree. Much, much slower than
 * the method above, but it demonstrates the capability of converting full voxel
 * data as well (i.e. CT scans)
 */
#define GENERATE_ON_DISK   0

/* Maximum allowed memory allocation sizes for lookup table and cache blocks.
 * Larger => faster conversion usually, but adapt this to your own RAM size.
 * The conversion will still succeed with memory sizes much, much smaller than
 * the size of the voxel data, only slower.
 */
static const size_t  lutMemory = 512*1024*1024;
static const size_t dataMemory = 512*1024*1024;

VoxelOctree *initScene() {
#if GENERATE_IN_MEMORY
    PlyLoader *loader = new PlyLoader("models/xyzrgb_dragon.ply");
    VoxelData *data = new VoxelData(loader, 256, lutMemory, dataMemory);
    VoxelOctree *tree = new VoxelOctree(data);
    delete data;
    delete loader;
    tree->save("models/XYZRGB-Dragon.oct");
#elif GENERATE_ON_DISK
    PlyLoader *loader = new PlyLoader("models/xyzrgb_dragon.ply");
    loader->convertToVolume("models/XYZRGB-Dragon.voxel", 256, lutMemory + dataMemory);
    VoxelData *data = new VoxelData("models/XYZRGB-Dragon.voxel", lutMemory, dataMemory);
    VoxelOctree *tree = new VoxelOctree(data);
    delete data;
    delete loader;
    tree->save("models/XYZRGB-Dragon.oct");
#else
    VoxelOctree *tree = new VoxelOctree("models/XYZRGB-Dragon.oct");
#endif

    return tree;
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("Sparse Voxel Octrees", "Sparse Voxel Octrees");
    backBuffer = SDL_SetVideoMode(GWidth, GHeight, 32, SDL_SWSURFACE);

    VoxelOctree *tree = initScene();

    MatrixStack::set(VIEW_STACK, Mat4::translate(Vec3(0.0, 0.0, -2.0)));

    SDL_Thread *threads[NumThreads - 1];
    BatchData threadData[NumThreads];

    barrier = new ThreadBarrier(NumThreads);
    doTerminate = false;

    if (SDL_MUSTLOCK(backBuffer))
        SDL_LockSurface(backBuffer);

    int stride = (GHeight - 1)/NumThreads + 1;
    for (int i = 0; i < NumThreads; i++) {
        threadData[i].id = i;
        threadData[i].tree = tree;
        threadData[i].x0 = 0;
        threadData[i].x1 = GWidth;
        threadData[i].y0 = i*stride;
        threadData[i].y1 = min((i + 1)*stride, GHeight);
        threadData[i].tilesX = (threadData[i].x1 - threadData[i].x0 - 1)/TileSize + 2;
        threadData[i].tilesY = (threadData[i].y1 - threadData[i].y0 - 1)/TileSize + 2;
        threadData[i].depthBuffer = new float[threadData[i].tilesX*threadData[i].tilesY];
    }

    for (int i = 1; i < NumThreads; i++)
        threads[i - 1] = SDL_CreateThread(&renderLoop, (void *)&threadData[i]);

    renderLoop((void *)&threadData[0]);

    for (int i = 1; i < NumThreads; i++)
        SDL_WaitThread(threads[i - 1], 0);

    if (SDL_MUSTLOCK(backBuffer))
        SDL_UnlockSurface(backBuffer);

    SDL_Quit();

    return 0;
}
