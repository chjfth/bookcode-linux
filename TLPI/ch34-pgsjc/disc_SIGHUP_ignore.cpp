/* disc_SIGHUP_ignore.cpp
 
Chj: This file differs from disc_SIGHUP.cpp in that:
The parent process deliberately ignores SIGHUP, and we want to find out
whether STO-disconnect will kill the parent process.

The answer is: The behavior is a bit complex.
* The SIGHUP is NOT lost, but "cached".
* When the parent process(me) exits(main returns 0), the cached
  SIGHUP is then delivered to other foreground processes.

By running

   exec ./disc_SIGHUP_ignore.out d s s > sig2.log 2>&1

from a telnet/ssh terminal, then close the terminal(PuTTY) window,
and after 60 seconds has passed, sig2.log has something like this:

$ cat ~/barn/x64/__Debug-remote-gcc/sig2.log
[00:53:25.772](+  0.000s) PID of parent process is:       8492
[00:53:25.772](+  0.000s) Foreground process group ID is: 8492
[00:53:25.772](+  0.000s) Parent process(me) now ignores SIGHUP.
[00:53:25.873](+  0.101s) PID=9277 PGID=8492
[00:53:25.873](+  0.101s) PID=9276 PGID=8492
[00:53:25.973](+  0.201s) PID=9275 PGID=9275
.
[00:54:15.773](+ 50.001s) [Note]Parent process's parent PID has changed from 8488 to 1.
.
[00:54:15.773](+ 49.900s) PID 9277: caught signal  1 (Hangup)
.
[00:54:15.773](+ 49.900s) PID 9276: caught signal  1 (Hangup)
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE     /* Get strsignal() declaration from <string.h> */
#endif
#include <time.h>
#include <string.h>
#include <signal.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

static void
Ignore_SIGHUP_to_me()
{
	struct sigaction sigact = {};
	sigact.sa_handler = SIG_IGN;

	int err = sigaction(SIGHUP, &sigact, nullptr);
	if (err != 0)
		errExit("Set SIG_IGN error.");
}

static void             /* Handler for SIGHUP */
handler(int sig)
{
	PrnTs("PID %ld: caught signal %2d (%s)",                  // (1)
		(long)getpid(),
		sig, strsignal(sig));
		/* UNSAFE printf (see Section 21.1.2), but OK for trivial use. */

	fflush(stdout);
}

int
main(int argc, char *argv[])
{
	pid_t parentPid = 0, childPid = 0;
	int j = 0;
	struct sigaction sa = {};

	if (argc < 2 || strcmp(argv[1], "--help") == 0)
		usageErr("%s {d|s}... [ > sig.log 2>&1 ]\n", argv[0]);

	setbuf(stdout, NULL);              /* Make stdout unbuffered */

	parentPid = getpid();
	PrnTs("PID of parent process is:       %ld", (long) parentPid);
	PrnTs("Foreground process group ID is: %ld",
			(long) tcgetpgrp(STDIN_FILENO));

	for (j = 1; j < argc; j++)         /* Create child processes      (2) */
	{
		childPid = fork();
		if (childPid == -1)
			errExit("fork");

		if (childPid == 0)             /* If child...  */
		{
			usleep(100 * 1000); // chj: Child sleeps 0.1s, so that parent goes first.

			if (argv[j][0] == 'd')     /* 'd' --> to different pgrp   (3) */
			{
				usleep(100 * 1000);    // 'd'-child sleeps more 0.1s
				if (setpgid(0, 0) == -1)
					errExit("setpgid");
			}

			sigemptyset(&sa.sa_mask);
			sa.sa_flags = 0;
			sa.sa_handler = handler;
			if (sigaction(SIGHUP, &sa, NULL) == -1) //                (4)
				errExit("sigaction");
			break;                      /* Child exits loop */
		}
	}

	// === Chj new code ===
	
	if(childPid!=0) // for parent process
	{
		Ignore_SIGHUP_to_me();
		PrnTs("Parent process(me) now ignores SIGHUP.");
	}

	/* All processes fall through to here */

	if(childPid!=0)
	{
		// Parent process do:
		
		pid_t old_paparent = getppid();
		sleep(50);

		pid_t new_paparent = getppid();

		if(new_paparent!=old_paparent)
		{
			PrnTs("[Note]Parent process's parent PID has changed from %ld to %ld.",
				(long)old_paparent, (long)new_paparent);
		}
	}
	else
	{
		// Child processes do:
		alarm(60);      /* Ensure each process eventually terminates      (5) */

		PrnTs("PID=%ld PGID=%ld", (long)getpid(), (long)getpgrp()); // (6)
		for (;;)
			pause();        /* Wait for signals                           (7) */
	}
}
