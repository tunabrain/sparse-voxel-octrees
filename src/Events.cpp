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

#include "Events.hpp"

#include <SDL.h>

static int mouseX = 0;
static int mouseY = 0;
static int mouseZ = 0;
static int mouseXSpeed = 0;
static int mouseYSpeed = 0;
static int mouseZSpeed = 0;
static int mouseDown[] = {0, 0};
static char keyHit[SDLK_LAST];
static char keyDown[SDLK_LAST];

static void processEvent(SDL_Event event) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            mouseX = event.motion.x;
            mouseY = event.motion.y;
            mouseXSpeed = event.motion.xrel;
            mouseYSpeed = event.motion.yrel;

            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_WHEELUP) {
                mouseZSpeed = 1;
                mouseZ     += 1;
            } else if(event.button.button == SDL_BUTTON_WHEELDOWN) {
                mouseZSpeed = -1;
                mouseZ     -=  1;
            } else if (event.button.button == SDL_BUTTON_LEFT)
                mouseDown[0] = 1;
            else if (event.button.button == SDL_BUTTON_RIGHT)
                mouseDown[1] = 1;

            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT)
                mouseDown[0] = 0;
            else if (event.button.button == SDL_BUTTON_RIGHT)
                mouseDown[1] = 0;

            break;
        case SDL_KEYDOWN:
            keyHit[event.key.keysym.sym] = 1;
            keyDown[event.key.keysym.sym] = 1;

            break;
        case SDL_KEYUP:
            keyDown[event.key.keysym.sym] = 0;

            break;
        case SDL_QUIT:
            exit(0);
    }
}

int waitEvent() {
    SDL_Event event;
    SDL_WaitEvent(&event);
    processEvent(event);

    return event.type;
}

void checkEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event))
        processEvent(event);
}

int getMouseX() {
    return mouseX;
}

int getMouseY() {
    return mouseY;
}

int getMouseZ() {
    return mouseZ;
}

int getMouseXSpeed() {
    int Temp = mouseXSpeed;

    mouseXSpeed = 0;

    return Temp;
}

int getMouseYSpeed() {
    int Temp = mouseYSpeed;

    mouseYSpeed = 0;

    return Temp;
}

int getMouseZSpeed() {
    int Temp = mouseZSpeed;

    mouseZSpeed = 0;

    return Temp;
}

int getMouseDown(int Button) {
    return mouseDown[Button];
}

int getKeyHit(int Key) {
    int Temp = keyHit[Key];

    keyHit[Key] = 0;

    return Temp;
}

int getKeyDown(int Key) {
    return keyDown[Key];
}
