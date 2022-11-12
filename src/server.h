/*

	CWP Mockup with FLTK 1.3

	(c) 2020 by Christian.Lorenz@gromeck.de


	This file is part of CWP Mockup.

    CWP Mockup is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CWP Mockup is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CWP Mockup.  If not, see <https://www.gnu.org/licenses/>.

*/
#ifndef __SERVER_H__
#define __SERVER_H__

#include "coordinate.h"

/*
**	update rate for the track recomputation (in ms)
*/
#define SERVER_TRACK_UPDATE_RATE   			500

/*
**	if we are close to the headed position, we will bounce
*/
#define SERVER_DISTANCE_REACHED				5

/*
**	struct to hold the track information
*/
typedef struct {
	char callsign[CALLSIGN_LENGTH + 1];
	Coordinate position;
	Coordinate heading;
	Coordinate prediction;
	int speed;
	unsigned long last_update;
} SERVER_TRACK;

int runServer(int port);

#endif
