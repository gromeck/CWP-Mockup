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

var _airspace = false;

function clickedResetPanAndZoomButton()
{
	_airspace.setScreenOffsetX(0);
	_airspace.setScreenOffsetY(0);
	_airspace.mapZoomReset();
}

function resizeHandler(container,viewport)
{
	console.log('resize event');

	// this is a workaround as resizing doesn't always work in horizontal direction
	var containerobj = document.getElementById(container);
	containerobj.style.width = '100%';
	var viewportobj = document.getElementById(viewport);
	viewportobj.style.width = '100%';

	if (_airspace)
		_airspace.resizeHandler(document.getElementById(viewport));
}

function runClient(sysinfo,clock,refreshrate,viewport)
{
	/*
	**	initialize the sub systems
	*/
	SysinfoSetup(document.getElementById(sysinfo));
	ClockSetup(document.getElementById(clock));
	TracksSetup();
	_airspace = new Airspace(document.getElementById(viewport));

	/*
	**	update the refreshrate
	*/
	const refreshrateobj = document.getElementById(refreshrate);
	refreshrateobj.value = _airspace.getRefreshRate();
	refreshrateobj.addEventListener('change',function() {
			_airspace.setRefreshRate(this.value);
			let refreshrate = _airspace.getRefreshRate();
			if (this.value != refreshrate)
				refreshrateobj.value = refreshrate;
		});
}/**/
