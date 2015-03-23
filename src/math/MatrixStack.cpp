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

#include "MatrixStack.hpp"
#include "Debug.hpp"

#include <deque>

std::stack<Mat4> MatrixStack::_stacks[3] = {
    std::stack<Mat4>(std::deque<Mat4>(1, Mat4())),
    std::stack<Mat4>(std::deque<Mat4>(1, Mat4())),
    std::stack<Mat4>(std::deque<Mat4>(1, Mat4()))
};

void MatrixStack::set(StackName n, const Mat4 &m) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].top() = m;
}

void MatrixStack::mulR(StackName n, const Mat4 &m) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].top() = _stacks[n].top()*m;
}

void MatrixStack::mulL(StackName n, const Mat4 &m) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].top() = m*_stacks[n].top();
}

void MatrixStack::get(StackName n, Mat4 &m) {
    switch(n) {
    case PROJECTION_STACK:
    case MODEL_STACK:
    case VIEW_STACK:
        m = _stacks[n].top();
        break;
    case MODELVIEW_STACK:
        m = _stacks[VIEW_STACK].top().pseudoInvert()*_stacks[MODEL_STACK].top();
        break;
    case MODELVIEWPROJECTION_STACK:
        m = _stacks[PROJECTION_STACK].top()*
            _stacks[VIEW_STACK].top().pseudoInvert()*
            _stacks[MODEL_STACK].top();
        break;
    case INV_MODEL_STACK:
        m = _stacks[MODEL_STACK].top().pseudoInvert();
        break;
    case INV_VIEW_STACK:
        m = _stacks[VIEW_STACK].top().pseudoInvert();
        break;
    case INV_MODELVIEW_STACK:
        m = _stacks[MODEL_STACK].top().pseudoInvert()*_stacks[VIEW_STACK].top();
        break;
    default:
        FAIL("Invalid matrix stack\n");
    }
}

void MatrixStack::copyPush(StackName n) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].push(_stacks[n].top());
}

void MatrixStack::push(StackName n) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].push(Mat4());
}

void MatrixStack::pop(StackName n) {
    ASSERT(n <= VIEW_STACK, "Cannot manipulate virtual stacks\n");
    _stacks[n].pop();
}
