/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 34-7 */

/* orphaned_pgrp_SIGHUP_ts.c // Chj: Using PrnTs instead of printf

   Usage: orphaned_pgrp_SIGHUP {s|p} ...

		(e.g.: orphaned_pgrp_SIGHUP s p p)

   Creates an orphaned process group containing one process for each
   command-line argument. If the command-line argument corresponding to this
   child is 's', then the child stops itself by raising SIGSTOP.  If the
   command-line argument is 'p' then the child does a pause().

   This program can be used to show that when a process group that contains
   stopped children becomes orphaned, then all members of the process group are
   sent a SIGHUP signal, to inform them that they have been disconnected from
   their session, followed by a SIGCONT signal, to ensure that they resume
   execution. Try running the following commands and observing the difference
   in output:

		orphaned_pgrp_SIGHUP s p
		orphaned_pgrp_SIGHUP p p
*/
#define _GNU_SOURCE     /* Get declaration of strsignal() from <string.h> */
#include <string.h>
#include <signal.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

static void             /* Signal handler */
handler(int sig)
{
	PrnTs("PID=%ld: caught signal %d (%s)", (long) getpid(),      // (1)
			sig, strsignal(sig));  /* UNSAFE (see Section 21.1.2) */
}

int
main(int argc, char *argv[])
{
	int j;
	struct sigaction sa;

	if (argc < 2 || strcmp(argv[1], "--help") == 0)
		usageErr("%s {s|p} ...\n", argv[0]);

	setbuf(stdout, NULL);               /* Make stdout unbuffered */

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	if (sigaction(SIGHUP, &sa, NULL) == -1)                          // (2)
		errExit("sigaction");
	if (sigaction(SIGCONT, &sa, NULL) == -1)
		errExit("sigaction");

	PrnTs("parent: PID=%ld, PPID=%ld, PGID=%ld, SID=%ld",
			(long) getpid(), (long) getppid(),
			(long) getpgrp(), (long) getsid(0));
	printf("\n");

	/* Create one child for each command-line argument */

	for (j = 1; j < argc; j++)                                       // (3)
	{
		switch (fork()) {
		case -1:
			errExit("fork");

		case 0:         /* Child */
			PrnTs("child:  PID=%ld, PPID=%ld, PGID=%ld, SID=%ld",
					(long) getpid(), (long) getppid(),
					(long) getpgrp(), (long) getsid(0));

			if (argv[j][0] == 's') {    /* Stop via signal */
				PrnTs("PID=%ld stopping", (long) getpid());       // (4)
				printf("\n");
				raise(SIGSTOP);
			} else {                    /* Wait for signal */
				alarm(60);              /* So we die if not SIGHUPed */
				PrnTs("PID=%ld pausing", (long) getpid());
				printf("\n");
				pause();                                             // (5)
			}

			_exit(EXIT_SUCCESS);

		default:        /* Parent carries on round loop */
			break;
		}

		usleep(100 * 1000); // Chj: some delay to avoid output interleaving
	}

	/* Parent falls through to here after creating all children */

	sleep(3);                    // Give children a chance to start     (7)
	PrnTs("parent exiting");
	exit(EXIT_SUCCESS);          // And orphan them and their group     (8)
}
