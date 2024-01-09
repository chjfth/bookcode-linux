/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 22-6 extended */

/* t_sigwaitinfoC.cpp

   Demonstrate the use of sigwaitinfo() to synchronously wait for a signal.

   Chj: Based on t_sigwaitinfo.c, this program can be used as a general
   test program that monitors signals sent to this process itself.

   * For each signal instance received, it will print timestamp prefix and
   self process ID.

   * SIGINT's info is printed as well, and then the program quits.
*/
#include <string.h>
#include <signal.h>
#include <time.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

int
main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0)
		usageErr("%s [delay-secs]\n", argv[0]);

	setbuf(stdout, nullptr);
	setbuf(stderr, nullptr);

	long mypidL = (long)getpid();
	
	PrnTs("%s: My PID is {%ld}", argv[0], mypidL);

	/* Block all signals (except SIG36, SIG37) */

	PrnTs("{%ld} Masking all signals.", mypidL);
	sigset_t allsigs = {};
	sigfillset(&allsigs);
	if (sigprocmask(SIG_SETMASK, &allsigs, NULL) == -1)
		errExit("sigprocmask");

	if (argc > 1) {             /* Delay so that signals can be sent to us */
		PrnTs("{%ld} about to delay %s seconds", mypidL, argv[1]);
		sleep(getInt(argv[1], GN_GT_0, "delay-secs"));
		PrnTs("{%ld} finished delay", mypidL);
	}

	for (;;) {                  /* Fetch signals until SIGINT (^C) or SIGTERM */
		siginfo_t si = {};
		int sig = sigwaitinfo(&allsigs, &si);
		if (sig == -1)
			errExit("sigwaitinfo");

		PrnTs(
			"{%ld} got signal: %d (%s)\n"
			"    si_signo=%d, si_code=%d (%s), si_value=%d\n"
			"    si_pid=%ld, si_uid=%ld"
			, 
			mypidL, sig, strsignal(sig),

			si.si_signo, si.si_code,
			(si.si_code == SI_USER) ? "SI_USER" :
				(si.si_code == SI_QUEUE) ? "SI_QUEUE" :
				(si.si_code == SI_KERNEL) ? "SI_KERNEL" : "other",
			si.si_value.sival_int,

			(long)si.si_pid, (long)si.si_uid
		);

		if (sig == SIGINT)
		{
			PrnTs("{%ld} Due to SIGINT, now I quit.", mypidL);
			exit(EXIT_SUCCESS);
		}
	}
}
