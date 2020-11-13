/*

	CWP-Mockup with FLTK 1.3

	(c) 2020 by Christian.Lorenz@gromeck.de

	This file is part of CWP-MockUp.

    CWP-MockUp is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CWP-MockUp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CWP-MockUp.  If not, see <https://www.gnu.org/licenses/>.

*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include "common.h"
#include "client.h"

/*
**	return random coordinates
*/
COORD getRandomCoordinates(void)
{
	COORD coord;

	coord.x = RANDOM(0,MAP_WIDTH);
	coord.y = RANDOM(0,MAP_HEIGHT);
	coord.z = RANDOM(0,MAP_DEPTH);

	return coord;
}

/*
**	subtract coordinates and return the result
*/
COORD subtractCoordinates(COORD a,COORD b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;

	return a;
}

/*
**	add coordinates and return the result
*/
COORD addCoordinates(COORD a,COORD b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;

	return a;
}

/*
**	multiply coordinates and return the result
*/
COORD multiplyCoordinates(COORD a,double factor)
{
	a.x *= factor;
	a.y *= factor;
	a.z *= factor;

	return a;
}

/*
**	multiply coordinates and return the result
*/
bool insideMapCoordinates(COORD a)
{
	if (a.x < 0 || a.x > MAP_WIDTH)
		return false;
	if (a.y < 0 || a.y > MAP_HEIGHT)
		return false;
	if (a.z < 0 || a.z > MAP_DEPTH)
		return false;
	return true;
}

/*
**	compute the distance between two points
*/
double computeDistance(COORD a,COORD b)
{
	double x = a.x - b.x;
	double y = a.y - b.y;
	double z = a.z - b.z;

	return sqrt(x * x + y * y + z * z);
}/**/
