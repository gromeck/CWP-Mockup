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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <FL/Fl.H>
#include <FL/names.h>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include "common.h"
#include "client.h"

static bool _connected = false;
static bool _receiving = false;
static Poco::Timestamp _last_update;

/****************************************************************************

 T R A C K   H A N D L I N G

****************************************************************************/

static CLIENT_TRACK _track[TRACKS_MAX];

static bool updateTrack(int idx,CLIENT_TRACK_UPDATE *track)
{
	if (idx < 0 || idx >= TRACKS_MAX)
		return false;

	Poco::Timestamp now;

	if (_track[idx].valid) {
		Poco::Timestamp::TimeDiff delta_tv;

		delta_tv = now - _track[idx].last_history_update;

		if (delta_tv > CLIENT_TRACK_HISTORY_TIME * USEC_PER_MSEC) {
			/*
			**	shift the history dots
			*/
			for (int dot = CLIENT_HISTORY_DOTS_MAX - 1;dot > 0;dot--)
				_track[idx].history[dot] = _track[idx].history[dot - 1];
			_track[idx].history[0] = _track[idx].track.position;
			if (_track[idx].history_dots < CLIENT_HISTORY_DOTS_MAX)
				_track[idx].history_dots++;
			_track[idx].last_history_update = now;
		}
	}
	else {
		/*
		**	first update for this track
		*/
		_track[idx].stca = false;
		_track[idx].history_dots = 0;
	}

	/*
	**	copy the data
	*/
	_track[idx].valid = true;
	_track[idx].coasting = false;
	_track[idx].track = *track;
	_track[idx].last_update = now;

	_last_update = _track[idx].last_update;
	_receiving = true;
	return _track->valid;
}

/*
**	find the closest track at the given position -- in any case it has to be
**	within a radius of CLIENT_SELECT_MINDISTANCE
*/
static CLIENT_TRACK *lookupTrack(double x,double y)
{
	Coordinate position = { x, y, 0 };
	double found_distance;
	int found_idx = -1;

	for (int idx = 0;idx < TRACKS_MAX;idx++) {
		if (_track[idx].valid) {
			double distance = _track[idx].track.position.getDistanceXY(position);

			if (distance < CLIENT_SELECT_MINDISTANCE && (found_idx < 0 || distance < found_distance)) {
				found_idx = idx;
				found_distance = distance;
				LOG_INFO("idx=%d  distance=%.1f",idx,distance);
			}
		}
	}
	return (found_idx >= 0) ? &_track[found_idx] : NULL;
}

/*
**	do some book-keeping
*/
static void *runClientTracking(void *arg)
{
	while (!_shutdown) {
		struct timeval delta_tv;

		Poco::Timestamp now;
		if (now - _last_update > CLIENT_TRACK_RECEIVING_TIMEOUT * USEC_PER_MSEC) {
			/*
			**	it looks like we did not receiving data
			*/
			_receiving = false;
		}

		for (int idx = 0;idx < TRACKS_MAX;idx++) {
			if (_track[idx].valid) {
				/*
				**	check for coasting/invalid because of missing updates
				*/
				if (now - _track[idx].last_update > CLIENT_TRACK_COASTING_TIMEOUT * USEC_PER_MSEC) {
					_track[idx].coasting = true;
				}
				if (now - _track[idx].last_update > CLIENT_TRACK_INVALID_TIMEOUT * USEC_PER_MSEC) {
					_track[idx].valid = false;
				}

				/*
				**	check the other tracks if these are too close
				*/
				bool stca = false;

				for (int idx2 = 0;idx2 < TRACKS_MAX;idx2++) {
					if (_track[idx2].valid && idx != idx2) {
						if (_track[idx].track.position.getDistance(_track[idx2].track.position) < CLIENT_STCA_DISTANCE) {
							stca = true;
							break;
						}
					}
				}
				_track[idx].stca = stca;
			}
		}
		usleep(CLIENT_TRACK_UPDATE_RATE * USEC_PER_MSEC);
	}
	return NULL;
}

/****************************************************************************

 S O C K E T   C O M M U N I C A T I O N

****************************************************************************/

static int _port = 0;
static const char *_server = NULL;

