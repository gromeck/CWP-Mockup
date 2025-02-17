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

/*
**	the airspace class
*/
class Airspace
{
	last_info = 0;
	lastframes_between_info = 0;
	lastrendering_time = 0;
	text_stats = '';

	/*
	**	offset and scale of the map
	*/
	screen_offset_x = 0;
	screen_offset_y = 0;
	map_scale = 1;
	panning_start_x = 0;
	panning_start_y = 0;

	app = 0;
	graphics = 0;

	msg_text = '-';
	msg_style = null;
	msg_obj = null;

	stats_text = 'empty';
	stats_obj = null;
	stats_style = null;

	/*
	**	timing parameters
	*/
	refresh_time = 0;
	refresh_rate = REFRESH_RATE_DEFAULT;

	/*
	**	labels & label style
	*/
	label = null;
	label_style = null;
	label_style_tooold = null;

	constructor(viewportobj)
	{
		this.resizeHandler(viewportobj);
		console.log('Airspace::constructor: viewport: +' + this.x + '+' + this.y + '-' + this.w + 'x' + this.h);

		/*
		**	create the pixi application
		*/
		this.app = new PIXI.Application({ width: this.w, height: this.h, resizeTo: viewport, backgroundColor: CLIENT_COLOR_BACKGROUND });
	//	this.app = new PIXI.Application({ resizeTo: viewportobj, backgroundColor: CLIENT_COLOR_BACKGROUND });
		viewportobj.appendChild(this.app.view);

		/*
		**	create the text styles
		*/
		this.graphics = new PIXI.Graphics();
		this.app.stage.addChild(this.graphics);

		/*
		**	create the label text style & object
		*/
		this.label_style = new PIXI.TextStyle({
				fontFamily: CLIENT_LABEL_FONTFACE,
				fontSize: CLIENT_LABEL_FONTSIZE,
				fontWeight: CLIENT_LABEL_FONTWEIGHT,
				fill: CLIENT_LABEL_COLOR,
			});
		this.label_style_old = new PIXI.TextStyle({
				fontFamily: CLIENT_LABEL_FONTFACE,
				fontSize: CLIENT_LABEL_FONTSIZE,
				fontWeight: CLIENT_LABEL_FONTWEIGHT,
				fill: CLIENT_LABEL_COLOR_OLD,
			});
		this.label = new Array(TRACKS_MAX).fill(false);

		/*
		**	create the message text style & object
		*/
		this.msg_style = new PIXI.TextStyle({
				fontFamily: CLIENT_NODATA_FONTFACE,
				fontSize: CLIENT_NODATA_FONTSIZE,
				fontWeight: CLIENT_NODATA_FONTWEIGHT,
				fill: CLIENT_NODATA_FOREGROUND,
			});
		this.msg_obj = new PIXI.Text(this.msg_text,this.msg_style);
		this.msg_obj.anchor.set(0.5,0.0);
		this.msg_obj.x = this.x + this.w / 2;
		this.msg_obj.y = this.y;
		this.app.stage.addChild(this.msg_obj);

		/*
		**	create the stats text style & object
		*/
		this.stats_style = new PIXI.TextStyle({
				fontFamily: CLIENT_STATS_FONTFACE,
				fontSize: CLIENT_STATS_FONTSIZE,
				fontWeight: CLIENT_STATS_FONTWEIGHT,
				fill: CLIENT_STATS_FOREGROUND,
			});
		this.stats_obj = new PIXI.Text(this.stats_text,this.stats_style);
		this.stats_obj.anchor.set(0.0,1.0);
		this.stats_obj.x = this.x;
		this.stats_obj.y = this.y + this.h;
		this.app.stage.addChild(this.stats_obj);

		/*
		**	install the event handler for zooming
		*/
		viewportobj.addEventListener('wheel', (event) => {
			if (event.ctrlKey == true)
				event.preventDefault();
			if (event.deltaY < 0)
				this.mapZoomIn(event.x,event.y);
			else
				this.mapZoomOut(event.x,event.y);
			this.refresh_time = 0;
		});

		/*
		**	install the event handler for panning
		*/
		viewportobj.addEventListener('mousedown', (event) => {
			if (event.buttons == 1) {
				this.panning_start_x = event.offsetX;
				this.panning_start_y = event.offsetY;
				this.refresh_time = 0;
			}
		});
		viewportobj.addEventListener('mousemove', (event) => {
			if (event.buttons == 1) {
				this.screen_offset_x -= this.panning_start_x - event.offsetX;
				this.screen_offset_y -= this.panning_start_y - event.offsetY;
				this.panning_start_x = event.offsetX;
				this.panning_start_y = event.offsetY;
				this.refresh_time = 0;
			}
		});

		console.log('Airspace::Airspace: refresh_rate:' + this.refresh_rate);
		this.app.ticker.add((delta) => {
			// Limit to the frame rate
			var now = Date.now();

			if (now - this.refresh_time >= this.refresh_rate) {
				/*
				**	time to redraw the airspace
				*/
				this.refresh_time = now;
				this.draw();
			}
		});
	}

