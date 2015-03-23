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

#ifndef MATH_MAT4_HPP_
#define MATH_MAT4_HPP_

#include "Vec3.hpp"

struct Mat4 {
    union {
        struct {
            float a11, a12, a13, a14;
            float a21, a22, a23, a24;
            float a31, a32, a33, a34;
            float a41, a42, a43, a44;
        };
        float a[16];
    };

    Mat4();
    Mat4(float _a11, float _a12, float _a13, float _a14,
         float _a21, float _a22, float _a23, float _a24,
         float _a31, float _a32, float _a33, float _a34,
         float _a41, float _a42, float _a43, float _a44);

    Mat4 transpose() const;
    Mat4 pseudoInvert() const;

    Mat4 operator*(const Mat4 &b) const;
    Vec3 operator*(const Vec3 &b) const;

    Vec3 transformVector(const Vec3 &v) const;

    static Mat4 translate(const Vec3 &v);
    static Mat4 scale(const Vec3 &s);
    static Mat4 rotXYZ(const Vec3 &rot);
    static Mat4 rotYZX(const Vec3 &rot);
    static Mat4 rotAxis(const Vec3 &axis, float angle);

    static Mat4 ortho(float l, float r, float b, float t, float near, float far);
    static Mat4 perspective(float aov, float ratio, float near, float far);
    static Mat4 lookAt(const Vec3 &pos, const Vec3 &fwd, const Vec3 &up);
};

#endif