/*
**	receive tracks from the communication port
*/
static void *runClientCommunication(void *arg)
{
	struct sockaddr_in serv_addr;
	struct sockaddr *addr;
	struct hostent *server;
	socklen_t addr_len = sizeof(*addr);
	int sockfd = -1;
	FILE *connectionfd = NULL;
	char buffer[256];

	/*
	**	lookup the host
	*/
    if ((server = gethostbyname(_server)) == NULL) {
        perror("ERROR, no such host");
        exit(0);
    }

	while (!_shutdown) {
		/*
		**	create socket
		*/
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("ERROR opening socket");
			break;
		}

		/*
		**	Initialize socket structure
		*/
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *) server->h_addr,(char *) &serv_addr.sin_addr.s_addr,server->h_length);
		serv_addr.sin_port = htons(_port);

		/*
		**	connect to the socket
		*/
		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
			if (_debug)
				perror("ERROR on connect");
			close(sockfd);
			sockfd = -1;
		}
		else {
			/*
			**	dup the fd to use streams
			*/
			if (!(connectionfd = fdopen(sockfd,"r"))) {
				if (_debug)
					perror("ERROR on fdopen");
			}
			else
				_connected = true;
		}

		/*
		**	read the data to the client
		*/
		while (connectionfd && !_shutdown) {
			int  idx;
			CLIENT_TRACK_UPDATE track;
			char line[1024];

			LOG_INFO("waiting for track data");
			if (fgets(line,sizeof(line),connectionfd)) {
				// parse the received content
				LOG_INFO("fgets(): %s",(std::string) line);
				memset(&track,0,sizeof(track));
				double posX,posY,posZ;
				double preX,preY,preZ;
				unsigned long timestamp;

				if (sscanf(line,"TRACK=%d: %7s,%lf,%lf,%lf,%d,%lf,%lf,%lf,%lu",
						&idx,
						&track.callsign,
						&posX,
						&posY,
						&posZ,
						&track.speed,
						&preX,
						&preY,
						&preZ,
						&timestamp) < 0) {
					LOG_ERROR("received invalid line: %s",(std::string) line);
				}
				else {
					/*
					**	process this valid track
					*/
					track.position.set(posX,posY,posZ);
					track.prediction.set(preX,preY,preZ);
					track.timestamp = Poco::Timestamp(timestamp);
					LOG_INFO("received: idx=%d callsign=%s position=(%6.1f/%6.1f/%6.1f) speed=%d  prediction=(%6.1f/%6.1f/%6.1f)",
							idx,(std::string) track.callsign,
							track.position.getX(),
							track.position.getY(),
							track.position.getZ(),
							track.speed,
							track.prediction.getX(),
							track.prediction.getY(),
							track.prediction.getZ());
					updateTrack(idx,&track);
				}
			}
			else {
				LOG_ERROR("fgets() failed");
				break;
			}
		}

		// disconnect
		_connected = false;
		LOG_NOTICE("shutting down connection to server");
		if (connectionfd) {
			fclose(connectionfd);
			connectionfd = NULL;
		}
		if (sockfd > 0) {
			close(sockfd);
			sockfd = -1;
		}

		// wait before reconnect
		sleep(1);
	}
	return NULL;
}


/****************************************************************************

 F R O N T E N D

****************************************************************************/

/*
**	the used widgets
*/
static Fl_Double_Window* mainWindow;
static Fl_Box *mainWindowRefreshRateLabel;
static Fl_Int_Input *mainWindowRefreshRateInput;
static Fl_Return_Button *mainWindowSetButton;
static Fl_Button *mainWindowResetPanAndZoomButton;
static Fl_Box *mainWindowSysinfoWidget;
static Fl_Box *mainWindowClockWidget;

static Fl_Double_Window* trackInfoWindow;
static Fl_Box *trackInfoWindowCallsignLabel;
static Fl_Box *trackInfoWindowCallsignInfo;
static Fl_Box *trackInfoWindowPositionLabel;
static Fl_Box *trackInfoWindowPositionInfo;
static Fl_Box *trackInfoWindowHeightLabel;
static Fl_Box *trackInfoWindowHeightInfo;
static Fl_Box *trackInfoWindowFlightLevelLabel;
static Fl_Box *trackInfoWindowFlightLevelInfo;
static Fl_Box *trackInfoWindowSpeedLabel;
static Fl_Box *trackInfoWindowSpeedInfo;

