#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <algorithm>

#include "math/Vec3.hpp"
#include "math/Vec4.hpp"
#include "math/Mat4.hpp"
#include "math/MatrixStack.hpp"
#include "PlyLoader.hpp"
#include "Util.hpp"
#include "Events.hpp"
#include "VoxelData.hpp"
#include "VoxelOctree.hpp"

using namespace std;

static const int GWidth  = 800;
static const int GHeight = 600;
static const int NumThreads = 20;

static SDL_Surface *backBuffer;

static volatile int waitCount;
static volatile bool doTerminate;
static SDL_mutex *barrierMutex;
static SDL_sem *turnstile1, *turnstile2;

void barrierWaitPre() {
	SDL_mutexP(barrierMutex);
	waitCount++;
	if (waitCount == NumThreads)
		for (int i = 0; i < NumThreads; i++)
			SDL_SemPost(turnstile1);
	SDL_mutexV(barrierMutex);

	SDL_SemWait(turnstile1);
}

void barrierWaitPost() {
	SDL_mutexP(barrierMutex);
	waitCount--;
	if (waitCount == 0)
		for (int i = 0; i < NumThreads; i++)
			SDL_SemPost(turnstile2);
	SDL_mutexV(barrierMutex);

	SDL_SemWait(turnstile2);
}

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

struct BatchData {
	int id;
	int x0, y0;
	int x1, y1;

	VoxelOctree *tree;
};

Vec3 shade(int intNormal, const Vec3 &light) {
	Vec3 n;
	decompressNormal(intNormal, n);

	return Vec3(max(light.dot(n), 0.0f));
}

void renderBatch(BatchData *data) {
	int x0 = data->x0, y0 = data->y0;
	int x1 = data->x1, y1 = data->y1;
	VoxelOctree *tree = data->tree;

	Mat4 tform;
	MatrixStack::get(INV_MODELVIEW_STACK, tform);

	Vec3 pos = tform*Vec3();

	tform.a14 = tform.a24 = tform.a34 = 0.0f;

	uint8_t *buffer = (uint8_t *)backBuffer->pixels;
	int pitch       = backBuffer->pitch;

	float scale = 2.0f/GWidth;
	float planeDist = 1.0/tan(M_PI/6.0f);
	float zx = planeDist*tform.a13, zy = planeDist*tform.a23, zz = planeDist*tform.a33;

	Vec3 light = (tform*Vec3(1.0)).normalize();

	float dy = 1.0 - y0*scale;
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
			if (tree->raymarch(pos, dir, intNormal, t))
				col = shade(intNormal, light);

			int idx = x*4 + y*pitch;
			buffer[idx + 0] = (uint8_t)(min(col.x, 1.0f)*255.0);
			buffer[idx + 1] = (uint8_t)(min(col.y, 1.0f)*255.0);
			buffer[idx + 2] = (uint8_t)(min(col.z, 1.0f)*255.0);
			buffer[idx + 3] = 0xFF;
		}
	}
}

int renderLoop(void *threadData) {
	BatchData *data = (BatchData *)threadData;

	while (!doTerminate) {
		barrierWaitPre();
		renderBatch(data);
		barrierWaitPost();

		if (data->id == 0) {
			if (SDL_MUSTLOCK(backBuffer))
				SDL_UnlockSurface(backBuffer);

			SDL_UpdateRect(backBuffer, 0, 0, 0, 0);

			printf("Time: %fms\n", stopTimer());

			int EventType;
			while ((EventType = WaitEvent()) != SDL_MOUSEBUTTONDOWN && EventType != SDL_KEYDOWN && GetMouseDown(0) == 0);

			if (GetKeyDown(SDLK_ESCAPE)) {
				doTerminate = true;
				for (int i = 0; i < NumThreads; i++) {
					SDL_SemPost(turnstile1);
					SDL_SemPost(turnstile2);
				}
			}

			float yaw   =  GetMouseXSpeed();
			float pitch = -GetMouseYSpeed();
			if (GetMouseDown(0))
				MatrixStack::mulR(MODEL_STACK, Mat4::rotYZX(Vec3(pitch, yaw, 0.0f)));

			startTimer();

			if (SDL_MUSTLOCK(backBuffer))
				SDL_LockSurface(backBuffer);
		}
	}

	return 0;
}

void generateSampleData(const char *path, int size) {
	uint16_t *data = new uint16_t[size*size*size];

	for (int z = 0, idx = 0; z < size; z++)
		for (int y = 0; y < size; y++)
			for (int x = 0; x < size; x++, idx++)
				data[idx] = (uint16_t)(((sin(x*0.1)*sin(y*0.1)*sin(z*0.1))*0.5 + 0.5)*65535.0);

	FILE *fp = fopen(path, "wb");
	if (fp) {
		fwrite(&size, sizeof(int), 1, fp);
		fwrite(&size, sizeof(int), 1, fp);
		fwrite(&size, sizeof(int), 1, fp);
		fwrite(data, sizeof(uint16_t), size*size*size, fp);
		fclose(fp);
	}
}

int main(int argc, char *argv[]) {
	PlyLoader loader("xyzrgb_dragon.ply");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_WM_SetCaption("Sparse Voxel Octrees", "Sparse Voxel Octrees");
	backBuffer = SDL_SetVideoMode(GWidth, GHeight, 32, SDL_SWSURFACE);

	//generateSampleData("Test.voxel", 512);
	VoxelData data("Test.voxel");
	startTimer();
	VoxelOctree tree(&data);
	printf("Octree built in %fms\n", stopTimer());
	//VoxelOctree tree("HeadNew.oct");

	//return 0;

	MatrixStack::set(VIEW_STACK, Mat4::translate(Vec3(0.0, 0.0, -2.0)));

	SDL_Thread *threads[NumThreads - 1];
	BatchData threadData[NumThreads];

	barrierMutex = SDL_CreateMutex();
	turnstile1   = SDL_CreateSemaphore(0);
	turnstile2   = SDL_CreateSemaphore(0);
	waitCount = 0;
	doTerminate = false;

	if (SDL_MUSTLOCK(backBuffer))
		SDL_LockSurface(backBuffer);

	int stride = (GHeight - 1)/NumThreads + 1;
	for (int i = 0; i < NumThreads; i++) {
		threadData[i].id = i;
		threadData[i].tree = &tree;
		threadData[i].x0 = 0;
		threadData[i].x1 = GWidth;
		threadData[i].y0 = i*stride;
		threadData[i].y1 = min((i + 1)*stride, GHeight);
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
