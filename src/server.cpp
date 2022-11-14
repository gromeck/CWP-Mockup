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
#include <unistd.h>
#include <math.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/PrintHandler.h>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <iostream>
#include "common.h"
#include "server.h"

/****************************************************************************

  A I R   T R A F F I C

****************************************************************************/
static int _new_tracks = TRACKS_DEFAULT;
static int _tracks = 0;
static SERVER_TRACK _track[TRACKS_MAX];

#define RANDOM_UPPER_CHAR		((char) ('A' + random() % ('Z' - 'A' + 1)))
#define HTDOC_DIR				"/usr/share/" __TITLE__ "/htdocs/"

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
	track.last_update = millis();

	return &track;
}

/*
**	thread to simulate the air traffic
*/
static void *runServerTraffic(void *arg)
{
	while (!_shutdown) {
		int n;
		unsigned long now = millis();

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
		for (n = 0;n < _tracks;n++) {
			unsigned long delta = now - _track[n].last_update;

			if (delta > SERVER_TRACK_UPDATE_RATE) {
				/*
				**	this track has to be moved
				*/
				double distance;
				double scale;

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
				scale = (double) KNOTS2NMS(_track[n].speed) * delta / MSEC_PER_SEC / distance;
				_track[n].position = _track[n].position + (_track[n].heading - _track[n].position) * scale;

				// compute the prediction
				scale = (double) KNOTS2NMS(_track[n].speed) * PREDICTION_TIME / distance;
				_track[n].prediction = _track[n].position + (_track[n].heading - _track[n].position) * scale;

				// mark the track as updated
				_track[n].last_update = now;
			}
		}

		/*
		**	wait some time
		*/
		msleep(SERVER_TRACK_UPDATE_RATE);
	}

	LOG_NOTICE("shutting down thread runServerTraffic()");

	return NULL;
}

/****************************************************************************

 S O C K E T   C O M M U N I C A T I O N

****************************************************************************/

static int _port = 0;

class myHttpRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	virtual void handleRequest(Poco::Net::HTTPServerRequest &req, Poco::Net::HTTPServerResponse &resp)
	{
		unsigned long now = millis();

		/*
		**	create an JSON document with all the tracks
		*/
		LOG_NOTICE("HTTP: creating JSON document");
		Poco::JSON::Object::Ptr json = new Poco::JSON::Object();

		/*
		**	add the numer of tracks
		*/
		LOG_NOTICE("HTTP:JSON: adding tracks element");
		json->set("tracks", _tracks);

		/*
		**	add all tracks
		*/
		Poco::JSON::Array::Ptr trackArray = new Poco::JSON::Array();
		for (int n = 0;n < _tracks;n++) {
			LOG_NOTICE("HTTP:JSON: adding track element");
			Poco::JSON::Object::Ptr track = new Poco::JSON::Object();

			track->set("index",n);
			track->set("callsign",std::string(_track[n].callsign));

			Poco::JSON::Object::Ptr position = new Poco::JSON::Object();
			position->set("x",_track[n].position.getX());
			position->set("y",_track[n].position.getY());
			position->set("z",_track[n].position.getZ());
			track->set("position",position);

			track->set("speed",_track[n].speed);
			Poco::JSON::Object::Ptr prediction = new Poco::JSON::Object();
			prediction->set("x",_track[n].prediction.getX());
			prediction->set("y",_track[n].prediction.getY());
			prediction->set("z",_track[n].prediction.getZ());
			track->set("prediction",prediction);

			track->set("timestamp",now);
			trackArray->add(track);
		}
		json->set("track", trackArray);

		/*
		**	sending response
		*/
		LOG_NOTICE("HTTP:JSON: initializing reply");
		resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
		resp.setContentType("text/json");
		//resp.setContentLength(payload.size());
		std::ostream &out = resp.send();

		/*
		**	write the JSON to the socket
		*/
		LOG_NOTICE("HTTP: sending JSON document");
		json->stringify(out,1);

		LOG_NOTICE("HTTP: flushing output");

		out.flush();
	}
};

/*
**	handle the requests
*/
class myHttpRequestHandlerFactory: public Poco::Net::HTTPRequestHandlerFactory
{
public:
	virtual Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest &request)
	{
		// currently we will reply always with the tracklist
		LOG_NOTICE("serving request to %s",request.getURI());
    	return new myHttpRequestHandler;
  	}
};

/*
**	run the listener as an HTTP server
*/
static void *runServerCommunication(void *arg)
{
	try {
		Poco::Net::HTTPServerParams* params = new Poco::Net::HTTPServerParams;
		params->setMaxQueued(100);
		params->setMaxThreads(16);
		Poco::Net::ServerSocket socket(_port);
		Poco::Net::HTTPServer srv(new myHttpRequestHandlerFactory(), socket, params);
		srv.start();
		while (!_shutdown)
			sleep(1);
		srv.stop();
	}
	catch (Poco::Exception& exc) {
		LOG_ERROR("network exception: %s",exc.displayText());
	}

	LOG_NOTICE("shutting down server communication");
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

int runServer(int port,const char *docroot)
{
	pthread_t communicationPID;
	pthread_t trafficPID;

	_port = port;
	LOG_NOTICE("runServer: port=%d  docroot=%s",port,std::string(docroot));

	/*
	**	start a thread with the socket communication
	*/
	pthread_create(&communicationPID,NULL,runServerCommunication,(void *) docroot);

	/*
	**	start a thread with the air traffic simulation
	*/
	pthread_create(&trafficPID,NULL,runServerTraffic,NULL);

	/*
	**	run the frontend thread in foreground
	*/
	return runServerFrontend();
}/**/