	/*
	**	set/get the airspace rate
	*/
	getRefreshRate()
	{
		return this.refresh_rate;
	}
	setRefreshRate(refresh_rate)
	{
		refresh_rate = Math.min(refresh_rate,REFRESH_RATE_MAX);
		refresh_rate = Math.max(refresh_rate,REFRESH_RATE_MIN);
		this.refresh_rate = refresh_rate;
		console.log('Airspace::setRefreshRate: changed refresh rate to ' + this.refresh_rate);
	}

	/*
	**	the window got resized
	*/
	resizeHandler(viewportobj)
	{
		// get the current coordinates
		this.x = viewportobj.clientLeft;
		this.y = viewportobj.clientTop;
		this.w = viewportobj.clientWidth;
		this.h = viewportobj.clientHeight;
		console.log('Airspace::resizeHandler: viewpor: +' + this.x + '+' + this.y + 'x' + this.w + 'x' + this.h);

		// recenter the message
		if (this.msg_obj) {
			this.msg_obj.x = this.x + this.w / 2;
			this.msg_obj.y = this.y;
		}

		// renew the placement for the statistics
		if (this.stats_obj) {
			this.stats_obj.x = this.x;
			this.stats_obj.y = this.y + this.h;
		}
	}

	/*
	**	set the screen X offset
	*/
	setScreenOffsetX(offset_x)
	{
		this.screen_offset_x = offset_x;
	}

	/*
	**	set the screen Y offset
	*/
	setScreenOffsetY(offset_y)
	{
		this.screen_offset_y = offset_y;
	}

	/*
	**	compute a X coordinate on the map for the screen
	*/
	mapXToScreen(map_x)
	{
		return (this.x + this.screen_offset_x + (map_x * this.map_scale) / MAP_WIDTH  * this.w);
	}

	/*
	**	compute a width on the map for the screen
	*/
	mapWToScreen(map_width)
	{
		return (map_width * this.map_scale) / MAP_WIDTH  * this.w;
	}

	/*
	**	compute a X coordinate on the screen for the map
	*/
	mapScreenToX(scrx)
	{
		return (scrx - this.x - this.screen_offset_x) * MAP_WIDTH / this.w / this.map_scale;
	}

	/*
	**	compute a height on the map for the screen
	*/
	mapHToScreen(map_height)
	{
		return (map_height * this.map_scale) / MAP_HEIGHT  * this.h;
	}

	/*
	**	compute a Y coordinate on the map for the screen
	*/
	mapYToScreen(mapy)
	{
		return (this.y + this.screen_offset_y + (mapy * this.map_scale) / MAP_HEIGHT * this.h);
	}

	/*
	**	compute a Y coordinate on the screen for the map
	*/
	mapScreenToY(scry)
	{
		return (scry - this.y - this.screen_offset_y) * MAP_HEIGHT / this.h / this.map_scale;
	}

	/*
	**	zoom into the map
	**	keep the point where zooming happens at the same position
	*/
	mapZoomIn(screen_x,screen_y)
	{
		var map_x = this.mapScreenToX(screen_x);
		var map_y = this.mapScreenToY(screen_y);

		if ((this.map_scale *= 1.0 + MAP_ZOOM_STEP / 100.0) > MAP_ZOOM_MAX)
			this.map_scale = MAP_ZOOM_MAX;

		var new_screen_x = this.mapXToScreen(map_x);
		var new_screen_y = this.mapYToScreen(map_y);

		console.log('Airspace::mapZoomIn: ' +
				'screen=' + screen_x + '/' + screen_y + ' ' +
				'new_screen=' + new_screen_x + '/' + new_screen_y + ' ' +
				'delta=' + (new_screen_x - screen_x) + '/' + (new_screen_y - screen_y));
		this.screen_offset_x -= new_screen_x - screen_x;
		this.screen_offset_y -= new_screen_y - screen_y;
		console.log('Airspace::mapZoomIn: screen_offset=' + this.screen_offset_x + '/' + this.screen_offset_y);
	}

