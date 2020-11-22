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

#define CLIENT_SYSINFO_WIDTH            150
#define CLIENT_SYSINFO_HEIGHT           12
#define CLIENT_SYSINFO_COLOR            FL_BLACK
#define CLIENT_SYSINFO_FONTFACE         FL_SCREEN

#define CLIENT_CLOCK_WIDTH              150
#define CLIENT_CLOCK_HEIGHT             (CLIENT_CLOCK_WIDTH * 2 / 10)
#define CLIENT_CLOCK_COLOR              FL_DARK2
#define CLIENT_CLOCK_FONTFACE           FL_SCREEN_BOLD

#define CLIENT_REFRESH_RATE_MIN		    10
#define CLIENT_REFRESH_RATE_MAX		    5000
#define CLIENT_REFRESH_RATE_DEFAULT	    1000

#define CLIENT_WINDOW_WIDTH_MIN	        800
#define CLIENT_WINDOW_HEIGHT_MIN        600

// number of history dots
#define CLIENT_HISTORY_DOTS_MAX         10
// time between the history dots in ms
#define CLIENT_TRACK_HISTORY_TIME       10000

// all items in px
#define CLIENT_SYMBOL_WIDTH             15
#define CLIENT_SYMBOL_HEIGHT            15
#define CLIENT_LABEL_OFFSET_X           50
#define CLIENT_LABEL_OFFSET_Y           -25
#define CLIENT_LABEL_LINES              3
#define CLIENT_LABEL_FONTFACE           FL_COURIER
#define CLIENT_LABEL_FONTSIZE           10

#define CLIENT_TRACK_UPDATE_RATE        1

// in ms
#define CLIENT_TRACK_COASTING_TIMEOUT   3000
#define CLIENT_TRACK_INVALID_TIMEOUT    10000
#define CLIENT_TRACK_TOOOLD_TIMEOUT     1000

// short term conflict alert, nm
#define CLIENT_STCA_DISTANCE            5.0

// frozen picture
#define CLIENT_NODATA_BACKGROUND        FL_RED
#define CLIENT_NODATA_FOREGROUND        FL_WHITE
#define CLIENT_NODATA_WIDTH             300
#define CLIENT_NODATA_HEIGHT            30
#define CLIENT_NODATA_FONTFACE          FL_HELVETICA_BOLD
#define CLIENT_NODATA_FONTSIZE          24
#define CLIENT_NODATA_MESSAGE_NODATA    " NO TRACK DATA "
#define CLIENT_NODATA_MESSAGE_NOCONN    " NO SERVER CONNECTION "

// information
#define CLIENT_STATS_DISPPLAY_RATE      1.0 // seconds
#define CLIENT_STATS_FOREGROUND         FL_WHITE
#define CLIENT_STATS_BACKGROUND         FL_DARK2
#define CLIENT_STATS_FONTFACE           FL_HELVETICA
#define CLIENT_STATS_FONTSIZE           16

// some frontend colors
#define CLIENT_COLOR_BACKGROUND         FL_LIGHT1
#define CLIENT_COLOR_SYMBOL             FL_WHITE
#define CLIENT_COLOR_LABEL_LINE         FL_GRAY
#define CLIENT_COLOR_LABEL              FL_DARK_GREEN
#define CLIENT_COLOR_LABEL_OLD          FL_DARK_BLUE
#define CLIENT_COLOR_ALERT              FL_RED
#define CLIENT_COLOR_PREDICTION         FL_WHITE
#define CLIENT_COLOR_HISTORY            FL_WHITE
#define CLIENT_COLOR_MAP_BORDER         FL_YELLOW

/*
**  map projection
*/
#define CLIENT_MAP_ZOOM_MIN             0.1
#define CLIENT_MAP_ZOOM_MAX             10.0
#define CLIENT_MAP_ZOOM_STEP            10 //percent

typedef struct {
    char callsign[CALLSIGN_LENGTH + 1];
    Coordinate position;
    int speed;
    Coordinate prediction;
    struct timeval timestamp;
} CLIENT_TRACK_UPDATE;

typedef struct {
    bool valid;
    bool coasting;
    bool stca;
    int history_dots;
    Coordinate history[CLIENT_HISTORY_DOTS_MAX];
    struct timeval last_history_update;
    struct timeval last_update;
    CLIENT_TRACK_UPDATE track;
} CLIENT_TRACK;

int runClient(const char *server,int port,bool fullscreen);

#endif
