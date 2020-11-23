#
#	configure.ac
#
#	Copyright (c) 2020 by Christian Lorenz
#
#	Process this file with autoconf to produce a configure script.
#

AC_INIT([CWPMockUp], [m4_esyscmd_s([cat VERSION])])
AC_CONFIG_SRCDIR([.])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_CXX

# static
AC_ARG_ENABLE(static,
		[  --enable-static         enable static build],
			       STATIC_ENABLED="$enableval",STATIC_ENABLED=no)
AM_CONDITIONAL(STATIC_ENABLED, test "$STATIC_ENABLED" = yes)

#######################################
#
#	CFLAGS & LIBS
#
if test "$STATIC_ENABLED" = yes; then
	CFLAGS="$CFLAGS --static"
	LDFLAGS="$LDFLAGS --static"
fi
POCOLIBS="-lPocoUtil -lPocoFoundation"
CFLAGS="$CFLAGS -g -ggdb -fno-strict-aliasing -fPIC -Wall"
CFLAGS="$CFLAGS -D_GNU_SOURCE"
CFLAGS="$CFLAGS "
LDFLAGS="$LDFLAGS -g -ggdb"
LIBS="$LIBS -lfltk $POCOLIBS -lpthread"
if test "$STATIC_ENABLED" = yes; then
	LIBS="$LIBS -lXinerama -lXcursor -lxkbcommon -lXfixes -lXft -lX11-xcb -lfontconfig -lexpat -lfreetype -lpng16 -lXrender -lXext -lX11 -lxcb -lXau -lXdmcp -lm -ldl -lz -luuid -lpthread -pthread"
fi

#
#	find all Makefiles
#
AC_CONFIG_FILES([ Makefile ])
AC_OUTPUT
