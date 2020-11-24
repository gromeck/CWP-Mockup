/*

	CWPMockUp with FLTK 1.3

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
#include <stdio.h>
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
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include "common.h"
#include "server.h"

/****************************************************************************

  A I R   T R A F F I C

****************************************************************************/
static int _new_tracks = TRACKS_DEFAULT;
static int _tracks = 0;
static SERVER_TRACK _track[TRACKS_MAX];

#define RANDOM_UPPER_CHAR		('A' + random() % ('Z' - 'A' + 1))
/*
**	generate a callsign
*/
static const char *createCallsign(int idx)
{
	static char callsign[CALLSIGN_LENGTH + 1];

	snprintf(callsign,sizeof(callsign),"%c%c%c%04d",
		RANDOM_UPPER_CHAR,
		RANDOM_UPPER_CHAR,
		RANDOM_UPPER_CHAR,
		idx);
	return callsign;
}

/*
**	initialize a track
*/
static SERVER_TRACK *initilize_track(int idx)
{
	static SERVER_TRACK track;

	memset(&track,0,sizeof(track));

	/*
	**	create a call sign
	*/
	strcpy(track.callsign,createCallsign(idx));

	/*
	**	generate random position, heading and speed
	*/
	track.position.setRandom();
	track.heading.setRandom();
	track.speed = RANDOM(SPEED_MIN,SPEED_MAX);

	/*
	**	timestamp
	*/
	gettimeofday(&track.last_update,NULL);

	return &track;
}

/*
**	thread to simulate the air traffic
*/
static void *runServerTraffic(void *arg)
{
	while (!_shutdown) {
		int n;
		struct timeval now;
		struct timeval max_delta;

		/*
		**	check the number of requested tracks
		*/
		if (_new_tracks < TRACKS_MIN)
			_new_tracks = TRACKS_MIN;
		if (_new_tracks > TRACKS_MAX)
			_new_tracks = TRACKS_MAX;

		/*
		**	decrease traffic -- simply cut off the superflous tracks
		*/
		if (_new_tracks < _tracks)
			_tracks = _new_tracks;

		/*
		**	increase traffic -- initialize new tracks
		*/
		if (_new_tracks > _tracks) {
			for (;_tracks < _new_tracks;_tracks++)
				_track[_tracks] = *initilize_track(_tracks);
		}

		/*
		**	move all tracks
		*/
		gettimeofday(&now,NULL);
		max_delta.tv_sec = SERVER_TRACK_UPDATE_RATE / MSEC_PER_SEC;
		max_delta.tv_usec = (SERVER_TRACK_UPDATE_RATE - max_delta.tv_sec * MSEC_PER_SEC) / USEC_PER_MSEC;
		for (n = 0;n < _tracks;n++) {
			struct timeval delta_tv;

			timersub(&now,&_track[n].last_update,&delta_tv);
			if (timercmp(&delta_tv,&max_delta,>)) {
				/*
				**	this track has to be moved
				*/
				double distance;
				double scale;
				double delta_time = delta_tv.tv_sec + (double) delta_tv.tv_usec / USEC_PER_MSEC / MSEC_PER_SEC;

				// compute the distance to the heading position
				distance = _track[n].position.getDistance(_track[n].heading);

				if (distance < SERVER_DISTANCE_REACHED || !_track[n].position.isInsideRange()) {
					/*
					**	we have reached the headed point or we left the map, so get a new heading
					*/
					_track[n].position.wrapToRange();
					_track[n].heading.setRandom();
				}

				// compute the distance to the heading position
				distance = _track[n].position.getDistance(_track[n].heading);

				// compute the new position
				scale = (double) KNOTS2NMS(_track[n].speed) * delta_time / distance;
				_track[n].position = _track[n].position + (_track[n].heading - _track[n].position) * scale;

				// compute the prediction
				scale = (double) KNOTS2NMS(_track[n].speed) * PREDICTION_TIME / distance;
				_track[n].prediction = _track[n].position + (_track[n].heading - _track[n].position) * scale;

				// mark the track as updated
				_track[n].last_update = now;

				// print some information about the track status
#if 0
				// disabled for now as the poco lib doesn't support 15 arguments :-(
				LOG_INFO("[%04d] %s position=(%6.1f/%6.1f/%6.1f) heading=(%6.1f/%6.1f/%6.1f) @ %d distance=%6.1f prediction=(%6.1f/%6.1f/%6.1f)",
						n,
						(std::string) _track[n].callsign,
						_track[n].position.getX(),
						_track[n].position.getY(),
						_track[n].position.getZ(),
						_track[n].heading.getX(),
						_track[n].heading.getY(),
						_track[n].heading.getZ(),
						_track[n].speed,
						distance,
						_track[n].prediction.getX(),
						_track[n].prediction.getY(),
						_track[n].prediction.getZ());
#endif
			}
		}

		/*
		**	wait some time
		*/
		usleep(SERVER_TRACK_UPDATE_RATE * USEC_PER_MSEC);
	}

	LOG_NOTICE("shutting down thread runServerTraffic()");

	return NULL;
}

