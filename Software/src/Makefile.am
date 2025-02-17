#
#   CWPMockUp
#
#   (c) 2023 by Christian.Lorenz@gromeck.de
#
#   This file is part of CWPMockUp
#
#   CWPMockUp is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   CWPMockUp is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with CWPMockUp.  If not, see <https://www.gnu.org/licenses/>.
#
#

bin_PROGRAMS=CWPMockUp

CWPMockUp_SOURCES=\
		main.cpp \
		coordinate.cpp \
		client.cpp \
		server.cpp

AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DDATADIR=\"$(datarootdir)/$(PACKAGE)/\"

install-exec-am:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/bin
	$(install_sh) $(bin_PROGRAMS) $(DESTDIR)$(prefix)/bin