static int _refresh_rate = CLIENT_REFRESH_RATE_DEFAULT;

/*
**	open the track info window
*/
static void openTrackInfoWindow(CLIENT_TRACK *track)
{
	if (track) {
		trackInfoWindowCallsignInfo->label(track->track.callsign);
		trackInfoWindowPositionInfo->copy_label(Poco::format("%.1f/%.1f",track->track.position.getX(),track->track.position.getY()).c_str());
		trackInfoWindowHeightInfo->copy_label(Poco::format("%.1fft",NM2FT(track->track.position.getZ())).c_str());
		trackInfoWindowFlightLevelInfo->copy_label(Poco::format("%d",(int) FT2FL(NM2FT(track->track.position.getZ()))).c_str());
		trackInfoWindowSpeedInfo->copy_label(Poco::format("%dkn",track->track.speed).c_str());
		trackInfoWindow->show();
	}
	else
		trackInfoWindow->hide();
}

class airspaceWidget: public Fl_Box
{
private:
	Poco::Timestamp last_info;
	int frames_between_info = 0;
	double rendering_time = 0;
	char info[1000];

	/*
	**	offset and scale of the map
	*/
	int screen_offset_x = 0;
	int screen_offset_y = 0;
	double map_scale = 1;
	int panning_start_x = 0;
	int panning_start_y = 0;

public:
	airspaceWidget(int x, int y, int w, int h, const char *label) : Fl_Box::Fl_Box(x, y, w, h, label)
	{
		// nothing here for now
	}

	virtual int handle(int event)
	{
		int rc = 0;
		CLIENT_TRACK *track = NULL;

		LOG_INFO("%s (%d)",(std::string) fl_eventnames[event],event);

		switch (event) {
			case FL_PUSH:
				/*
				**	start panning
				*/
				LOG_INFO("%s(%d): x=%d  y=%d  button=%d",
						(std::string) fl_eventnames[event],event,
						Fl::event_x(),Fl::event_y(),
						Fl::event_button());
				if (Fl::event_button() == 1) {
					this->panning_start_x = Fl::event_x();
					this->panning_start_y = Fl::event_y();
					rc = 1;
				}
				break;
			case FL_DRAG:
				/*
				**	pan with mouse button held down and move
				*/
				LOG_INFO("%s(%d): x=%d  y=%d",
						(std::string) fl_eventnames[event],event,
						Fl::event_x(),Fl::event_y());
				this->screen_offset_x -= this->panning_start_x - Fl::event_x();
				this->screen_offset_y -= this->panning_start_y - Fl::event_y();
				this->panning_start_x = Fl::event_x();
				this->panning_start_y = Fl::event_y();
				this->redraw();
				rc = 1;
				break;
			case FL_RELEASE:
					/*
					**	user release the button (after a press) on a target
					*/
					LOG_INFO("%s(%d): x=%d  y=%d  button=%d",
							(std::string) fl_eventnames[event],event,
							Fl::event_x(),Fl::event_y(),
							Fl::event_button());
					/*
					**	show some information about the selected track
					*/
					openTrackInfoWindow(lookupTrack(this->mapScreenToX(Fl::event_x()),this->mapScreenToY(Fl::event_y())));
					break;
			case FL_MOUSEWHEEL:
				/*
				**	zoom with the mouse wheel
				*/
				LOG_INFO("%s(%d): x=%d  y=%d  dx=%d  dy=%d",
						(std::string) fl_eventnames[event],event,
						Fl::event_x(),Fl::event_y(),
						Fl::event_dx(),Fl::event_dy());
				if (Fl::event_dy() < 0)
					this->mapZoomIn(Fl::event_x(),Fl::event_y());
				if (Fl::event_dy() > 0)
					this->mapZoomOut(Fl::event_x(),Fl::event_y());
				this->redraw();
				rc = 1;
				break;
		}
		return rc;
	}

	/*
	**	set the screen X offset
	*/
	void setScreenOffsetX(int offset_x)
	{
		this->screen_offset_x = offset_x;
	}

	/*
	**	set the screen Y offset
	*/
	void setScreenOffsetY(int offset_y)
	{
		this->screen_offset_y = offset_y;
	}

