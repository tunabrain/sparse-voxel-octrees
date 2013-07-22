/*
 * Util.cpp
 *
 *  Created on: 09.01.2011
 *      Author: Noobody
 */

#include "Util.hpp"

uint32_t compressNormal(const Vec3 &n) {
	const int32_t uScale = (1 << 15) - 1;
	const int32_t vScale = (1 << 14) - 1;

	uint32_t face = 0;
	float dominantDir = fabsf(n.x);
	if (fabsf(n.y) > dominantDir) dominantDir = fabsf(n.y), face = 1;
	if (fabsf(n.z) > dominantDir) dominantDir = fabsf(n.z), face = 2;

	uint32_t sign = n.a[face] < 0.0f;

	const int mod3[] = {0, 1, 2, 0, 1};
	float n1 = n.a[mod3[face + 1]]/dominantDir;
	float n2 = n.a[mod3[face + 2]]/dominantDir;

	uint32_t u = (int32_t)((n1*0.5 + 0.5)*uScale);
	uint32_t v = (int32_t)((n2*0.5 + 0.5)*vScale);

	return (sign << 31) | (face << 29) | (u << 14) | v;
}

void decompressNormal(uint32_t normal, Vec3 &dst) {
	uint32_t sign = (normal & 0x80000000) >> 31;
	uint32_t face = (normal & 0x60000000) >> 29;
	uint32_t u    = (normal & 0x1FFFC000) >> 14;
	uint32_t v    = (normal & 0x00003FFF);

	const int mod3[] = {0, 1, 2, 0, 1};
	dst.a[     face     ] = (sign ? -1.0 : 1.0);
	dst.a[mod3[face + 1]] = u*3.0519e-5*2.0f - 1.0f;
	dst.a[mod3[face + 2]] = v*6.1037e-5*2.0f - 1.0f;

	fastNormalization(dst);
}

float invSqrt(float x) { //Inverse square root as used in Quake III
	float x2 = x*0.5f;
	float y  = x;

	uint32_t i = floatBitsToUint(y);
	i = 0x5f3759df - (i >> 1);

	y = uintBitsToFloat(i);
	y = y*(1.5f - x2*y*y);

	return y;
}

void fastNormalization(Vec3 &v) {
	v *= invSqrt(v.dot(v));
}

float uintBitsToFloat(uint32_t i) {
	union { uint32_t i; float f; } unionHack;
	unionHack.i = i;
	return unionHack.f;
}

uint32_t floatBitsToUint(float f) {
	union { uint32_t i; float f; } unionHack;
	unionHack.f = f;
	return unionHack.i;
}

int roundToPow2(int x) {
	int y;
	for (y = 1; y < x; y *= 2);
	return y;
}

//See http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
int findHighestBit(uint32_t v) {
	static const uint32_t b[] = {
		0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000
	};
	uint32_t r = (v & b[0]) != 0;
	for (int i = 4; i > 0; i--)
		r |= ((v & b[i]) != 0) << i;
	return r;
}
