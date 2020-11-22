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
#include <time.h>
#include <sys/time.h>
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
static struct timeval _last_update;

/****************************************************************************

 T R A C K   H A N D L I N G

****************************************************************************/

static CLIENT_TRACK _track[TRACKS_MAX];

static bool updateTrack(int idx,CLIENT_TRACK_UPDATE *track)
{
	if (idx < 0 || idx >= TRACKS_MAX)
		return false;

	struct timeval now;
	gettimeofday(&now,NULL);

	if (_track[idx].valid) {
		struct timeval history_delta,delta_tv;

		history_delta.tv_sec = CLIENT_TRACK_HISTORY_TIME / MSEC_PER_SEC;
		history_delta.tv_usec = (CLIENT_TRACK_HISTORY_TIME - history_delta.tv_sec * MSEC_PER_SEC) / USEC_PER_MSEC;

		timersub(&now,&_track[idx].last_history_update,&delta_tv);

		if (timercmp(&delta_tv,&history_delta,>)) {
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
**	do some book-keeping
*/
static void *runClientTracking(void *arg)
{
	struct timeval now,receiving_delta,costing_delta,invalid_delta;

	receiving_delta.tv_sec = CLIENT_TRACK_COASTING_TIMEOUT / MSEC_PER_SEC;
	receiving_delta.tv_usec = (CLIENT_TRACK_COASTING_TIMEOUT - costing_delta.tv_sec * MSEC_PER_SEC) / USEC_PER_MSEC;
	costing_delta.tv_sec = CLIENT_TRACK_COASTING_TIMEOUT / MSEC_PER_SEC;
	costing_delta.tv_usec = (CLIENT_TRACK_COASTING_TIMEOUT - costing_delta.tv_sec * MSEC_PER_SEC) / USEC_PER_MSEC;
	invalid_delta.tv_sec = CLIENT_TRACK_INVALID_TIMEOUT / MSEC_PER_SEC;
	invalid_delta.tv_usec = (CLIENT_TRACK_INVALID_TIMEOUT - invalid_delta.tv_sec * MSEC_PER_SEC) / USEC_PER_MSEC;

	while (!_shutdown) {
		struct timeval delta_tv;

		gettimeofday(&now,NULL);
		timersub(&now,&_last_update,&delta_tv);
		if (timercmp(&delta_tv,&receiving_delta,>)) {
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
				timersub(&now,&_track[idx].last_update,&delta_tv);
				if (timercmp(&delta_tv,&costing_delta,>)) {
					_track[idx].coasting = true;
				}
				if (timercmp(&delta_tv,&invalid_delta,>)) {
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
		sleep(CLIENT_TRACK_UPDATE_RATE);
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

			if (_debug)
				printf("waiting for track data\n");
			if (fgets(line,sizeof(line),connectionfd)) {
				// parse the received content
				if (_debug)
					printf("fgets(): %s\n",line);
				memset(&track,0,sizeof(track));
				double posX,posY,posZ;
				double preX,preY,preZ;

				if (sscanf(line,"TRACK=%d: %7s,%lf,%lf,%lf,%d,%lf,%lf,%lf,%lu,%lu",
						&idx,
						&track.callsign,
						&posX,
						&posY,
						&posZ,
						&track.speed,
						&preX,
						&preY,
						&preZ,
						&track.timestamp.tv_sec,&track.timestamp.tv_usec) < 0) {
					if (_debug)
						printf("received invalid line: %s\n",line);
				}
				else {
					/*
					**	process this valid track
					*/
					track.position.set(posX,posY,posZ);
					track.prediction.set(preX,preY,preZ);
					if (_debug)
						printf("received: idx=%d callsign=%s position=(%6.1f/%6.1f/%6.1f) speed=%d  prediction=(%6.1f/%6.1f/%6.1f)\n",
								idx,track.callsign,
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
				if (_debug)
					perror("fgets() failed");
				break;
			}
		}

		// disconnect
		_connected = false;
		if (_debug)
			printf("shutting down connection to server\n");
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

static int _refresh_rate = CLIENT_REFRESH_RATE_DEFAULT;

class airspaceWidget: public Fl_Box
{
private:
	struct timeval last_info = { 0, 0 };
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

		//printf("handle: %s (%d)\n",fl_eventnames[event],event);

		switch (event) {
			case FL_PUSH:
				/*
				**	start panning
				*/
				printf("handle: %s(%d): x=%d  y=%d  button=%d\n",
					fl_eventnames[event],event,
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
				printf("handle: %s(%d): x=%d  y=%d\n",
					fl_eventnames[event],event,
					Fl::event_x(),Fl::event_y());
				this->screen_offset_x -= this->panning_start_x - Fl::event_x();
				this->screen_offset_y -= this->panning_start_y - Fl::event_y();
				this->panning_start_x = Fl::event_x();
				this->panning_start_y = Fl::event_y();
				this->redraw();
				rc = 1;
				break;
			case FL_MOUSEWHEEL:
				/*
				**	zoom with the mouse wheel
				*/
				printf("handle: %s(%d): x=%d  y=%d  dx=%d  dy=%d\n",
					fl_eventnames[event],event,
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

	void setMapOffsetX(int offset_x)
	{
		this->screen_offset_x = offset_x;
	}

	void setMapOffsetY(int offset_y)
	{
		this->screen_offset_y = offset_y;
	}

	int mapXToScreen(double map_x)
	{
		return (this->x() + this->screen_offset_x + (map_x * this->map_scale) / MAP_WIDTH  * this->w());
	}

	int mapWToScreen(double map_width)
	{
		return (map_width * this->map_scale) / MAP_WIDTH  * this->w();
	}

	double mapScreenToX(int scrx)
	{
		return (scrx - this->x() - this->screen_offset_x) * MAP_WIDTH / this->w() / this->map_scale;
	}

	int mapHToScreen(double map_height)
	{
		return (map_height * this->map_scale) / MAP_HEIGHT  * this->h();
	}

	int mapYToScreen(double mapy)
	{
		return (this->y() + this->screen_offset_y + (mapy * this->map_scale) / MAP_HEIGHT * this->h());
	}

	double mapScreenToY(int scry)
	{
		return (scry - this->y() - this->screen_offset_y) * MAP_HEIGHT / this->h() / this->map_scale;
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

		printf("mapZoomIn: screen=%d/%d new_screen=%d/%d  detla=%d/%d\n",
			screen_x,screen_y,
			new_screen_x,new_screen_y,
			new_screen_x - screen_x,new_screen_y - screen_y);
		this->screen_offset_x -= new_screen_x - screen_x;
		this->screen_offset_y -= new_screen_y - screen_y;
		printf("mapZoomIn: screen_offset=%d/%d\n",this->screen_offset_x,this->screen_offset_y);
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

		printf("mapZoomIn: screen=%d/%d new_screen=%d/%d  detla=%d/%d\n",
			screen_x,screen_y,
			new_screen_x,new_screen_y,
			new_screen_x - screen_x,new_screen_y - screen_y);
		this->screen_offset_x -= new_screen_x - screen_x;
		this->screen_offset_y -= new_screen_y - screen_y;
		printf("mapZoomIn: screen_offset=%d/%d\n",this->screen_offset_x,this->screen_offset_y);
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
		//printf("airspaceWidget::draw(): ...\n");

		struct timeval start_rendering;
		gettimeofday(&start_rendering,NULL);

		// set clipping to the widget area
		fl_push_clip(x(),y(),w(),h());

		// draw the background
		fl_draw_box(FL_FLAT_BOX,x(),y(),w(),h(),CLIENT_COLOR_BACKGROUND);

		// draw the map boarders
		fl_color(CLIENT_COLOR_MAP_BORDER);
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
				fl_color(CLIENT_COLOR_SYMBOL);
				fl_loop(pos_x,pos_y - CLIENT_SYMBOL_HEIGHT / 2,
					pos_x - CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2,
					pos_x + CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2);

				// draw history dots
				fl_color(CLIENT_COLOR_HISTORY);
				for (int dot = 0;dot < _track[idx].history_dots;dot++)
					fl_point(
						this->mapXToScreen(_track[idx].history[dot].getX()),
						this->mapYToScreen(_track[idx].history[dot].getY()));

				// draw red circle if another aircraft is close
				if (_track[idx].stca) {
					fl_color(CLIENT_COLOR_ALERT);
					fl_circle(pos_x,pos_y,
						this->mapWToScreen(CLIENT_STCA_DISTANCE));
				}

				if (!_track[idx].coasting) {
					// draw prediction line
					fl_color(CLIENT_COLOR_PREDICTION);
					fl_line(pos_x,pos_y,
							this->mapXToScreen(_track[idx].track.prediction.getX()),
							this->mapYToScreen(_track[idx].track.prediction.getY()));

					// draw the label
					char label[CLIENT_LABEL_LINES][50];
					int linenr = 0;
					struct timeval now;
					struct timeval delta;

					gettimeofday(&now,NULL);
					timersub(&now,&_track[idx].track.timestamp,&delta);
					unsigned long age = delta.tv_sec * MSEC_PER_SEC + delta.tv_usec / USEC_PER_MSEC;

					fl_color(CLIENT_COLOR_LABEL_LINE);
					fl_line(pos_x,pos_y,pos_x + CLIENT_LABEL_OFFSET_X,pos_y  + CLIENT_LABEL_OFFSET_Y);

					sprintf(label[linenr++],"%s",_track[idx].track.callsign);
					sprintf(label[linenr++],"%d %lu",_track[idx].track.speed,age);
					sprintf(label[linenr++],"%d",(int) FT2FL(NM2FT(_track[idx].track.position.getZ())));

					fl_color((age < CLIENT_TRACK_TOOOLD_TIMEOUT) ? CLIENT_COLOR_LABEL : CLIENT_COLOR_LABEL_OLD);
					fl_font(CLIENT_LABEL_FONTFACE,CLIENT_LABEL_FONTSIZE);
					for (linenr = 0;linenr < CLIENT_LABEL_LINES;linenr++)
						fl_draw(label[linenr],pos_x + CLIENT_LABEL_OFFSET_X,pos_y + CLIENT_LABEL_OFFSET_Y + linenr * fl_height());
				}
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
		struct timeval now,delta_tv;
		double delta,fps;

		gettimeofday(&now,NULL);

		timersub(&now,&start_rendering,&delta_tv);
		this->rendering_time += delta_tv.tv_sec + (double) delta_tv.tv_usec / USEC_PER_MSEC / MSEC_PER_SEC;

		timersub(&now,&this->last_info,&delta_tv);
		delta = delta_tv.tv_sec + (double) delta_tv.tv_usec / USEC_PER_MSEC / MSEC_PER_SEC;
		this->frames_between_info++;

		if (delta >= CLIENT_STATS_DISPPLAY_RATE) {
			fps = (delta > 0) ? (double) this->frames_between_info / delta : 0;
			sprintf(this->info," Tracks:%d  Rendering Time:%.3fms  Refresh Rate:%.1fms  FPS:%.1f ",
					tracks,this->rendering_time / this->frames_between_info * MSEC_PER_SEC,
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
		fl_draw_box(FL_FLAT_BOX,x(),y() + h() - 2 * fl_height(),width,2 * fl_height(),CLIENT_STATS_BACKGROUND);
		fl_color(CLIENT_STATS_FOREGROUND);
		fl_draw(this->info,x(),y() + h() - fl_height());

		// pop the clipping again
		fl_pop_clip();
	}
};

/*
**	the used widgets
*/
static Fl_Double_Window* window;
static Fl_Box *refreshRateLabel;
static Fl_Int_Input *refreshRateInput;
static Fl_Return_Button *setButton;
static Fl_Button *resetPanAndZoomButton;
static Fl_Box *sysinfoWidget;
static Fl_Box *clockWidget;
static airspaceWidget *airspaceDisplay;

/*
**	force the redraw of the airspace
*/
static void refreshClock(void *)
{
	static char buffer[10];
	time_t now;
	struct tm * timeinfo;

	time(&now);
	timeinfo = localtime(&now);
	sprintf(buffer,"%02d:%02d:%02d",
			timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	clockWidget->label(buffer);

	// schedule the next refresh
	Fl::add_timeout(1,refreshClock);
}

/*
**	force the redraw of the airspace
*/
static void refreshDisplay(void *)
{
	// schedule the next refresh
	Fl::add_timeout((double) _refresh_rate / 1000,refreshDisplay);
	if (_shutdown)
		window->hide();

	//	do the refresh
	airspaceDisplay->redraw();
}

/*
**	update the refresh rate on the screen
*/
static void updateRefreshRateInput(int refresh_rate)
{
	char buffer[10];

	sprintf(buffer,"%d",refresh_rate);
	refreshRateInput->value(buffer);
}

/*
**	user clicked the set button
*/
static void clickedSetButton(Fl_Widget *widget)
{
	_refresh_rate = atoi(refreshRateInput->value());

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
	airspaceDisplay->setMapOffsetX(0);
	airspaceDisplay->setMapOffsetY(0);
	airspaceDisplay->mapZoomReset();
	airspaceDisplay->redraw();
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
**	gather information about cpu cores and model name
*/
static int getCpuInfo(char **model_name)
{
	FILE *cpuinfo = fopen("/proc/cpuinfo","rb");
#define LINE_BUFFER_SIZE	1000
	char buffer[LINE_BUFFER_SIZE];
	int cores = 0;
	char *line;

	*model_name = NULL;
	while (line = fgets(buffer,sizeof(buffer),cpuinfo)) {
		//printf("cores: %d  model name=%s\n",cores,*model_name);
		//puts(line);
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
	//printf("cores: %d  model name=%s\n",cores,*model_name);

	fclose(cpuinfo);
	return cores;
}

static int handleUserEvents(int event)
{
	int rc = 0;

	printf("handleUserEvents: %s (%d)\n",fl_eventnames[event],event);

	switch (event) {
		case FL_PUSH:
			/*
			**	start panning
			*/
			printf("handleUserEvents: FL_PUSH: x=%d  y=%d\n",
				Fl::event_x(),Fl::event_y());

			break;
		case FL_DRAG:
			/*
			**	pan with mouse button held down and move
			*/
			printf("handleUserEvents: FL_DRAG: x=%d  y=%d\n",
				Fl::event_x(),Fl::event_y());

			break;
		case FL_MOUSEWHEEL:
			/*
			**	zoom with the mouse wheel
			*/
			printf("handleUserEvents: FL_MOUSEWHEEL: x=%d  y=%d  dx=%d  dy=%d\n",
				Fl::event_x(),Fl::event_y(),
				Fl::event_dx(),Fl::event_dy());
			if (Fl::event_dy() < 0)
				airspaceDisplay->mapZoomIn(Fl::event_x(),Fl::event_y());
			if (Fl::event_dy() > 0)
				airspaceDisplay->mapZoomOut(Fl::event_x(),Fl::event_y());
			rc = 1;
			break;
	}
	return rc;
}

/*
**	handle the frontend generation
*/
static int runClientFrontend(bool fullscreen)
{
	window = new Fl_Double_Window(CLIENT_WINDOW_WIDTH_MIN,CLIENT_WINDOW_HEIGHT_MIN,"Client " __TITLE__);
	window->size_range(CLIENT_WINDOW_WIDTH_MIN,CLIENT_WINDOW_HEIGHT_MIN);
	if (fullscreen)
		window->fullscreen();

	refreshRateLabel = new Fl_Box(10,15,280,30,"Refresh Rate [ms]:");
	refreshRateLabel->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);

	refreshRateInput = new Fl_Int_Input(150,10,60,30,"");
	refreshRateInput->color((Fl_Color) 55);
	refreshRateInput->maximum_size(5);
	updateRefreshRateInput(_refresh_rate);

	setButton = new Fl_Return_Button(220,10,80,30,"Set");
	setButton->callback(clickedSetButton);

	resetPanAndZoomButton = new Fl_Button(320,10,160,30,"Reset Pan && Zoom");
	resetPanAndZoomButton->callback(clickedResetPanAndZoomButton);

	char *model_name = NULL;
	int cores = getCpuInfo(&model_name);
	char hostname[HOST_NAME_MAX];
	gethostname(hostname,sizeof(hostname));

	sysinfoWidget = new Fl_Box(500,10,CLIENT_SYSINFO_WIDTH,CLIENT_SYSINFO_HEIGHT,"");
	sysinfoWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT|FL_ALIGN_TOP);
	sysinfoWidget->color(FL_BACKGROUND_COLOR);
	sysinfoWidget->box(FL_FLAT_BOX);
	sysinfoWidget->labelcolor(CLIENT_SYSINFO_COLOR);
	sysinfoWidget->labelfont(CLIENT_SYSINFO_FONTFACE);
	sysinfoWidget->labelsize(CLIENT_SYSINFO_HEIGHT);
	static char buffer[10 * HOST_NAME_MAX];
	sprintf(buffer,"%s\n%dx %s",hostname,cores,model_name);
	sysinfoWidget->label(buffer);

	clockWidget = new Fl_Box(CLIENT_WINDOW_WIDTH_MIN - CLIENT_CLOCK_WIDTH - 10,10,CLIENT_CLOCK_WIDTH,CLIENT_CLOCK_HEIGHT,"");
	clockWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER);
	clockWidget->color(FL_BACKGROUND_COLOR);
	clockWidget->box(FL_FLAT_BOX);
	clockWidget->labelcolor(CLIENT_CLOCK_COLOR);
	clockWidget->labelfont(CLIENT_CLOCK_FONTFACE);
	clockWidget->labelsize(CLIENT_CLOCK_HEIGHT);
	Fl::add_timeout(1,refreshClock);

	airspaceDisplay = new airspaceWidget(0,50,window->w(),window->h() - 40,"display");
	airspaceDisplay->color(CLIENT_COLOR_BACKGROUND);
	//Fl::add_handler(handleUserEvents);
	window->resizable(airspaceDisplay);

	Fl::add_timeout((double) _refresh_rate / 1000,refreshDisplay);

	// handle shutdown
	handleShutdown(NULL);

	window->end();
	window->show();

	return Fl::run();
}

int runClient(const char *server,int port,bool fullscreen)
{
	pthread_t communicationPID;
	pthread_t trackingPID;
	_port = port;
	_server = (server) ? strdup(server) : "localhost";
	memset(_track,0,sizeof(_track));

	if (_debug)
		printf("map: %d/%d/%d",MAP_WIDTH,MAP_HEIGHT,MAP_DEPTH);

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
