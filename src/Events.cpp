#include <SDL/SDL.h>

#include "Events.hpp"

int MouseX = 0;
int MouseY = 0;
int MouseZ = 0;
int MouseXSpeed = 0;
int MouseYSpeed = 0;
int MouseZSpeed = 0;
int MouseDown[] = {0, 0};
char KeyHit[SDLK_LAST];
char KeyDown[SDLK_LAST];

void ProcessEvent(SDL_Event Event) {
	switch (Event.type) {
		case SDL_MOUSEMOTION:
			MouseX = Event.motion.x;
			MouseY = Event.motion.y;
			MouseXSpeed = Event.motion.xrel;
			MouseYSpeed = Event.motion.yrel;

			break;
		case SDL_MOUSEBUTTONDOWN:
			if (Event.button.button == SDL_BUTTON_WHEELUP) {
				MouseZSpeed = 1;
				MouseZ     += 1;
			} else if(Event.button.button == SDL_BUTTON_WHEELDOWN) {
				MouseZSpeed = -1;
				MouseZ     -=  1;
			} else if (Event.button.button == SDL_BUTTON_LEFT)
				MouseDown[0] = 1;
			else if (Event.button.button == SDL_BUTTON_RIGHT)
				MouseDown[1] = 1;

			break;
		case SDL_MOUSEBUTTONUP:
			if (Event.button.button == SDL_BUTTON_LEFT)
				MouseDown[0] = 0;
			else if (Event.button.button == SDL_BUTTON_RIGHT)
				MouseDown[1] = 0;

			break;
		case SDL_KEYDOWN:
			KeyHit[Event.key.keysym.sym] = 1;
			KeyDown[Event.key.keysym.sym] = 1;

			break;
		case SDL_KEYUP:
			KeyDown[Event.key.keysym.sym] = 0;

			break;
		case SDL_QUIT:
			exit(0);
	}
}

int WaitEvent() {
	SDL_Event Event;

	SDL_WaitEvent(&Event);

	ProcessEvent(Event);

	return Event.type;
}

void CheckEvents() {
    SDL_Event Event;

    while (SDL_PollEvent(&Event))
    	ProcessEvent(Event);
}

int GetMouseX() {
	return MouseX;
}

int GetMouseY() {
	return MouseY;
}

int GetMouseZ() {
	return MouseZ;
}

int GetMouseXSpeed() {
	int Temp = MouseXSpeed;

	MouseXSpeed = 0;

	return Temp;
}

int GetMouseYSpeed() {
	int Temp = MouseYSpeed;

	MouseYSpeed = 0;

	return Temp;
}

int GetMouseZSpeed() {
	int Temp = MouseZSpeed;

	MouseZSpeed = 0;

	return Temp;
}

int GetMouseDown(int Button) {
	return MouseDown[Button];
}

int GetKeyHit(int Key) {
	int Temp = KeyHit[Key];

	KeyHit[Key] = 0;

	return Temp;
}

int GetKeyDown(int Key) {
	return KeyDown[Key];
}
