/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 22 */

/* demo_SIGFPE.c

   Demonstrate the generation of the SIGFPE signal.

   Usage: demo_SIGFPE [optstr]

   The main program executes code that generates a SIGFPE signal. Before doing
   so, the program optionally ignores and/or blocks SIGFPE.

   If 'optstr' contains 'i', then SIGFPE is ignored, otherwise it is caught
   by a handler.

   If 'optstr' contains 'b', then SIGFPE is blocked before it is delivered.

   The behavior that occurs when SIGFPE is generated depends on the kernel
   version (Linux 2.6 is different from Linux 2.4 and earlier).

   NOTE: Don't compile this program with optimization, as the arithmetic
   below is likely to be optimized away completely, with the result that
   we don't get SIGFPE at all.
*/
#define _GNU_SOURCE     /* Get strsignal() declaration from <string.h> */
#include <string.h>
#include <signal.h>
#include "tlpi_hdr.h"
#include "PrnTs.h" // Chj: Use PrnTs() to output timestamp prefix.

static void
sigfpeCatcher(int sig)
{
	PrnTs("Caught signal %d (%s)", sig, strsignal(sig));
								/* UNSAFE (see Section 21.1.2) */
	sleep(1);                   /* Slow down execution of handler */
}

int
main(int argc, char *argv[])
{
	int x, y;
	sigset_t blockSet = {}, prevMask = {};
	bool blocking = false;
	struct sigaction sa = {};

	/* If no command-line arguments specified, catch SIGFPE, else ignore it */

	if (argc > 1 && strchr(argv[1], 'i') != NULL) 
	{
		printf("Ignoring SIGFPE\n");
		
		if (signal(SIGFPE, SIG_IGN) == SIG_ERR)
			errExit("signal");
	}
	else 
	{
		printf("Catching SIGFPE\n");
		
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = sigfpeCatcher;
		if (sigaction(SIGFPE, &sa, NULL) == -1)
			errExit("sigaction");
	}

	blocking = argc > 1 && strchr(argv[1], 'b') != NULL;
	if (blocking) 
	{
		printf("Blocking SIGFPE\n");
		
		sigemptyset(&blockSet);
		sigaddset(&blockSet, SIGFPE);
		if (sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
			errExit("sigprocmask");
	}

	PrnTs("About to generate SIGFPE");
	y = 0;
	x = 1 / y;
	y = x;      /* Avoid complaints from "gcc -Wunused-but-set-variable" */

	if (blocking) 
	{
		PrnTs("Sleeping 2 sec before unblocking");

		sleep(2);

		PrnTs("Unblocking SIGFPE");

		if (sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1)
			errExit("sigprocmask");
	}

	printf("PANIC! Shouldn't get here!\n");
	exit(EXIT_FAILURE);
}
