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

var _clockobj = false;

/*
**	draw the clock
*/
function ClockUpdate()
{
	let date = new Date();
	let hh = date.getHours();
	let mm = date.getMinutes();
	let ss = date.getSeconds();
	hh = (hh < 10) ? '0' + hh : hh;
	mm = (mm < 10) ? '0' + mm : mm;
	ss = (ss < 10) ? '0' + ss : ss;
	_clockobj.innerHTML = hh + ':' + mm + ':' + ss;
}

function ClockSetup(clockobj)
{
	/*
	**	get the clock object
	*/
	_clockobj = clockobj;

	setInterval(ClockUpdate,MSEC_PER_SEC);
}/**/
