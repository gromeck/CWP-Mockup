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

HTDOCSDIR=$(DESTDIR)$(datarootdir)/$(PACKAGE)/htdocs

install-data-am:
	$(mkinstalldirs) $(HTDOCSDIR)
	cd $(srcdir)/htdocs ; find . -type d -exec $(install_sh) -d $(HTDOCSDIR)/"{}" ";" ; find . -type f -exec $(install_sh) -m 0644 "{}" $(HTDOCSDIR)/"{}" ";"
