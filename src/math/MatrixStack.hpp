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

#ifndef RENDER_MATRIXSTACK_HPP_
#define RENDER_MATRIXSTACK_HPP_

#include "Mat4.hpp"

#include <stack>

enum StackName {
    PROJECTION_STACK,
    MODEL_STACK,
    VIEW_STACK,
    /* Virtual stacks */
    MODELVIEW_STACK,
    MODELVIEWPROJECTION_STACK,
    INV_MODEL_STACK,
    INV_VIEW_STACK,
    INV_MODELVIEW_STACK,

    MATRIX_STACK_COUNT
};

enum StackFlag {
    PROJECTION_FLAG          = (1 << 0),
    MODEL_FLAG               = (1 << 1),
    VIEW_FLAG                = (1 << 2),
    /* Virtual stacks */
    MODELVIEW_FLAG           = (1 << 3),
    MODELVIEWPROJECTION_FLAG = (1 << 4),
    INV_MODEL_FLAG           = (1 << 5),
    INV_VIEW_FLAG            = (1 << 6),
    INV_MODELVIEW_FLAG       = (1 << 7)
};

class MatrixStack {
    static std::stack<Mat4> _stacks[];

    MatrixStack();

public:
    static void set(StackName n, const Mat4 &m);
    static void mulR(StackName n, const Mat4 &m);
    static void mulL(StackName n, const Mat4 &m);
    static void get(StackName n, Mat4 &m);

    static void copyPush(StackName n);
    static void push(StackName n);
    static void pop(StackName n);
};


#endif /* RENDER_MATRIXSTACK_HPP_ */
