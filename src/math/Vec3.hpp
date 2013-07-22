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

#ifndef MATH_VEC3_HPP_
#define MATH_VEC3_HPP_

#include <algorithm>
#include <math.h>
#include <ostream>

struct Vec3 {
    union {
        struct { float x, y, z; };
        float a[3];
    };

    Vec3 cross(const Vec3 &b) const {
        return Vec3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
    }

    Vec3 invert() const {
        return Vec3(1.0f/x, 1.0f/y, 1.0f/z);
    }

    float dot(const Vec3 &b) const {
        return x*b.x + y*b.y + z*b.z;
    }

    float length() const {
        return sqrtf(x*x + y*y + z*z);
    }

    Vec3 normalize() const {
        float invSqrt = 1.0f/sqrtf(x*x + y*y + z*z);

        return Vec3(x*invSqrt, y*invSqrt, z*invSqrt);
    }

    Vec3 abs() const {
        return Vec3(fabsf(x), fabsf(y), fabsf(z));
    }

    bool isNan() const {
        return isnan(x) || isnan(y) || isnan(z);
    }

    Vec3 &operator+=(const Vec3 &b) {
        x += b.x;
        y += b.y;
        z += b.z;
        return *this;
    }

    Vec3 &operator-=(const Vec3 &b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        return *this;
    }

    Vec3 &operator*=(const Vec3 &b) {
        x *= b.x;
        y *= b.y;
        z *= b.z;
        return *this;
    }

    Vec3 &operator/=(const Vec3 &b) {
        x /= b.x;
        y /= b.y;
        z /= b.z;
        return *this;
    }

    Vec3 &operator*=(float b) {
        x *= b;
        y *= b;
        z *= b;
        return *this;
    }

    Vec3 &operator/=(float b) {
        x /= b;
        y /= b;
        z /= b;
        return *this;
    }

    Vec3() : x(0.0), y(0.0), z(0.0) {}
    Vec3(float a) : x(a), y(a), z(a) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

Vec3 operator-(const Vec3 &a);
Vec3 operator+(const Vec3 &a, const Vec3 &b);
Vec3 operator-(const Vec3 &a, const Vec3 &b);
Vec3 operator*(const Vec3 &a, const Vec3 &b);
Vec3 operator/(const Vec3 &a, const Vec3 &b);
Vec3 operator*(const Vec3 &a, float b);
Vec3 operator*(float a, const Vec3 &b);
Vec3 operator/(const Vec3 &a, float b);
Vec3 operator/(float a, const Vec3 &b);
bool operator>(const Vec3 &a, const Vec3 &b);
bool operator<(const Vec3 &a, const Vec3 &b);
bool operator>=(const Vec3 &a, const Vec3 &b);
bool operator<=(const Vec3 &a, const Vec3 &b);
bool operator==(const Vec3 &a, const Vec3 &b);
bool operator!=(const Vec3 &a, const Vec3 &b);

Vec3 expf(const Vec3 &v);
Vec3 powf(const Vec3 &v, float p);

std::ostream &operator<<(std::ostream &os, const Vec3 &v);

#endif
