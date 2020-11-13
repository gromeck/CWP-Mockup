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
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "coordinate.h"

#define CLIENT_REFRESH_RATE_MIN		    10
#define CLIENT_REFRESH_RATE_MAX		    5000
#define CLIENT_REFRESH_RATE_DEFAULT	    1000

#define CLIENT_WINDOW_WIDTH_MIN	        800
#define CLIENT_WINDOW_HEIGHT_MIN        600

#define CLIENT_HISTORY_DOTS_MAX         10

// all items in px
#define CLIENT_SYMBOL_WIDTH             15
#define CLIENT_SYMBOL_HEIGHT            15
#define CLIENT_LABEL_OFFSET_X           50
#define CLIENT_LABEL_OFFSET_Y           -25

#define CLIENT_TRACK_UPDATE_RATE        1
// in ms
#define CLIENT_TRACK_COASTING_TIMEOUT   3000
#define CLIENT_TRACK_INVALID_TIMEOUT    10000

// short term conflict alert, nm
#define CLIENT_STCA_DISTANCE            10.0

// some frontend colors
#define CLIENT_COLOR_BACKGROUND         FL_LIGHT1
#define CLIENT_COLOR_SYMBOL             FL_WHITE
#define CLIENT_COLOR_LABEL_LINE         FL_GRAY
#define CLIENT_COLOR_LABEL              FL_DARK_GREEN
#define CLIENT_COLOR_ALERT              FL_RED
#define CLIENT_COLOR_PREDICTION         FL_WHITE
#define CLIENT_COLOR_HISTORY            FL_WHITE

typedef struct {
    char callsign[CALLSIGN_LENGTH + 1];
    Coordinate position;
    int speed;
    Coordinate prediction;
} CLIENT_TRACK_UPDATE;

typedef struct {
    bool valid;
    bool coasting;
    bool stca;
    int history_dots;
    Coordinate history[CLIENT_HISTORY_DOTS_MAX];
    struct timeval last_update;
    CLIENT_TRACK_UPDATE track;
} CLIENT_TRACK;

int runClient(const char *server,int port,bool fullscreen);

#endif
