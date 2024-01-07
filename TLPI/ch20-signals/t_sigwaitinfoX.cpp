/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 22-6 extended*/

/* t_sigwaitinfoX.cpp

   Demonstrate the use of sigwaitinfo() to synchronously wait for a signal.

   Chj: Verify 2x2 combination of sigwaitinfo and signal-handler.
*/
#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <time.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

static void             
siginfoHandler(int sig, siginfo_t* si, void* ucontext)
{
	(void)si; (void)ucontext;
	PrnTs("Signal %d's handler start...", sig);
	sleep(1);
	PrnTs("Signal %d's handler end.", sig);
}

#define SIG35 35
#define SIG36 36
#define SIG37 37

int
main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0)
		usageErr("%s [delay-secs]\n", argv[0]);

	printf("%s: PID is %ld\n", argv[0], (long) getpid());

	struct sigaction sa = {};
	sa.sa_sigaction = siginfoHandler;
	sa.sa_flags = SA_SIGINFO;
	sigfillset(&sa.sa_mask);
	int err = sigaction(SIG36, &sa, nullptr);
	if (err == -1)
		errExit("sigaction");
	err = sigaction(SIG37, &sa, nullptr);
	if (err == -1)
		errExit("sigaction");


	/* Block all signals (except SIG36, SIG37) */

	printf("Masking all signals, except %d, %d\n", SIG36, SIG37);
	sigset_t allsigs_mask = {};
	sigfillset(&allsigs_mask);
	sigdelset(&allsigs_mask, SIG36); 
	sigdelset(&allsigs_mask, SIG37);
	if (sigprocmask(SIG_SETMASK, &allsigs_mask, NULL) == -1)
		errExit("sigprocmask");

	printf("%s: signals blocked\n", argv[0]);

	if (argc > 1) {             /* Delay so that signals can be sent to us */
		printf("%s: about to delay %s seconds\n", argv[0], argv[1]);
		sleep(getInt(argv[1], GN_GT_0, "delay-secs"));
		printf("%s: finished delay\n", argv[0]);
	}

	sigset_t allsigs_wait = {};
	sigfillset(&allsigs_wait);
	sigdelset(&allsigs_wait, SIG35);
	sigdelset(&allsigs_wait, SIG37);

	printf("%s: Doing sigwaitinfo() on all signals, except %d, %d\n", 
		argv[0], SIG35, SIG37);
	
	for (;;) {                  /* Fetch signals until SIGINT (^C) or SIGTERM */
		siginfo_t si = {};
		int sig = sigwaitinfo(&allsigs_wait, &si);
		if (sig == -1)
			errExit("sigwaitinfo");

		if (sig == SIGINT || sig == SIGTERM)
			exit(EXIT_SUCCESS);

		PrnTs(
			"got signal: %d (%s)\n"
			"    si_signo=%d, si_code=%d (%s), si_value=%d\n"
			"    si_pid=%ld, si_uid=%ld"
			, 
			sig, strsignal(sig),

			si.si_signo, si.si_code,
			(si.si_code == SI_USER) ? "SI_USER" :
				(si.si_code == SI_QUEUE) ? "SI_QUEUE" : "other",
			si.si_value.sival_int,

			(long)si.si_pid, (long)si.si_uid
		);		
	}
}