	/*
	**	compute a X coordinate on the map for the screen
	*/
	int mapXToScreen(double map_x)
	{
		return (this->x() + this->screen_offset_x + (map_x * this->map_scale) / MAP_WIDTH  * this->w());
	}

	/*
	**	compute a width on the map for the screen
	*/
	int mapWToScreen(double map_width)
	{
		return (map_width * this->map_scale) / MAP_WIDTH  * this->w();
	}

	/*
	**	compute a X coordinate on the screen for the map
	*/
	double mapScreenToX(int scrx)
	{
		return (double) (scrx - this->x() - this->screen_offset_x) * MAP_WIDTH / this->w() / this->map_scale;
	}

	/*
	**	compute a height on the map for the screen
	*/
	int mapHToScreen(double map_height)
	{
		return (map_height * this->map_scale) / MAP_HEIGHT  * this->h();
	}

	/*
	**	compute a Y coordinate on the map for the screen
	*/
	int mapYToScreen(double mapy)
	{
		return (this->y() + this->screen_offset_y + (mapy * this->map_scale) / MAP_HEIGHT * this->h());
	}

	/*
	**	compute a Y coordinate on the screen for the map
	*/
	double mapScreenToY(int scry)
	{
		return (double) (scry - this->y() - this->screen_offset_y) * MAP_HEIGHT / this->h() / this->map_scale;
	}

	/*
	**	zoom into the map
	**	keep the point where zooming happens at the same position
	*/
	void mapZoomIn(int screen_x,int screen_y)
	{
		double map_x = mapScreenToX(screen_x);
		double map_y = mapScreenToY(screen_y);

		if ((this->map_scale *= 1.0 + (double) CLIENT_MAP_ZOOM_STEP / 100.0) > CLIENT_MAP_ZOOM_MAX)
			this->map_scale = CLIENT_MAP_ZOOM_MAX;

		int new_screen_x = mapXToScreen(map_x);
		int new_screen_y = mapYToScreen(map_y);

		LOG_INFO("screen=%d/%d new_screen=%d/%d  detla=%d/%d",
				screen_x,screen_y,
				new_screen_x,new_screen_y,
				new_screen_x - screen_x,new_screen_y - screen_y);
		this->screen_offset_x -= new_screen_x - screen_x;
		this->screen_offset_y -= new_screen_y - screen_y;
		LOG_INFO("screen_offset=%d/%d",this->screen_offset_x,this->screen_offset_y);
	}

	/*
	**	zoom out of the map
	*/
	void mapZoomOut(int screen_x,int screen_y)
	{
		double map_x = mapScreenToX(screen_x);
		double map_y = mapScreenToY(screen_y);

		if ((this->map_scale /= 1.0 + (double) CLIENT_MAP_ZOOM_STEP / 100.0) < CLIENT_MAP_ZOOM_MIN)
			this->map_scale = CLIENT_MAP_ZOOM_MIN;

		int new_screen_x = mapXToScreen(map_x);
		int new_screen_y = mapYToScreen(map_y);

		LOG_INFO("mapZoomIn: screen=%d/%d new_screen=%d/%d  detla=%d/%d",
				screen_x,screen_y,
				new_screen_x,new_screen_y,
				new_screen_x - screen_x,new_screen_y - screen_y);
		this->screen_offset_x -= new_screen_x - screen_x;
		this->screen_offset_y -= new_screen_y - screen_y;
		LOG_INFO("mapZoomIn: screen_offset=%d/%d",this->screen_offset_x,this->screen_offset_y);
	}

	/*
	**	reset the zoom
	*/
	void mapZoomReset(void)
	{
		this->map_scale = 1;
	}