/****************************************************************************

 S O C K E T   C O M M U N I C A T I O N

****************************************************************************/
static int _port = 0;

/*
**	list on the given port
*/
static void *runServerCommunication(void *arg)
{
	struct sockaddr_in serv_addr;
	struct sockaddr *addr;
	socklen_t addr_len = sizeof(*addr);
	int sockfd,connection;
	FILE *connectionfd;

	/*
	**	create socket
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOG_ERROR("socket() failed");
		exit(1);
	}

	/*
	**	Initialize socket structure
	*/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(_port);

	/*
	**	bind the socket
	*/
	if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		LOG_ERROR("bind failed");
		exit(1);
	}

	/*
	**	listen on the socket
	*/
	if (listen(sockfd,5) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	while (!_shutdown) {
		/*
		**	accept connection
		*/
		LOG_INFO("waiting for incoming connection request ...");
		if ((connection = accept(sockfd,(struct sockaddr *) &addr,&addr_len)) < 0) {
			LOG_ERROR("accept() failed");
		}
		else {
			if (!(connectionfd = fdopen(connection,"w+"))) {
				LOG_ERROR("fdopen() failed");
			}
		}
		LOG_NOTICE("connection established");

		/*
		**	push the data to the client
		*/
		while (connectionfd && !_shutdown) {
			int track;

			LOG_INFO("sending track data ...");

			if (fprintf(connectionfd,"TRACKS=%d\n",_tracks) < 0)
				break;
			for (track = 0;track < _tracks;track++) {
				struct timeval now;

				gettimeofday(&now,NULL);
				if (fprintf(connectionfd,"TRACK=%d: %s,%f,%f,%f,%d,%f,%f,%f,%lu,%lu\n",
					track,
					_track[track].callsign,
					_track[track].position.getX(),
					_track[track].position.getY(),
					_track[track].position.getZ(),
					_track[track].speed,
					_track[track].prediction.getX(),
					_track[track].prediction.getY(),
					_track[track].prediction.getZ(),
					now.tv_sec,now.tv_usec) < 0)
				break;
			}
			if (track < _tracks) {
				/*
				**	we were not able to send all data
				*/
				break;
			}
			if (fflush(connectionfd) < 0)
				break;

			/*
			**	wait some time
			*/
			usleep(SERVER_COMMUNICATION_UPDATE_RATE * USEC_PER_MSEC);
		}

		/*
		**	close this connection
		*/
		LOG_NOTICE("closing connection");
		if (connectionfd) {
			fclose(connectionfd);
			connectionfd = NULL;
		}
		if (connection > 0) {
			close(connection);
			connection = -1;
		}
	}
	LOG_NOTICE("shutting down thread runServerCommunication()");
	return NULL;
}

/****************************************************************************

 F R O N T E N D

****************************************************************************/
static Fl_Double_Window* window;
static Fl_Box *numTracksLabel;
static Fl_Int_Input *numTracksInput;
static Fl_Return_Button *setButton;

static void updateNumTracksInput(int tracks)
{
	numTracksInput->value(Poco::format("%d",tracks).c_str());
}

static void clickedSetButton(Fl_Widget *widget)
{
	int tracks = atoi(numTracksInput->value());

	if (tracks < TRACKS_MIN)
		tracks = TRACKS_MIN;
	if (tracks > TRACKS_MAX)
		tracks = TRACKS_MAX;
	updateNumTracksInput(_new_tracks = tracks);
}

/*
**	handle the shutdown of the frontend
*/
static void handleShutdown(void *)
{
	if (_shutdown)
		window->hide();
	// schedule the next refresh
	Fl::add_timeout(0.1,handleShutdown);
}

/*
**	handle the frontend generation
*/
static int runServerFrontend(void)
{
	window = new Fl_Double_Window(300,120,"Server " __TITLE__);

	numTracksLabel = new Fl_Box(10,10,280,30,"Number of Tracks:");
	numTracksLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

	numTracksInput = new Fl_Int_Input(10,40,280,30,"");
	numTracksInput->color((Fl_Color) 55);
	numTracksInput->maximum_size(5);
	updateNumTracksInput(_new_tracks);

	setButton = new Fl_Return_Button(10,80,280,30,"Set");
	setButton->callback(clickedSetButton);

	// handle shutdown
	handleShutdown(NULL);

	window->end();
	window->show();

	return Fl::run();
}

int runServer(int port)
{
	pthread_t communicationPID;
	pthread_t trafficPID;

	_port = port;

	/*
	**	start a thread with the socket communication
	*/
	pthread_create(&communicationPID,NULL,runServerCommunication,NULL);

	/*
	**	start a thread with the air traffic simulation
	*/
	pthread_create(&trafficPID,NULL,runServerTraffic,NULL);

	/*
	**	run the frontend thread in foreground
	*/
	return runServerFrontend();
}/**/
