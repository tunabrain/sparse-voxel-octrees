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

#ifndef MATH_VEC4_HPP_
#define MATH_VEC4_HPP_

#include <algorithm>
#include <math.h>
#include <ostream>

#include "Vec3.hpp"

struct Vec4 {
    union {
        struct { float x, y, z, w; };
        float a[4];
    };

    Vec4 invert() const {
        return Vec4(1.0f/x, 1.0f/y, 1.0f/z, 1.0f/w);
    }

    float dot(const Vec4 &b) const {
        return x*b.x + y*b.y + z*b.z + w*b.w;
    }

    float length() const {
        return sqrtf(x*x + y*y + z*z + w*w);
    }

    Vec4 normalize() const {
        float invSqrt = 1.0f/sqrtf(x*x + y*y + z*z + w*w);

        return Vec4(x*invSqrt, y*invSqrt, z*invSqrt, w*invSqrt);
    }

    Vec4 abs() const {
        return Vec4(fabs(x), fabs(y), fabs(z), fabs(w));
    }

    Vec4 &operator+=(const Vec4 &b) {
        x += b.x;
        y += b.y;
        z += b.z;
        w += b.w;
        return *this;
    }

    Vec4 &operator-=(const Vec4 &b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        w -= b.w;
        return *this;
    }

    Vec4 &operator*=(const Vec4 &b) {
        x *= b.x;
        y *= b.y;
        z *= b.z;
        w *= b.w;
        return *this;
    }

    Vec4 &operator/=(const Vec4 &b) {
        x /= b.x;
        y /= b.y;
        z /= b.z;
        w /= b.w;
        return *this;
    }

    Vec4 &operator*=(float b) {
        x *= b;
        y *= b;
        z *= b;
        w *= b;
        return *this;
    }

    Vec4 &operator/=(float b) {
        x /= b;
        y /= b;
        z /= b;
        w *= b;
        return *this;
    }

    Vec4() : x(0.0), y(0.0), z(0.0), w(0.0) {}
    Vec4(float s) : x(s), y(s), z(s), w(s) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    Vec4(const Vec3 &b, float _w = 1.0) : x(b.x), y(b.y), z(b.z), w(_w) {}
};

Vec4 operator-(const Vec4 &a);
Vec4 operator+(const Vec4 &a, const Vec4 &b);
Vec4 operator-(const Vec4 &a, const Vec4 &b);
Vec4 operator*(const Vec4 &a, const Vec4 &b);
Vec4 operator/(const Vec4 &a, const Vec4 &b);
Vec4 operator*(const Vec4 &a, float b);
Vec4 operator*(float a, const Vec4 &b);
Vec4 operator/(const Vec4 &a, float b);
Vec4 operator/(float a, const Vec4 &b);
bool operator>(const Vec4 &a, const Vec4 &b);
bool operator<(const Vec4 &a, const Vec4 &b);
bool operator>=(const Vec4 &a, const Vec4 &b);
bool operator<=(const Vec4 &a, const Vec4 &b);
bool operator==(const Vec4 &a, const Vec4 &b);
bool operator!=(const Vec4 &a, const Vec4 &b);

Vec4 expf(const Vec4 &v);
Vec4 powf(const Vec4 &v, float p);

std::ostream &operator<<(std::ostream &os, const Vec4 &v);
#endif
