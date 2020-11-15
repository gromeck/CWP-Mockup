/*

	CWP Mockup with FLTK 1.3

	(c) 2020 by Christian.Lorenz@gromeck.de


	This file is part of CWP Mockup.

    CWP Mockup is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CWP Mockup is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CWP Mockup.  If not, see <https://www.gnu.org/licenses/>.

*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "common.h"
#include "server.h"
#include "client.h"

bool _shutdown = false;
bool _debug = false;

/*
**  the shutdown handler sets <_shutdown> to non-zero
*/
static void shutdown_handler(int sig)
{
	printf(__TITLE__ ": received signal %d (%s) -- shutting down\n",sig,strsignal(sig));
	_shutdown = true;
}

/*
**	print the usage
*/
static void usage(const char *argv0)
{
	fprintf(stderr,"Usage: %s [options]\n",argv0);
	fprintf(stderr,"Options:\n");
	fprintf(stderr," -V\n --version\n"
					"          print version number an exit\n");
	fprintf(stderr," -?\n --help\n"
					"          show this help\n");
	fprintf(stderr," -s <name or IP address>\n --server <name or IP address>\n"
					"          if this option is set, the " __TITLE__ " will run in client mode and will connect the referenced server;\n"
					"          this option is not passed, " __TITLE__ " will run in server mode\n");
	fprintf(stderr," -p <port>\n --port <port>\n"
					"          use <port> for the communication between server and client; default is %d\n",DEFAULT_PORT);
	fprintf(stderr," -d\n --debug\n"
					"          enable debug mode and print some information\n");
	fprintf(stderr," -f\n --fullscreen\n"
					"          when " __TITLE__ " runs in client mode, open the main window fullscreen\n");
	exit(-1);
}

int main(int argc,char *argv[])
{
	int c,n,m;
	char *server = NULL;
	int port = DEFAULT_PORT;
	int fullscreen = false;

	char short_options[BUFSIZ];
	struct option long_options[] = {
		{ "version",	0,	0,	'V' },
		{ "help",		0,	0,	'?' },
		{ "server",		1,	0,	's' },
		{ "port",		1,	0,	'p' },
		{ "fullscreen",	0,	0,	'f' },
		{ "debug",		0,	0,	'd' },
		{ NULL,			0,	0,	0	},
	};

	/*
	**	setup the short option string
	*/
	for (n = m = 0;long_options[n].name;n++) {
		short_options[m++] = long_options[n].val;
		if (long_options[n].has_arg)
			short_options[m++] = ':';
	}
	short_options[m++] = '\0';

	while ((c = getopt_long(argc,argv,short_options,long_options,NULL)) >= 0) {
		switch (c) {
			case 'V':	/*
						**	print version and exit
						*/
						printf("*** %s Version %s ***\n",__TITLE__,__VERSION_NR__);
						printf("(c) 2020 by Christian Lorenz\n");
						exit(0);
						break;
			case '?':	/*
						**	print the usage
						*/
						goto usage;
						break;
			case 's':	/*
						**	run in client mode
						*/
						server = strdup(optarg);
						break;
			case 'p':	/*
						**	run in testmode
						*/
						port = atoi(optarg);
						break;
			case 'f':	/*
						**	run the client in fullscreen mode
						*/
						fullscreen = true;
						break;
			case 'd':	/*
						**	print some debug
						*/
						_debug = true;
						break;
			default:
			usage:		/*
						**	usage
						*/
						fprintf(stderr,"%s: unknown option %c\n",
							__TITLE__,c);
						usage(argv[0]);
						exit(-1);
						break;
		}
	}

	/*
	**	set signal handlers
	*/
	signal(SIGHUP,SIG_IGN);
	signal(SIGINT,shutdown_handler);
	signal(SIGQUIT,shutdown_handler);
	signal(SIGABRT,shutdown_handler);
	signal(SIGTERM,shutdown_handler);
	signal(SIGUSR1,SIG_IGN);
	signal(SIGUSR2,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);
	signal(SIGTRAP,SIG_IGN);
	signal(SIGALRM,SIG_IGN);
	signal(SIGURG,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGVTALRM,SIG_IGN);

	if (_debug)
		printf("%s: running in %s mode with port %d\n",argv[0],(server) ? "client" : "server",port);

	if (server)
		runClient(server,port,fullscreen);
	else
		runServer(port);
}/**/