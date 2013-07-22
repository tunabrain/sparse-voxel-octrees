/*
 * Events.h
 *
 *  Created on: 10.01.2011
 *      Author: Noobody
 */

#ifndef EVENTS_H_
#define EVENTS_H_

void CheckEvents();
int WaitEvent();
int GetMouseX();
int GetMouseY();
int GetMouseZ();
int GetMouseXSpeed();
int GetMouseYSpeed();
int GetMouseZSpeed();
int GetMouseDown(int Button);
int GetKeyHit(int Button);
int GetKeyDown(int Button);

#endif /* EVENTS_H_ */
