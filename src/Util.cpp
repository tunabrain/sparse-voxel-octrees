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

#include "Util.hpp"

#include <sstream>

std::string prettyPrintMemory(uint64 size)
{
    const char *unit;
    uint64 base;
    if (size < 1024) {
        base = 1;
        unit = " bytes";
    } else if (size < 1024*1024) {
        base = 1024;
        unit = " KB";
    } else if (size < uint64(1024)*1024*1024) {
        base = 1024*1024;
        unit = " MB";
    } else {
        base = uint64(1024)*1024*1024;
        unit = " GB";
    }

    std::ostringstream out;

    if (size/base < 10)
        out << ((size*100)/base)*0.01;
    else if (size/base < 100)
        out << ((size*10)/base)*0.1;
    else
        out << size/base;
    out << unit;

    return out.str();
}
