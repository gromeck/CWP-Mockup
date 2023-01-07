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
#ifndef __COMMON_H__
#define __COMMON_H__

#include <Poco/Channel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>

#define __TITLE__		    "CWP-Mockup"
#define __VERSION_NR__	    "0.1"


/*
**	default server port
*/
#define DEFAULT_PORT    	2566

/*
**	default document root
*/
#define DEFAULT_HTDOC_ROOT	"/usr/share/" __TITLE__ "/htdocs/"

/*
**  some very common stuff
*/
#define MSEC_PER_SEC        1000
#define USEC_PER_MSEC       1000
#define USEC_PER_SEC        (USEC_PER_MSEC * MSEC_PER_SEC)

#define RANDOM(a,b)         ((a) + (random() % (unsigned long) ((b) - (a))))

/*
**  some atc basics
*/
#define KNOTS2NMS(knots)    ((knots) * 0.0002777777777)
#define NMS2KNOTS(nms)      ((nms) / KNOTS2NMS(1))
#define NM2FT(nm)           ((nm) * 6076.12)
#define FT2NM(ft)           ((ft) / NM2FT(1))
#define FT2FL(ft)           ((ft) / 100)
#define FL2FT(fl)           ((fl) * 100)

/*
**	sleep for milliseconds
*/
#define msleep(ms)			usleep((ms) * USEC_PER_MSEC)

/*
**	get the current time in ms
*/
#define millis()			(Poco::Timestamp().epochMicroseconds() / USEC_PER_MSEC)

/*
**  some defines to layout the track file
*/
#define TRACKS_DEFAULT      2
#define TRACKS_MIN          1
#define TRACKS_MAX          3000

/*
**  definition for the map [nautical miles, NM]
*/
#define MAP_WIDTH           500
#define MAP_HEIGHT          400
#define MAP_DEPTH           FT2NM(FL2FT(400))

#define CALLSIGN_LENGTH     (3 + 4)

// speed in knots [kn]
#define SPEED_MIN           400
#define SPEED_MAX           800

// prediction time [s]
#define PREDICTION_TIME     60

// reduce the double to this precision
#define DOUBLE_PRECISION	1000.0

extern Poco::ConsoleChannel *_channel;
extern Poco::Logger &_logger;

#define LOG_ERROR(args...)          poco_error(_logger,Poco::format(__FILE__ ":%d: ",__LINE__) + Poco::format(args,NULL))
#define LOG_WARNING(args...)        poco_warning(_logger,Poco::format(__FILE__ ":%d: ",__LINE__) + Poco::format(args,NULL))
#define LOG_NOTICE(args...)         poco_notice(_logger,Poco::format(__FILE__ ":%d: ",__LINE__) + Poco::format(args,NULL))
#define LOG_INFO(args...)           poco_information(_logger,Poco::format(__FILE__ ":%d: ",__LINE__) + Poco::format(args,NULL))

extern bool _shutdown;
extern bool _debug;

#endif
