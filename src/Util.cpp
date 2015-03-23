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

#include "Util.hpp"

#include <sstream>

std::string prettyPrintMemory(uint64 size)
{
    const char *unit;
    uint64 base;
    if (size < 1024) {
        base = 1;
        unit = " bytes";
    } else if (size < 1024*1024) {
        base = 1024;
        unit = " KB";
    } else if (size < uint64(1024)*1024*1024) {
        base = 1024*1024;
        unit = " MB";
    } else {
        base = uint64(1024)*1024*1024;
        unit = " GB";
    }

    std::ostringstream out;

    if (size/base < 10)
        out << ((size*100)/base)*0.01;
    else if (size/base < 100)
        out << ((size*10)/base)*0.1;
    else
        out << size/base;
    out << unit;

    return out.str();
}

uint32 compressMaterial(const Vec3 &n, float shade) {
    const int32 uScale = (1 << 11) - 1;
    const int32 vScale = (1 << 11) - 1;

    uint32 face = 0;
    float dominantDir = fabsf(n.x);
    if (fabsf(n.y) > dominantDir) dominantDir = fabsf(n.y), face = 1;
    if (fabsf(n.z) > dominantDir) dominantDir = fabsf(n.z), face = 2;

    uint32 sign = n.a[face] < 0.0f;

    const int mod3[] = {0, 1, 2, 0, 1};
    float n1 = n.a[mod3[face + 1]]/dominantDir;
    float n2 = n.a[mod3[face + 2]]/dominantDir;

    uint32 u = std::min((int32)((n1*0.5f + 0.5f)*uScale), 0x7FF);
    uint32 v = std::min((int32)((n2*0.5f + 0.5f)*vScale), 0x7FF);
    uint32 c = std::min((int32)(shade*127.0f), 0x7F);

    return (sign << 31) | (face << 29) | (u << 18) | v << 7 | c;
}

void decompressMaterial(uint32 normal, Vec3 &dst, float &shade) {
    uint32 sign = (normal & 0x80000000) >> 31;
    uint32 face = (normal & 0x60000000) >> 29;
    uint32 u    = (normal & 0x1FFC0000) >> 18;
    uint32 v    = (normal & 0x0003FF80) >> 7;
    uint32 c    = (normal & 0x0000007F);

    const int mod3[] = {0, 1, 2, 0, 1};
    dst.a[     face     ] = (sign ? -1.0f : 1.0f);
    dst.a[mod3[face + 1]] = u*4.8852e-4f*2.0f - 1.0f;
    dst.a[mod3[face + 2]] = v*4.8852e-4f*2.0f - 1.0f;

    fastNormalization(dst);
    shade = c*1.0f/127.0f;
}

float invSqrt(float x) { //Inverse square root as used in Quake III
    float x2 = x*0.5f;
    float y  = x;

    uint32 i = floatBitsToUint(y);
    i = 0x5f3759df - (i >> 1);

    y = uintBitsToFloat(i);
    y = y*(1.5f - x2*y*y);

    return y;
}

void fastNormalization(Vec3 &v) {
    v *= invSqrt(v.dot(v));
}

float uintBitsToFloat(uint32 i) {
    union { uint32 i; float f; } unionHack;
    unionHack.i = i;
    return unionHack.f;
}

uint32 floatBitsToUint(float f) {
    union { uint32 i; float f; } unionHack;
    unionHack.f = f;
    return unionHack.i;
}

int roundToPow2(int x) {
    int y;
    for (y = 1; y < x; y *= 2);
    return y;
}

