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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "Debug.hpp"

const char *levelStr[] = {"WARN", "INFO", "DEBUG"};

void debugLog(const char *module, DebugLevel level, const char *fmt, ...)
{
    if (level > DEBUG_LEVEL)
        return;
    va_list argp;
    va_start(argp, fmt);
    printf("%s | %-10s | ", levelStr[level], module);
    vprintf(fmt, argp);
    va_end(argp);
    fflush(stdout);
}

void debugAssert(const char *file, int line, bool exp, const char *format, ...) {
    if (!exp) {
        printf("ASSERTION FAILURE:  %s:%d:  ", file, line);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
}

void debugFail(const char *file, int line, const char *format, ...) {
    printf("PROGRAM FAILURE:  %s:%d:  ", file, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    exit(EXIT_FAILURE);
}
