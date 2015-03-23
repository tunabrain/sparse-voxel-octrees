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

#include "ThreadBarrier.hpp"

#include <SDL.h>

ThreadBarrier::ThreadBarrier(int numThreads) : _numThreads(numThreads) {
    _barrierMutex = SDL_CreateMutex();
    _turnstile1   = SDL_CreateSemaphore(0);
    _turnstile2   = SDL_CreateSemaphore(0);
    _waitCount = 0;
}

ThreadBarrier::~ThreadBarrier() {
    SDL_DestroyMutex(_barrierMutex);
    SDL_DestroySemaphore(_turnstile1);
    SDL_DestroySemaphore(_turnstile2);
}

void ThreadBarrier::waitPre() {
    SDL_mutexP(_barrierMutex);
    if (++_waitCount == _numThreads)
        for (int i = 0; i < _numThreads; i++)
            SDL_SemPost(_turnstile1);
    SDL_mutexV(_barrierMutex);

    SDL_SemWait(_turnstile1);
}

void ThreadBarrier::waitPost() {
    SDL_mutexP(_barrierMutex);
    if (--_waitCount == 0)
        for (int i = 0; i < _numThreads; i++)
            SDL_SemPost(_turnstile2);
    SDL_mutexV(_barrierMutex);

    SDL_SemWait(_turnstile2);
}

void ThreadBarrier::releaseAll() {
    for (int i = 0; i < _numThreads; i++) {
        SDL_SemPost(_turnstile1);
        SDL_SemPost(_turnstile2);
    }
}
