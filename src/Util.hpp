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

#include <stdint.h>
#include "math/Vec3.hpp"

uint32_t compressMaterial(const Vec3 &n, float shade);
void decompressMaterial(uint32_t normal, Vec3 &dst, float &shade);

float invSqrt(float x);
void fastNormalization(Vec3 &v);

float uintBitsToFloat(uint32_t i);
uint32_t floatBitsToUint(float f);

int roundToPow2(int In);
int findHighestBit(uint32_t v);

#endif /* UTIL_H_ */
