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

#ifndef TIMER_HPP_
#define TIMER_HPP_

#include <iostream>
#include <string>
#include <chrono>
#if _WIN32
#include <windows.h>
#endif

// std::chrono::high_resolution_clock has disappointing accuracy on windows
// On windows, we use the WinAPI high performance counter instead
class Timer
{
#if _WIN32
    LARGE_INTEGER _pfFrequency;
    LARGE_INTEGER _start, _stop;
#else
    std::chrono::time_point<std::chrono::high_resolution_clock> _start, _stop;
#endif
public:
    Timer()
    {
#if _WIN32
        QueryPerformanceFrequency(&_pfFrequency);
#endif
        start();
    }

    void start()
    {
#if _WIN32
        QueryPerformanceCounter(&_start);
#else
        _start = std::chrono::high_resolution_clock::now();
#endif
    }

    void stop()
    {
#if _WIN32
        QueryPerformanceCounter(&_stop);
#else
        _stop = std::chrono::high_resolution_clock::now();
#endif
    }

    void bench(const std::string &s)
    {
        stop();
        std::cout << s << ": " << elapsed() << " s" << std::endl;
    }

    double elapsed() const
    {
#if _WIN32
        return double(_stop.QuadPart - _start.QuadPart)/double(_pfFrequency.QuadPart);
#else
        return std::chrono::duration<double>(_stop - _start).count();
#endif
    }
};

#endif /* TIMER_HPP_ */
