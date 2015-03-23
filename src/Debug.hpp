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

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#define DEBUG_LEVEL DEBUG

enum DebugLevel {
    WARN,
    INFO,
    DEBUG
};

#ifndef NDEBUG
# define DBG(MODULE, LEVEL, ...) debugLog(MODULE, LEVEL, __VA_ARGS__)
# define ASSERT(EXP, ...) debugAssert(__FILE__, __LINE__, (bool)(EXP), __VA_ARGS__)
# define FAIL(...) debugFail(__FILE__, __LINE__, __VA_ARGS__)
#else
# define DBG(MODULE, LEVEL, FMT, ...) do {} while (0);
# define ASSERT(A, B, ...) do { (bool)(A); } while (0);
# define FAIL(A, ...) do {} while (0);
#endif

void debugLog(const char *module, DebugLevel level, const char *format, ...);
void debugAssert(const char *file, int line, bool exp, const char *format, ...);
void debugFail(const char *file, int line, const char *format, ...);

#endif /* DEBUG_HPP_ */
