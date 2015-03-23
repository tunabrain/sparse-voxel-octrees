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

#include "Mat4.hpp"

#include <cmath>

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

Mat4::Mat4() {
    a12 = a13 = a14 = 0.0f;
    a21 = a23 = a24 = 0.0f;
    a31 = a32 = a34 = 0.0f;
    a41 = a42 = a43 = 0.0f;
    a11 = a22 = a33 = a44 = 1.0f;
}

Mat4::Mat4(
    float _a11, float _a12, float _a13, float _a14,
    float _a21, float _a22, float _a23, float _a24,
    float _a31, float _a32, float _a33, float _a34,
    float _a41, float _a42, float _a43, float _a44) :
    a11(_a11), a12(_a12), a13(_a13), a14(_a14),
    a21(_a21), a22(_a22), a23(_a23), a24(_a24),
    a31(_a31), a32(_a32), a33(_a33), a34(_a34),
    a41(_a41), a42(_a42), a43(_a43), a44(_a44) {}

Mat4 Mat4::transpose() const {
    return Mat4(
        a11, a21, a31, a41,
        a12, a22, a32, a42,
        a13, a23, a33, a43,
        a14, a24, a34, a44
    );
}

Mat4 Mat4::pseudoInvert() const {
    Mat4 trans = translate(Vec3(-a14, -a24, -a34));
    Mat4 rot = transpose();
    rot.a41 = rot.a42 = rot.a43 = 0.0f;

    return rot*trans;
}

Mat4 Mat4::operator*(const Mat4 &b) const {
    Mat4 result;
    for (int i = 0; i < 4; i++)
        for (int t = 0; t < 4; t++)
            result.a[i*4 + t] =
                a[i*4 + 0]*b.a[0*4 + t] +
                a[i*4 + 1]*b.a[1*4 + t] +
                a[i*4 + 2]*b.a[2*4 + t] +
                a[i*4 + 3]*b.a[3*4 + t];

    return result;
}

Vec3 Mat4::operator*(const Vec3 &b) const {
    return Vec3(
        a11*b.x + a12*b.y + a13*b.z + a14,
        a21*b.x + a22*b.y + a23*b.z + a24,
        a31*b.x + a32*b.y + a33*b.z + a34
    );
}

Vec3 Mat4::transformVector(const Vec3 &b) const {
    return Vec3(
        a11*b.x + a12*b.y + a13*b.z,
        a21*b.x + a22*b.y + a23*b.z,
        a31*b.x + a32*b.y + a33*b.z
    );
}

Mat4 Mat4::translate(const Vec3 &v) {
    return Mat4(
        1.0f, 0.0f, 0.0f, v.x,
        0.0f, 1.0f, 0.0f, v.y,
        0.0f, 0.0f, 1.0f, v.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Mat4 Mat4::scale(const Vec3 &s) {
    return Mat4(
         s.x, 0.0f, 0.0f, 0.0f,
        0.0f,  s.y, 0.0f, 0.0f,
        0.0f, 0.0f,  s.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Mat4 Mat4::rotXYZ(const Vec3 &rot) {
    Vec3 r = rot*float(M_PI)/180.0f;
    float c[] = {std::cos(r.x), std::cos(r.y), std::cos(r.z)};
    float s[] = {std::sin(r.x), std::sin(r.y), std::sin(r.z)};

    return Mat4(
        c[1]*c[2], -c[0]*s[2] + s[0]*s[1]*c[2],  s[0]*s[2] + c[0]*s[1]*c[2], 0.0f,
        c[1]*s[2],  c[0]*c[2] + s[0]*s[1]*s[2], -s[0]*c[2] + c[0]*s[1]*s[2], 0.0f,
            -s[1],                   s[0]*c[1],                   c[0]*c[1], 0.0f,
             0.0f,                        0.0f,                        0.0f, 1.0f
    );
}

Mat4 Mat4::rotYZX(const Vec3 &rot) {
    Vec3 r = rot*float(M_PI)/180.0f;
    float c[] = {std::cos(r.x), std::cos(r.y), std::cos(r.z)};
    float s[] = {std::sin(r.x), std::sin(r.y), std::sin(r.z)};

    return Mat4(
         c[1]*c[2],  c[0]*c[1]*s[2] - s[0]*s[1], c[0]*s[1] + c[1]*s[0]*s[2], 0.0f,
             -s[2],                   c[0]*c[2],                  c[2]*s[0], 0.0f,
        -c[2]*s[1], -c[1]*s[0] - c[0]*s[1]*s[2], c[0]*c[1] - s[0]*s[1]*s[2], 0.0f,
              0.0f,                        0.0f,                       0.0f, 1.0f
    );
}

Mat4 Mat4::rotAxis(const Vec3 &axis, float angle) {
    angle *= float(M_PI)/180.0f;
    float s = std::sin(angle);
    float c = std::cos(angle);
    float c1 = 1.0f - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return Mat4(
           c + x*x*c1,  x*y*c1 - z*s,  x*z*c1 + y*s, 0.0f,
         y*x*c1 + z*s,    c + y*y*c1,  y*z*c1 - x*s, 0.0f,
         z*x*c1 - y*s,  z*y*c1 + x*s,    c + z*z*c1, 0.0f,
                 0.0f,          0.0f,          0.0f, 1.0f
    );
}

Mat4 Mat4::ortho(float l, float r, float b, float t, float n, float f) {
    return Mat4(
        2.0f/(r-l),       0.0f,        0.0f, -(r+l)/(r-l),
              0.0f, 2.0f/(t-b),        0.0f, -(t+b)/(t-b),
              0.0f,       0.0f, -2.0f/(f-n), -(f+n)/(f-n),
              0.0f,       0.0f,        0.0f,          1.0f
    );
}

Mat4 Mat4::perspective(float aov, float ratio, float near, float far) {
    float t = 1.0f/std::tan(aov*(float(M_PI)/360.0f));
    float a = (far + near)/(far - near);
    float b = 2.0f*far*near/(far - near);
    float c = t/ratio;

    return Mat4(
           c, 0.0f,  0.0f, 0.0f,
        0.0f,    t,  0.0f, 0.0f,
        0.0f, 0.0f,    -a,   -b,
        0.0f, 0.0f, -1.0f, 0.0f
    );
}

Mat4 Mat4::lookAt(const Vec3 &pos, const Vec3 &fwd, const Vec3 &up) {
    Vec3 f = fwd.normalize();
    Vec3 r = f.cross(up).normalize();
    Vec3 u = r.cross(f).normalize();

    return Mat4(
         r.x,  u.x,  f.x, pos.x,
         r.y,  u.y,  f.y, pos.y,
         r.z,  u.z,  f.z, pos.z,
        0.0f, 0.0f, 0.0f,  1.0f
    );

}