	void draw(void)
	{
		//LOG_INFO("airspaceWidget::draw(): ...");
		Poco::Timestamp start_rendering;

		// set clipping to the widget area
		fl_push_clip(x(),y(),w(),h());

		// draw the background
		fl_draw_box(FL_FLAT_BOX,x(),y(),w(),h(),CLIENT_COLOR_BACKGROUND);

		// draw the map boarders
		fl_color(CLIENT_COLOR_MAP_BORDER);
		fl_line_style(0);
		fl_rect(
			this->mapXToScreen(0),
			this->mapYToScreen(0),
			this->mapWToScreen(MAP_WIDTH),
			this->mapHToScreen(MAP_HEIGHT));

		int tracks = 0;
		for (int idx = 0;idx < TRACKS_MAX;idx++) {
			if (_track[idx].valid) {
				/*
				**	compute the position on the screen within the widget
				*/
				tracks++;

				int pos_x = this->mapXToScreen(_track[idx].track.position.getX());
				int pos_y = this->mapYToScreen(_track[idx].track.position.getY());

				// draw the symbol
				fl_color(CLIENT_SYMBOL_COLOR);
				fl_line_style(FL_SOLID,CLIENT_SYMBOL_THICKNESS,NULL);
				fl_loop(pos_x,pos_y - CLIENT_SYMBOL_HEIGHT / 2,
					pos_x - CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2,
					pos_x + CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2);

				// draw history dots
				fl_color(CLIENT_HISTORY_COLOR);
				for (int dot = 0;dot < _track[idx].history_dots;dot++)
					if (CLIENT_HISTORY_THICKNESS > 1)
						fl_draw_box(FL_FLAT_BOX,
							this->mapXToScreen(_track[idx].history[dot].getX()) - CLIENT_HISTORY_THICKNESS / 2,
							this->mapYToScreen(_track[idx].history[dot].getY()) - CLIENT_HISTORY_THICKNESS / 2,
							CLIENT_HISTORY_THICKNESS,CLIENT_HISTORY_THICKNESS,CLIENT_HISTORY_COLOR);
					else
						fl_point(
							this->mapXToScreen(_track[idx].history[dot].getX()),
							this->mapYToScreen(_track[idx].history[dot].getY()));

				// draw red circle if another aircraft is close
				if (_track[idx].stca) {
					fl_color(CLIENT_ALERT_LINE_COLOR);
					fl_line_style(FL_SOLID,CLIENT_ALERT_LINE_THICKNESS,NULL);
					fl_circle(pos_x,pos_y,
						this->mapWToScreen(CLIENT_STCA_DISTANCE));
				}

				if (!_track[idx].coasting) {
					// draw prediction line
					if (_track[idx].track.prediction.getX() > 0 || _track[idx].track.prediction.getY() > 0 || _track[idx].track.prediction.getX()) {
						fl_color(CLIENT_PREDICTION_LINE_COLOR);
						fl_line_style(FL_SOLID,CLIENT_PREDICTION_LINE_THICKNESS,NULL);
						fl_line(pos_x,pos_y,
								this->mapXToScreen(_track[idx].track.prediction.getX()),
								this->mapYToScreen(_track[idx].track.prediction.getY()));
					}

					// draw the label
					char label[CLIENT_LABEL_LINES][50];
					int linenr = 0;
					Poco::Timestamp now;
					unsigned long age = (now - _track[idx].track.timestamp) / USEC_PER_MSEC;

					fl_color(CLIENT_LABEL_LINE_COLOR);
					fl_line_style(FL_SOLID,CLIENT_LABEL_LINE_THICKNESS,NULL);
					fl_line(pos_x,pos_y,pos_x + CLIENT_LABEL_OFFSET_X,pos_y  + CLIENT_LABEL_OFFSET_Y);

					sprintf(label[linenr++],"%s",_track[idx].track.callsign);
					sprintf(label[linenr++],"%d %lu",_track[idx].track.speed,age);
					sprintf(label[linenr++],"%d",(int) FT2FL(NM2FT(_track[idx].track.position.getZ())));

					fl_color((age < CLIENT_TRACK_TOOOLD_TIMEOUT) ? CLIENT_LABEL_COLOR : CLIENT_LABEL_COLOR_OLD);
					fl_font(CLIENT_LABEL_FONTFACE,CLIENT_LABEL_FONTSIZE);
					for (linenr = 0;linenr < CLIENT_LABEL_LINES;linenr++)
						fl_draw(label[linenr],pos_x + CLIENT_LABEL_OFFSET_X,pos_y + CLIENT_LABEL_OFFSET_Y + linenr * fl_height());
				}

				// reset the line style
				fl_line_style(0);
			}
		}

		if (!_connected || !_receiving) {
			/*
			**	display the frozen message
			*/
			const char *message = (_connected) ? CLIENT_NODATA_MESSAGE_NODATA : CLIENT_NODATA_MESSAGE_NOCONN;
			fl_font(CLIENT_NODATA_FONTFACE,CLIENT_NODATA_FONTSIZE);
			int width = fl_width(message);
			int height = fl_height();

			fl_draw_box(FL_FLAT_BOX,
					x() + (w() - width) / 2,y(),
					width,height * 1.1,CLIENT_NODATA_BACKGROUND);
			fl_color(CLIENT_NODATA_FOREGROUND);

			fl_draw(message,
				x() + (w() - width) / 2,
				y() + height - (height - CLIENT_NODATA_FONTSIZE) / 2);
		}

		// compute some statistic information
		Poco::Timestamp now;

		this->frames_between_info++;
		this->rendering_time += (double) (now - start_rendering) / USEC_PER_SEC;
		unsigned long delta = (double) (now - this->last_info) / USEC_PER_SEC;
		
		if (delta >= CLIENT_STATS_DISPPLAY_RATE) {
			double fps = (delta > 0) ? (double) this->frames_between_info / delta : 0;

			sprintf(this->info," Tracks:%d  Rendering Time:%.3fms  Refresh Rate:%.1fms  FPS:%.1f ",
					tracks,this->rendering_time / this->frames_between_info,
					MSEC_PER_SEC / fps,fps);
			this->rendering_time = 0;
			this->frames_between_info = 0;
			this->last_info = now;
		}

		/*
		**	display the statistics
		*/
		fl_font(CLIENT_STATS_FONTFACE,CLIENT_STATS_FONTSIZE);
		int width = fl_width(this->info);
		fl_draw_box(FL_FLAT_BOX,x(),y() + h() - fl_height(),width,fl_height(),CLIENT_STATS_BACKGROUND);
		fl_color(CLIENT_STATS_FOREGROUND);
		fl_draw(this->info,x(),y() + h() - (fl_height() - CLIENT_STATS_FONTSIZE));

		// pop the clipping again
		fl_pop_clip();
	}
};

