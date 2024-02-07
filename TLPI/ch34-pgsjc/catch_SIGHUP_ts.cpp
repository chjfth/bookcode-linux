/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 34-3 */

/* catch_SIGHUP_ts.cpp // Chj: Using PrnTs instead of printf

   Catch the SIGHUP signal and display a message.

   Usage: catch_SIGHUP [x] [ > logfile 2>&1 ]
*/
#if ! defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 500
#define _XOPEN_SOURCE 500
#endif
#include <unistd.h>
#include <signal.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

static void
handler(int sig)
{
}

int
main(int argc, char *argv[])
{
	pid_t childPid;
	struct sigaction sa;

	setbuf(stdout, NULL);       /* Make stdout unbuffered */

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	if (sigaction(SIGHUP, &sa, NULL) == -1)
		errExit("sigaction");

	childPid = fork();
	if (childPid == -1)
		errExit("fork");

	if (childPid == 0 && argc > 1)
	{
		usleep(100*1000); // chj: Child sleeps 0.1s, so that parent goes first.
		
		if (setpgid(0, 0) == -1)        /* Move to new process group */
			errExit("setpgid");
	}

	PrnTs("PID=%ld; PPID=%ld; PGID=%ld; SID=%ld", (long) getpid(),
			(long) getppid(), (long) getpgrp(), (long) getsid(0));

	alarm(60);                  /* An unhandled SIGALRM ensures this process
								   will die if nothing else terminates it */
	for (;;) {                  /* Wait for signals */
		pause();
		PrnTs("%ld: caught SIGHUP", (long) getpid());
	}
}
