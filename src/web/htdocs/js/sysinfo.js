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
**	gather some system informations & display them in the system info area
*/
function SysinfoSetup(sysinfo)
{
	var canvas = document.createElement('canvas');
	var gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
	var debugInfo = gl.getExtension('WEBGL_debug_renderer_info');

	sysinfo.innerHTML = gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL) +
			'<br>' +
			gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);
}/**/
