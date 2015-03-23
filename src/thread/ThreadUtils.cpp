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

#include "ThreadPool.hpp"

#include <algorithm>
#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <thread>

namespace ThreadUtils {

ThreadPool *pool = nullptr;

uint32 idealThreadCount()
{
    // std::thread::hardware_concurrency support is not great, so let's try
    // native APIs first
#if _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (info.dwNumberOfProcessors > 0)
        return info.dwNumberOfProcessors;
#else
    int nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs > 0)
        return nprocs;
#endif
    unsigned n = std::thread::hardware_concurrency();
    if (n > 0)
        return n;

    // All attempts failed. Let's take a guess
    return 4;
}

void startThreads(int numThreads)
{
    pool = new ThreadPool(numThreads);
}

void parallelFor(uint32 start, uint32 end, uint32 partitions, std::function<void(uint32)> func)
{
    auto taskRun = [&func, start, end](uint32 idx, uint32 num, uint32 /*threadId*/)
    {
        uint32 span = (end - start + num - 1)/num;
        uint32 iStart = start + span*idx;
        uint32 iEnd = std::min(iStart + span, end);
        for (uint32 i = iStart; i < iEnd; ++i)
            func(i);
    };
    if (partitions == 1)
        taskRun(0, 1, 0);
    else
        pool->yield(*pool->enqueue(taskRun, partitions));
}

}
