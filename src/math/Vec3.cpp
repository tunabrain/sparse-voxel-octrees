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

#include "Vec3.hpp"

Vec3 expf(const Vec3 &v) {
    return Vec3(
        expf(v.x),
        expf(v.y),
        expf(v.z)
    );
}

Vec3 powf(const Vec3 &v, float p) {
    return Vec3(
        powf(v.x, p),
        powf(v.y, p),
        powf(v.z, p)
    );
}

Vec3 operator-(const Vec3 &a) {
    return Vec3(-a.x, -a.y, -a.z);
}

Vec3 operator+(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 operator-(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 operator*(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x*b.x, a.y*b.y, a.z*b.z);
}

Vec3 operator/(const Vec3 &a, const Vec3 &b) {
    return Vec3(a.x/b.x, a.y/b.y, a.z/b.z);
}

Vec3 operator*(const Vec3 &a, float b) {
    return Vec3(a.x*b, a.y*b, a.z*b);
}

Vec3 operator*(float a, const Vec3 &b) {
    return Vec3(a*b.x, a*b.y, a*b.z);
}

Vec3 operator/(const Vec3 &a, float b) {
    return Vec3(a.x/b, a.y/b, a.z/b);
}

Vec3 operator/(float a, const Vec3 &b) {
    return Vec3(a/b.x, a/b.y, a/b.z);
}

bool operator>(const Vec3 &a, const Vec3 &b) {
    return a.x > b.x && a.y > b.y && a.z > b.z;
}

bool operator<(const Vec3 &a, const Vec3 &b) {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

bool operator>=(const Vec3 &a, const Vec3 &b) {
    return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}

bool operator<=(const Vec3 &a, const Vec3 &b) {
    return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}

bool operator==(const Vec3 &a, const Vec3 &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!=(const Vec3 &a, const Vec3 &b) {
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

std::ostream &operator<<(std::ostream &os, const Vec3 &v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}
