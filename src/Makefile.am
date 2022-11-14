#
#	Makefile.am
#
#	Copyright (c) 2020 by Christian Lorenz
#
#	====================================================================
#
#	This file is part of CWP-MockUp.
#
#	CWP-MockUp is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	CWP-MockUp is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with CWP-MockUp.  If not, see <http://www.gnu.org/licenses/>.
#

bin_PROGRAMS=CWPMockUp

CWPMockUp_SOURCES=main.cpp server.cpp client.cpp coordinate.cpp

install-exec-hook:
	$(mkinstalldirs) /usr/share/CWP-Mockup/htdocs/
	cp -pr $(srcdir)/web/htdocs /usr/share/CWP-Mockup/
