/*

	CWP-Mockup in JavaScript

	(c) 2022 by Christian.Lorenz@gromeck.de


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

const REFRESH_RATE_MIN = 10;
const REFRESH_RATE_MAX = 5000;
const REFRESH_RATE_DEFAULT = 1000;
const TRACKS_MAX = 3000;

/*
**	colors
*/
const CLIENT_SYSINFO_COLOR = 0x000000;		// FL_BLACK
const CLIENT_CLOCK_COLOR = 0x959595;		// FL_DARK2
const CLIENT_SYMBOL_COLOR = 0xffffff;		// FL_WHITE
const CLIENT_COLOR_BACKGROUND = 0xcbcbcb;	// FL_LIGHT1
const CLIENT_COLOR_MAP_BORDER = 0xffff00;	// FL_YELLOW


/*
**  some atc basics
*/
function KNOTS2NMS(knots)    { return knots * 0.0002777777777; }
function NMS2KNOTS(nms)      { return nms / KNOTS2NMS(1); }
function NM2FT(nm)           { return nm * 6076.12; }
function FT2NM(ft)           { return ft / NM2FT(1); }
function FT2FL(ft)           { return ft / 100; }
function FL2FT(fl)           { return fl * 100; }

/*
**	timing basics
*/
const MSEC_PER_SEC = 1000;
function FPS2MS(fps)         { return MSEC_PER_SEC / fps; }
function MS2FPS(fps)         { return MSEC_PER_SEC / ms; }
function MS2S(ms)            { return ms / MSEC_PER_SEC; }
function S2MS(s)             { return s * MSEC_PER_SEC; }

/*
**	some map definitions
*/
const MAP_ZOOM_MIN = 0.1;
const MAP_ZOOM_MAX = 10.0;
const MAP_ZOOM_STEP = 10;

const MAP_HEIGHT = 400;	// nm
const MAP_WIDTH = 500;	// nm
const MAP_DEPTH = FT2NM(FL2FT(400));

// number of history dots
const CLIENT_HISTORY_DOTS_MAX = 10;
// time between the history dots
const CLIENT_TRACK_HISTORY_TIME= 10000;   // ms

// client symbol definitions
const CLIENT_SYMBOL_WIDTH = 15;      // px
const CLIENT_SYMBOL_HEIGHT = 15;      // px
const CLIENT_SYMBOL_THICKNESS = 1;       // px

const CLIENT_LABEL_OFFSET_X = 50;      // px
const CLIENT_LABEL_OFFSET_Y = -25;     // px
const CLIENT_LABEL_LINES = 3;
const CLIENT_LABEL_FONTFACE = 'courier';
const CLIENT_LABEL_FONTSIZE = 10;      // pt
const CLIENT_LABEL_FONTWEIGHT = 'normal';
const CLIENT_LABEL_LINE_COLOR = 0xc0c0c0	// FL_GRAY
const CLIENT_LABEL_LINE_THICKNESS = 1;       // px
const CLIENT_LABEL_COLOR = 0x009100;	// FL_DARK_GREEN
const CLIENT_LABEL_COLOR_OLD = 0x00007f;	// FL_DARK_BLUE

const CLIENT_PREDICTION_LINE_COLOR = 0xffffff;	// FL_WHITE
const CLIENT_PREDICTION_LINE_THICKNESS = 2;       // px

const CLIENT_ALERT_LINE_COLOR = 0xff0000;	// FL_RED
const CLIENT_ALERT_LINE_THICKNESS = 1;       // px

const CLIENT_HISTORY_COLOR = 0xffffff;	//FL_WHITE
const CLIENT_HISTORY_THICKNESS = 2;       // px

// some timing constants
const CLIENT_COMMUNICATION_UPDATE_RATE = 1000;    // ms
const CLIENT_TRACK_UPDATE_RATE = 1000;		// ms
const CLIENT_TRACK_COASTING_TIMEOUT = 5000;    // ms
const CLIENT_TRACK_TOOOLD_TOLERANCE = 100;     // ms
const CLIENT_TRACK_INVALID_TIMEOUT = 10000;   // ms
const CLIENT_TRACK_RECEIVING_TIMEOUT = 10000;   // ms

// short term conflict alert
const CLIENT_STCA_DISTANCE = 5.0;     // nm

// frozen picture
const CLIENT_NODATA_BACKGROUND = 0xff0000;
const CLIENT_NODATA_FOREGROUND = 0xffffff;
const CLIENT_NODATA_WIDTH = 400;    // px
const CLIENT_NODATA_HEIGHT = 30;      // px
const CLIENT_NODATA_FONTFACE = 'Helvetica Bold,sans-serif';
const CLIENT_NODATA_FONTSIZE = 24;      // pt
const CLIENT_NODATA_FONTWEIGHT = 'bold';
const CLIENT_NODATA_MESSAGE_NODATA = ' NO TRACK DATA ';
const CLIENT_NODATA_MESSAGE_NOCONN = ' NO SERVER CONNECTION ';

// information
const CLIENT_STATS_DISPLAY_RATE = 1000;    // ms
const CLIENT_STATS_FOREGROUND = 0xffffff;
const CLIENT_STATS_BACKGROUND = 0x959595;
const CLIENT_STATS_FONTFACE = 'Helvetica';
const CLIENT_STATS_FONTWEIGHT = 'normal';
const CLIENT_STATS_FONTSIZE = 16;      // pt

/**/
