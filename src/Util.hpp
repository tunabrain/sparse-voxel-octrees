#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <stdint.h>
#include "math/Vec3.hpp"

uint32_t compressNormal(const Vec3 &n);
void decompressNormal(uint32_t normal, Vec3 &dst);

float invSqrt(float x);
void fastNormalization(Vec3 &v);

float uintBitsToFloat(uint32_t i);
uint32_t floatBitsToUint(float f);

int roundToPow2(int In);
int findHighestBit(uint32_t v);

#endif /* UTIL_H_ */
