/* disc_SIGHUP_ignore.cpp
 
Chj: This file differs fro disc_SIGHUP.cpp in that:
The parent process deliberately ignores SIGHUP, and we want to find out
whether STO-disconnect will kill the parent process.

The answer is: ...

*/
#define _GNU_SOURCE     /* Get strsignal() declaration from <string.h> */
#include <time.h>
#include <string.h>
#include <signal.h>
#include "tlpi_hdr.h"

static long
now_time()
{
	return (long)time(nullptr);
}

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
	printf("[%ld] PID %ld: caught signal %2d (%s)\n",                  // (1)
		now_time(),
		(long)getpid(),
		sig, strsignal(sig));
		/* UNSAFE printf (see Section 21.1.2), but OK for trivial use. */

	fflush(stdout);
}

int
main(int argc, char *argv[])
{
	printf("sizeof(long)=%d ......\n", (int)sizeof(long));
	
	pid_t parentPid = 0, childPid = 0;
	int j = 0;
	struct sigaction sa = {};

	if (argc < 2 || strcmp(argv[1], "--help") == 0)
		usageErr("%s {d|s}... [ > sig.log 2>&1 ]\n", argv[0]);

	setbuf(stdout, NULL);              /* Make stdout unbuffered */

	parentPid = getpid();
	printf("PID of parent process is:       %ld\n", (long) parentPid);
	printf("Foreground process group ID is: %ld\n",
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
		printf("[%ld] Parent process(me) now ignores SIGHUP.\n", now_time());
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
			printf("[%ld] [Note]Parent process's parent PID has changed from %ld to %ld.\n",
				now_time(), (long)old_paparent, (long)new_paparent);
		}
	}
	else
	{
		// Child processes do:
		alarm(60);      /* Ensure each process eventually terminates      (5) */

		printf("[%ld] PID=%ld PGID=%ld\n", now_time(), (long)getpid(), (long)getpgrp()); // (6)
		for (;;)
			pause();        /* Wait for signals                           (7) */
	}
}