static airspaceWidget *mainWindowAirspaceDisplay;

/*
**	force the redraw of the airspace
*/
static void refreshClock(void *)
{
	time_t now;
	struct tm * timeinfo;

	time(&now);
	timeinfo = localtime(&now);
	mainWindowClockWidget->copy_label(Poco::format("%02d:%02d:%02d",
			timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec).c_str());

	// schedule the next refresh
	Fl::repeat_timeout(1,refreshClock);
}

/*
**	force the redraw of the airspace
*/
static void refreshDisplay(void *)
{
	// schedule the next refresh
	Fl::repeat_timeout((double) _refresh_rate / MSEC_PER_SEC,refreshDisplay);
	if (_shutdown) {
		trackInfoWindow->hide();
		mainWindow->hide();
	}

	//	do the refresh
	mainWindowAirspaceDisplay->redraw();
}

/*
**	update the refresh rate on the screen
*/
static void updateRefreshRateInput(int refresh_rate)
{
	mainWindowRefreshRateInput->value(Poco::format("%d",refresh_rate).c_str());
}

/*
**	user clicked the set button
*/
static void clickedSetButton(Fl_Widget *widget)
{
	_refresh_rate = atoi(mainWindowRefreshRateInput->value());

	if (_refresh_rate < CLIENT_REFRESH_RATE_MIN)
		_refresh_rate = CLIENT_REFRESH_RATE_MIN;
	if (_refresh_rate > CLIENT_REFRESH_RATE_MAX)
		_refresh_rate = CLIENT_REFRESH_RATE_MAX;
	updateRefreshRateInput(_refresh_rate);
}

/*
**	user clicked the reset button
*/
static void clickedResetPanAndZoomButton(Fl_Widget *widget)
{
	mainWindowAirspaceDisplay->setScreenOffsetX(0);
	mainWindowAirspaceDisplay->setScreenOffsetY(0);
	mainWindowAirspaceDisplay->mapZoomReset();
	mainWindowAirspaceDisplay->redraw();
}

/*
**	handle the shutdown of the frontend
*/
static void handleShutdown(void *)
{
	if (_shutdown) {
		trackInfoWindow->hide();
		mainWindow->hide();
	}

	// schedule the next refresh
	Fl::repeat_timeout(0.1,handleShutdown);
}

