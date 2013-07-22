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

#include "Vec4.hpp"

Vec4 expf(const Vec4 &v) {
    return Vec4(
        expf(v.x),
        expf(v.y),
        expf(v.z),
        expf(v.w)
    );
}

Vec4 powf(const Vec4 &v, float p) {
    return Vec4(
        powf(v.x, p),
        powf(v.y, p),
        powf(v.z, p),
        powf(v.w, p)
    );
}

Vec4 operator-(const Vec4 &a) {
    return Vec4(-a.x, -a.y, -a.z, -a.w);
}

Vec4 operator+(const Vec4 &a, const Vec4 &b) {
    return Vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vec4 operator-(const Vec4 &a, const Vec4 &b) {
    return Vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.z - b.z);
}

Vec4 operator*(const Vec4 &a, const Vec4 &b) {
    return Vec4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w);
}

Vec4 operator/(const Vec4 &a, const Vec4 &b) {
    return Vec4(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w);
}

Vec4 operator*(const Vec4 &a, float b) {
    return Vec4(a.x*b, a.y*b, a.z*b, a.w*b);
}

Vec4 operator*(float a, const Vec4 &b) {
    return Vec4(a*b.x, a*b.y, a*b.z, a*b.w);
}

Vec4 operator/(const Vec4 &a, float b) {
    return Vec4(a.x/b, a.y/b, a.z/b, a.w*b);
}

Vec4 operator/(float a, const Vec4 &b) {
    return Vec4(a/b.x, a/b.y, a/b.z, a/b.w);
}

bool operator>(const Vec4 &a, const Vec4 &b) {
    return a.x > b.x && a.y > b.y && a.z > b.z && a.w > b.w;
}

bool operator<(const Vec4 &a, const Vec4 &b) {
    return a.x < b.x && a.y < b.y && a.z < b.z && a.w < b.w;
}

bool operator>=(const Vec4 &a, const Vec4 &b) {
    return a.x >= b.x && a.y >= b.y && a.z >= b.z && a.w >= b.w;
}

bool operator<=(const Vec4 &a, const Vec4 &b) {
    return a.x <= b.x && a.y <= b.y && a.z <= b.z && a.w <= b.w;
}

bool operator==(const Vec4 &a, const Vec4 &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

bool operator!=(const Vec4 &a, const Vec4 &b) {
    return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
}

std::ostream &operator<<(std::ostream &os, const Vec4 &v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}
