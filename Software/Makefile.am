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

SUBDIRS= src web

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = m4/ChangeLog

%.1.gz: %.1
	gzip <$< >$@

install-data-am: CWPMockUp.1.gz
	$(mkinstalldirs) $(DESTDIR)$(datarootdir)/man/man1
	$(install_sh) -m 0644 $< $(DESTDIR)$(datarootdir)/man/man1/