/*
**	gather information about cpu cores and model name
*/
static int getCpuInfo(char **model_name)
{
	FILE *cpuinfo = fopen("/proc/cpuinfo","rb");
#define BUFFER_SIZE	1000
	static char buffer[BUFFER_SIZE];
#undef BUFFER_SIZE
	int cores = 0;
	char *line;

	*model_name = NULL;
	while (line = fgets(buffer,sizeof(buffer),cpuinfo)) {
		LOG_INFO("cores: %d  model name=%s",cores,*model_name);
		LOG_INFO("%s",line);
		if (strncmp(line,"processor",9) == 0)
			++cores;
		if (strncmp(line,"model name",10) == 0 && !*model_name) {
			if (line = strchr(line,':')) {
				*model_name = strdup(line + 2);
				char *s = *model_name;
				while (*s && *s != '\n' && *s != '\r')
					s++;
				*s = '\0';
			}
		}
	}
	//LOG_INFO("cores: %d  model name=%s",cores,*model_name);

	fclose(cpuinfo);
	return cores;
}

/*
**	this callback is called upon closing the main main window
*/
static void callbackOnMainWindowClose(Fl_Widget *widget, void *)
{
	// also close the trackinfo
	trackInfoWindow->hide();
	mainWindow->hide();
}

/*
**	handle the frontend generation
*/
static int runClientFrontend(bool fullscreen)
{
	/*
	**	create the main window
	*/
	{
		int x = CLIENT_WIDGET_BORDER,y = CLIENT_WIDGET_BORDER, w = 100,h = 30;

		mainWindow = new Fl_Double_Window(CLIENT_MAIN_WINDOW_WIDTH_MIN,CLIENT_MAIN_WINDOW_HEIGHT_MIN,"Client " __TITLE__);
		mainWindow->size_range(CLIENT_MAIN_WINDOW_WIDTH_MIN,CLIENT_MAIN_WINDOW_HEIGHT_MIN);
		if (fullscreen)
			mainWindow->fullscreen();

		{
			// top banner
			Fl_Group *upperGroup = new Fl_Group(0,0,CLIENT_MAIN_WINDOW_WIDTH_MIN,CLIENT_MAIN_WINDOW_BANNER_HEIGHT);
			upperGroup->begin();

			mainWindowRefreshRateLabel = new Fl_Box(x,y,w = 140,h,"Refresh Rate [ms]:");
			mainWindowRefreshRateLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_CENTER);

			mainWindowRefreshRateInput = new Fl_Int_Input(x += w + CLIENT_WIDGET_BORDER,y,w = 60,h,"");
			mainWindowRefreshRateInput->color((Fl_Color) 55);
			mainWindowRefreshRateInput->maximum_size(5);
			updateRefreshRateInput(_refresh_rate);

			mainWindowSetButton = new Fl_Return_Button(x += w + CLIENT_WIDGET_BORDER,y,w = 80,h,"Set");
			mainWindowSetButton->callback(clickedSetButton);

			mainWindowResetPanAndZoomButton = new Fl_Button(x += w + CLIENT_WIDGET_BORDER,y,w = 160,h,"Reset Pan && Zoom");
			mainWindowResetPanAndZoomButton->callback(clickedResetPanAndZoomButton);

			char *model_name = NULL;
			int cores = getCpuInfo(&model_name);
			char hostname[HOST_NAME_MAX];
			gethostname(hostname,sizeof(hostname));

			mainWindowSysinfoWidget = new Fl_Box(x += w + CLIENT_WIDGET_BORDER,y,CLIENT_SYSINFO_WIDTH,CLIENT_SYSINFO_HEIGHT,"");
			mainWindowSysinfoWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
			mainWindowSysinfoWidget->color(FL_BACKGROUND_COLOR);
			mainWindowSysinfoWidget->box(FL_FLAT_BOX);
			mainWindowSysinfoWidget->labelcolor(CLIENT_SYSINFO_COLOR);
			mainWindowSysinfoWidget->labelfont(CLIENT_SYSINFO_FONTFACE);
			mainWindowSysinfoWidget->labelsize(CLIENT_SYSINFO_HEIGHT);
			static char buffer[10 * HOST_NAME_MAX];
			sprintf(buffer,"%s\n%dx %s",hostname,cores,model_name);
			mainWindowSysinfoWidget->label(buffer);

			mainWindowClockWidget = new Fl_Box(CLIENT_MAIN_WINDOW_WIDTH_MIN - CLIENT_CLOCK_WIDTH - CLIENT_WIDGET_BORDER,y,CLIENT_CLOCK_WIDTH,CLIENT_CLOCK_HEIGHT,"");
			mainWindowClockWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
			mainWindowClockWidget->color(FL_BACKGROUND_COLOR);
			mainWindowClockWidget->box(FL_FLAT_BOX);
			mainWindowClockWidget->labelcolor(CLIENT_CLOCK_COLOR);
			mainWindowClockWidget->labelfont(CLIENT_CLOCK_FONTFACE);
			mainWindowClockWidget->labelsize(CLIENT_CLOCK_HEIGHT);
			Fl::add_timeout(1,refreshClock);

			upperGroup->resizable(mainWindowSysinfoWidget);
			upperGroup->end();
		}

		mainWindowAirspaceDisplay = new airspaceWidget(0,CLIENT_MAIN_WINDOW_BANNER_HEIGHT,mainWindow->w(),mainWindow->h() - CLIENT_MAIN_WINDOW_BANNER_HEIGHT,"display");
		mainWindowAirspaceDisplay->color(CLIENT_COLOR_BACKGROUND);

		mainWindow->resizable(mainWindowAirspaceDisplay);
		mainWindow->end();

		// set handlers
		handleShutdown(NULL);
		Fl::add_timeout((double) _refresh_rate / MSEC_PER_SEC,refreshDisplay);
		mainWindow->callback(callbackOnMainWindowClose);
	}

	/*
	**	create the track info window
	*/
	{
		int x = CLIENT_WIDGET_BORDER,y = CLIENT_WIDGET_BORDER, w = 100,h = 30;

		trackInfoWindow = new Fl_Double_Window(CLIENT_TRACKINFO_WINDOW_WIDTH,CLIENT_TRACKINFO_WINDOW_HEIGHT,"Client " __TITLE__ " - Track Info");

		trackInfoWindowCallsignLabel = new Fl_Box(x,y,w,h,"Callsign:");
		trackInfoWindowCallsignLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
		trackInfoWindowCallsignInfo = new Fl_Box(x + w,y,w,h,"");
		trackInfoWindowCallsignInfo->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

		trackInfoWindowPositionLabel = new Fl_Box(x,y += h,w,h,"Position:");
		trackInfoWindowPositionLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
		trackInfoWindowPositionInfo = new Fl_Box(x + w,y,w,h,"");
		trackInfoWindowPositionInfo->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

		trackInfoWindowHeightLabel = new Fl_Box(x,y += h,w,h,"Height:");
		trackInfoWindowHeightLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
		trackInfoWindowHeightInfo = new Fl_Box(x + w,y,w,h,"");
		trackInfoWindowHeightInfo->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

		trackInfoWindowFlightLevelLabel = new Fl_Box(x,y += h,w,h,"Fight Level:");
		trackInfoWindowFlightLevelLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
		trackInfoWindowFlightLevelInfo = new Fl_Box(x + w,y,w,h,"");
		trackInfoWindowFlightLevelInfo->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

		trackInfoWindowSpeedLabel = new Fl_Box(x,y += h,w,h,"Speed:");
		trackInfoWindowSpeedLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
		trackInfoWindowSpeedInfo = new Fl_Box(x + w,y,w,h,"");
		trackInfoWindowSpeedInfo->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

		trackInfoWindow->end();
	}

	// go!
	mainWindow->show();

	return Fl::run();
}

int runClient(const char *server,int port,bool fullscreen)
{
	pthread_t communicationPID;
	pthread_t trackingPID;
	_port = port;
	_server = (server) ? strdup(server) : "localhost";
	memset(_track,0,sizeof(_track));

	LOG_INFO("map: %d/%d/%d",MAP_WIDTH,MAP_HEIGHT,MAP_DEPTH);

	/*
	**	start a thread with the socket communication
	*/
	pthread_create(&communicationPID,NULL,runClientCommunication,NULL);

	/*
	**	start a thread to let tracks coast
	*/
	pthread_create(&trackingPID,NULL,runClientTracking,NULL);

	/*
	**	run the frontend thread in foreground
	*/
	return runClientFrontend(fullscreen);
}/**/