	/*
	**	zoom out of the map
	*/
	mapZoomOut(screen_x,screen_y)
	{
		var map_x = this.mapScreenToX(screen_x);
		var map_y = this.mapScreenToY(screen_y);

		if ((this.map_scale /= 1.0 + MAP_ZOOM_STEP / 100.0) < MAP_ZOOM_MIN)
			this.map_scale = MAP_ZOOM_MIN;

		var new_screen_x = this.mapXToScreen(map_x);
		var new_screen_y = this.mapYToScreen(map_y);

		console.log('Airspace::mapZoomOut: ' +
				'screen=' + screen_x + '/' + screen_y + ' ' +
				'new_screen=' + new_screen_x + '/' + new_screen_y + ' ' +
				'delta=' + (new_screen_x - screen_x) + '/' + (new_screen_y - screen_y));
		this.screen_offset_x -= new_screen_x - screen_x;
		this.screen_offset_y -= new_screen_y - screen_y;
		console.log('Airspace::mapZoomOut: screen_offset=' + this.screen_offset_x + '/' + this.screen_offset_y);
	}

	/*
	**	reset the zoom
	*/
	mapZoomReset()
	{
		this.map_scale = 1;
	}

	draw()
	{
		//LOG_INFO("airspaceWidget::draw(): ...");
		var start_rendering = Date.now();

		this.graphics.clear();

		// draw the background
		this.graphics.beginFill(CLIENT_COLOR_BACKGROUND);
		this.graphics.drawRect(this.x,this.y,this.w,this.h);
		this.graphics.endFill();

		// draw the map borders
		this.graphics.lineStyle(2,CLIENT_COLOR_MAP_BORDER);
		this.graphics.drawRect(
			this.mapXToScreen(0),
			this.mapYToScreen(0),
			this.mapWToScreen(MAP_WIDTH),
			this.mapHToScreen(MAP_HEIGHT));
		this.graphics.lineStyle(0);

		var tracks = 0;
		for (var idx = 0;idx < TRACKS_MAX && idx < _track.length;idx++) {
			if (_track[idx].valid) {
				/*
				**	compute the position on the screen within the widget
				*/
				tracks++;

				var pos_x = this.mapXToScreen(_track[idx].track.position.x);
				var pos_y = this.mapYToScreen(_track[idx].track.position.y);

				// draw the symbol
				this.graphics.lineStyle(CLIENT_SYMBOL_THICKNESS,CLIENT_SYMBOL_COLOR);
				this.graphics.drawPolygon([ pos_x,pos_y - CLIENT_SYMBOL_HEIGHT / 2,
					pos_x - CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2,
					pos_x + CLIENT_SYMBOL_WIDTH / 2,pos_y + CLIENT_SYMBOL_HEIGHT / 2]);

				// draw history dots
				this.graphics.beginFill(CLIENT_HISTORY_COLOR);
				for (let dot = 0;dot < _track[idx].history_dots;dot++)
					this.graphics.drawRect(
						this.mapXToScreen(_track[idx].history[dot].x) - CLIENT_HISTORY_THICKNESS / 2,
						this.mapYToScreen(_track[idx].history[dot].y) - CLIENT_HISTORY_THICKNESS / 2,
						CLIENT_HISTORY_THICKNESS,CLIENT_HISTORY_THICKNESS);
				this.graphics.endFill();

				// draw red circle if another aircraft is close
				if (_track[idx].stca) {
					this.graphics.lineStyle(CLIENT_ALERT_LINE_THICKNESS,CLIENT_ALERT_LINE_COLOR);
					this.graphics.drawCircle(pos_x,pos_y,this.mapWToScreen(CLIENT_STCA_DISTANCE));
					this.graphics.lineStyle(0);
				}

				if (!_track[idx].coasting) {
					// draw prediction line
					if (_track[idx].track.prediction.x > 0 || _track[idx].track.prediction.y > 0) {
						this.graphics.lineStyle(CLIENT_PREDICTION_LINE_THICKNESS,CLIENT_PREDICTION_LINE_COLOR);
						this.graphics.moveTo(pos_x,pos_y);
						this.graphics.lineTo(
								this.mapXToScreen(_track[idx].track.prediction.x),
								this.mapYToScreen(_track[idx].track.prediction.y));
					}

					var age = start_rendering - _track[idx].track.timestamp;

					this.graphics.lineStyle(CLIENT_LABEL_LINE_THICKNESS,CLIENT_LABEL_LINE_COLOR);
					this.graphics.moveTo(pos_x,pos_y);
					this.graphics.lineTo(pos_x + CLIENT_LABEL_OFFSET_X,pos_y + CLIENT_LABEL_OFFSET_Y);

					// construct the label
					if (!this.label[idx]) {
						this.label[idx] = new PIXI.Text('',this.label_style);
						this.label[idx].anchor.set(0.0,0.0);
						this.app.stage.addChild(this.label[idx]);
					}

					// draw the label
					this.label[idx].x = pos_x + CLIENT_LABEL_OFFSET_X;
					this.label[idx].y = pos_y + CLIENT_LABEL_OFFSET_Y - CLIENT_LABEL_FONTSIZE;
					this.label[idx].text = _track[idx].track.callsign + '\n' +
						_track[idx].track.speed + ' ' + age + '\n' +
						Math.floor(FT2FL(NM2FT(_track[idx].track.position.z)));
					this.label[idx].style = (age < CLIENT_COMMUNICATION_UPDATE_RATE + this.refresh_rate + CLIENT_TRACK_TOOOLD_TOLERANCE) ? this.label_style : this.label_style_old;
				}
				else {
					// destroy the label
					this.app.stage.removeChild(this.label[idx]);
					delete this.label[idx];
					this.label[idx] = false;
				}

				// reset the line style
				this.graphics.lineStyle(0);
			}
		}

		if (!_connected || !_receiving) {
			/*
			**	display the frozen/unconnected message
			*/
			this.graphics.beginFill(CLIENT_NODATA_BACKGROUND);
			this.graphics.drawRect(this.x + (this.w - CLIENT_NODATA_WIDTH) / 2,this.y,CLIENT_NODATA_WIDTH,CLIENT_NODATA_HEIGHT);
			this.graphics.endFill();
			this.msg_text = (_connected) ? CLIENT_NODATA_MESSAGE_NODATA : CLIENT_NODATA_MESSAGE_NOCONN;
		}
		else
			this.msg_text = '';
		this.msg_obj.text = this.msg_text;

		// compute some statistic information
		var now = Date.now();

		this.frames_between_info++;
		this.rendering_time += now - start_rendering;
		var delta = (now - this.last_info) / MSEC_PER_SEC;

		if (delta >= CLIENT_STATS_DISPLAY_RATE / MSEC_PER_SEC) {
			/*
			**	update the statistics
			*/
			var fps = (delta > 0) ? this.frames_between_info / delta : 0;

			this.stats_text = ' Tracks:' + tracks + '  ' +
					'Rendering Time:' + Math.floor(this.rendering_time / this.frames_between_info * 1000) / 1000 + 'ms  ' +
					'Refresh Rate:' + Math.floor(MSEC_PER_SEC / fps * 10) / 10 + 'ms  ' +
					'FPS: ' + Math.floor(fps * 10) / 10 + ' ';
			console.log('Airspace::draw: rendering_time=' + this.rendering_time + ' delta=' + delta + ' frames_between_info=' + this.frames_between_info + ' FPS=' + fps);
			this.rendering_time = 0;
			this.frames_between_info = 0;
			this.last_info = now;
		}

		/*
		**	display the statistics
		*/
		this.stats_obj.text = this.stats_text;

		let textMetrics = PIXI.TextMetrics.measureText(this.stats_text,this.stats_style)
		var stats_width = textMetrics.width;
		var stats_height = textMetrics.height;

		this.graphics.beginFill(CLIENT_STATS_BACKGROUND);
		this.graphics.drawRect(this.x,this.y + this.h - stats_height,stats_width,stats_height);
		this.graphics.endFill();
	}
}/**/
