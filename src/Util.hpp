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

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include "math/Vec3.hpp"

#include "IntTypes.hpp"

uint32 compressMaterial(const Vec3 &n, float shade);
void decompressMaterial(uint32 normal, Vec3 &dst, float &shade);

float invSqrt(float x);
void fastNormalization(Vec3 &v);

float uintBitsToFloat(uint32 i);
uint32 floatBitsToUint(float f);

int roundToPow2(int In);

//See http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
static inline int findHighestBit(uint32 v) {
    static const uint32 b[] = {
        0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000
    };
    uint32 r = (v & b[0]) != 0;
    for (int i = 4; i > 0; i--)
        r |= ((v & b[i]) != 0) << i;
    return r;
}

#endif /* UTIL_H_ */
