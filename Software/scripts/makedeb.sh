#!/bin/bash
#
#	Build a debian package
#
#	prerequisites:
#
#		apt install build-essential devscripts debhelper
#

#
#	error handler
#
trap 'error_handler $? $LINENO' ERR

error_handler()
{
    [ $IGNERR ] && return
    echo "ERROR: ($1) occurred on line $2"
    exit 1
}

#
#	switch to the directory in which we expect the Software directory
#
CWD=$PWD
cd $( dirname $0 )/../..
if [ ! -d Software ]; then
	echo "Can't find my Software directory -- PWD=$PWD"
	exit 1
fi

#
#	setup some configs
#
PACKAGE="cwpmockup"
VERSION="$( cat Software/VERSION )"
DEBFULLNAME="Christian Lorenz"
DEBEMAIL="gromeck@gromeck.de"
export DEBFULLNAME
export DEBEMAIL

#
#	setup a clean build directory
#
BUILDDIR="$HOME/tmp/${PACKAGE}-debbuild/"
rm -rf $BUILDDIR
mkdir -p $BUILDDIR

#
#	create the tarball
#
tar --create \
	--gzip \
	--file $BUILDDIR/${PACKAGE}_${VERSION}.orig.tar.gz \
	--transform="s/Software/${PACKAGE}-${VERSION}/" \
	Software

#
#	switch to the build dir
#
cd $BUILDDIR

#
#	unpack the tarball
#
tar xzf ${PACKAGE}_${VERSION}.orig.tar.gz

#
#	create the debian directory
#
mkdir -p ${PACKAGE}-${VERSION}/debian/source
cd ${PACKAGE}-${VERSION}
rm -f debian/changelog
debchange \
	--create \
	-v $VERSION \
	--package $PACKAGE \
	--noquery \
	"Building ${PACKAGE} as a debian package." 

echo 10 >debian/compat
cat >debian/copyright <<EOM

Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: ${PACKAGE}
Upstream-Contact: ${DEBFULLNAME} <${DEBEMAIL}>
Source: https://github.com/gromeck/CWP-Mockup

Files: *
Copyright: 2022-2023 ${DEBFULLNAME}
License: GPL-2+
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License
 along with this package; if not, see <https://www.gnu.org/licenses/>
Comment:
 On Debian systems, the full text of the GNU General Public License
 version 2 can be found in the file '/usr/share/common-licenses/GPL-2'.
EOM
echo "3.0 (native)" >debian/source/format

cat >debian/control <<EOM
Source: ${PACKAGE}
Maintainer: ${DEBFULLNAME} <${DEBEMAIL}>
Section: contrib/games
Priority: optional
Standards-Version: 4.5.1
Build-Depends: debhelper (>= 9)

Package: ${PACKAGE}
Architecture: any
Depends: \${shlibs:Depends}, \${misc:Depends}, fonts-liberation
Description: A mockup for an ATC controller working position (CWP),
 which comes with an X11 client and a web client implementation.
 It can be used as a test dummy for different display or hardware setups.
EOM

cat >debian/rules <<EOM
#!/usr/bin/make -f
%:
	dh \$@
EOM
chmod +x debian/rules

#
#	build & present the result
#
debuild -us -uc
ls -l $BUILDDIR

#
#	switch back to where we came from
#
cd $CWD
mv $BUILDDIR/${PACKAGE}_${VERSION}_*.deb .
echo "Cleaning up."
rm -rf $BUILDDIR
dpkg --info ./${PACKAGE}_${VERSION}_*.deb
