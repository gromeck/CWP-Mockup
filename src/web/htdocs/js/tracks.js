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

var _track = false;
var _connected = false;
var _receiving = false;
var _last_track_update = 0;


/*
**	update a track
*/
function updateTrack(idx,track)
{
	if (idx < 0 || idx >= TRACKS_MAX)
		return false;

	var now = Date.now();

	if (_track[idx].valid) {
		if (now - _track[idx].last_history_update > CLIENT_TRACK_HISTORY_TIME) {
			/*
			**	shift the history dots
			*/
			for (var dot = CLIENT_HISTORY_DOTS_MAX - 1;dot > 0;dot--)
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
		_track[idx].history = new Array(CLIENT_HISTORY_DOTS_MAX);
		_track[idx].last_history_update = 0;
	}

	/*
	**	copy the data
	*/
	_track[idx].valid = true;
	_track[idx].coasting = false;
	_track[idx].track = track;
	_track[idx].last_update = now;

	_last_update = _track[idx].last_update;
	_receiving = true;
	return _track.valid;
}

function getDistance(a,b)
{
	return Math.sqrt(
			(a.x - b.x) * (a.x - b.x) +
			(a.y - b.y) * (a.y - b.y) +
			(a.z - b.z) * (a.z - b.z));
}

/*
**	do all cyclic track updates
*/
function runClientTracking()
{
	var now = Date.now();

	if (now - _last_track_update > CLIENT_TRACK_RECEIVING_TIMEOUT) {
		/*
		**	it looks like we did not receiving data
		*/
		_receiving = false;
	}

	for (var idx = 0;idx < TRACKS_MAX;idx++) {
		if (_track[idx].valid) {
			/*
			**	check for coasting/invalid because of missing updates
			*/
			if (!_track[idx].coasting && now - _track[idx].last_update > CLIENT_TRACK_COASTING_TIMEOUT) {
				_track[idx].coasting = true;
			}
			if (now - _track[idx].last_update > CLIENT_TRACK_INVALID_TIMEOUT) {
				_track[idx].valid = false;
			}

			/*
			**	check the other tracks if these are too close
			*/
			var stca = false;

			for (var idx2 = 0;idx2 < TRACKS_MAX;idx2++) {
				if (_track[idx2].valid && idx != idx2) {
					if (getDistance(_track[idx].track.position,_track[idx2].track.position) < CLIENT_STCA_DISTANCE) {
						stca = true;
						break;
					}
				}
			}
			_track[idx].stca = stca;
		}
	}

	if (0)
	for (var idx = 0;idx < TRACKS_MAX;idx++) {
		if (_track[idx].valid) {
			console.log('_track[' + idx + '] ' + _track[idx].track.callsign);
		}
	}
}

function runClientCommunication()
{
	var request = new XMLHttpRequest();
	request.onreadystatechange = function() {
			if (this.readyState == 4) {
				if (this.status == 200) {
					/*
					**	we received a track update
					*/
					_connected = true;
					var trackupdate = JSON.parse(this.responseText);
					var now = Date.now();
					console.log('runClientCommunication: tracks=' + trackupdate.track.length);
					for (var n = 0;n < trackupdate.track.length;n++) {
						updateTrack(trackupdate.track[n].index,trackupdate.track[n])
					}
					_last_track_update = Date.now();
				}
				else {
					_connected = false;
				}
			}
		};
	request.open("GET","/tracks.json",true);
	request.send();
}

/*
**	initialize the track subsystem
*/
function TracksSetup()
{
	/*
	**	initialize the track storage
	**	
	**	NOTE: don't use the .fill method on the array to initialize the array,
	**	as it would initialize all values with a *reference* to the same object
	*/
	_track = new Array(TRACKS_MAX); //.fill({valid: false});
	for (var idx = 0;idx < TRACKS_MAX && idx < _track.length;idx++)
		_track[idx] = {
				valid: false,
				coasting: false,
				track: false,
				last_update: false,
			};
	console.log('TracksSetup: TRACKS_MAX=' + _track.length);

	/*
	**	install handlers for cyclic updates
	*/
	setInterval(runClientCommunication,CLIENT_COMMUNICATION_UPDATE_RATE);
	setInterval(runClientTracking,CLIENT_TRACK_UPDATE_RATE);
}/**/
