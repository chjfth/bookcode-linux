/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 28 */

/* demo_clone.c

   Demonstrate the use of the Linux-specific clone() system call.

   This program creates a child using clone(). Various flags can be included
   in the clone() call by specifying option letters in the first command-line
   argument to the program (see usageError() below for a list of the option
   letters). Note that not all combinations of flags are valid. See Section
   28.2.1 or the clone(2) man page about information about which flag
   combinations are required or invalid.

   This program is Linux-specific.

   [2024-01-13] Chj adds 'T' param to do CLONE_THREAD.
*/
#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include "print_wait_status.h"
#include "tlpi_hdr.h"
#include "PrnTs.h"

#ifndef CHILD_SIG
#define CHILD_SIG SIGUSR1       /* Signal to be generated on termination
								   of cloned child */
#endif

#define CHILD_TWEAKED_GVAR (-2)

typedef struct {        /* For passing info to child startup function */
	int    fd;
	mode_t umask;
	int    exitStatus;
	int    signal;

	// Chj:
	int sleep_seconds; // Child sleeps these seconds.
	int pipefd[2];     // Child uses the pipe to inform parent its(child's) exit.
} ChildParams;

static int              /* Startup function for cloned child */
childFunc(void *arg)
{
	ChildParams *cp;

	PrnTs("Child:  PID=%ld PPID=%ld", (long) getpid(), (long) getppid());

	cp = (ChildParams *) arg;   /* Cast arg to true form */

	/* The following changes may affect parent */

	umask(cp->umask);
	
	if (close(cp->fd) == -1)
		errExit("child:close");

	if (signal(cp->signal, SIG_DFL) == SIG_ERR)
		errExit("child:signal");

	if(cp->sleep_seconds > 0)
	{
		PrnTs("Child sleeps %d seconds...", cp->sleep_seconds);
		sleep(cp->sleep_seconds);
		PrnTs("Child sleeps done.");
	}

	cp->sleep_seconds = CHILD_TWEAKED_GVAR;

	char c = '0';
	write(cp->pipefd[STDOUT_FILENO], &c, 1);
	
	return cp->exitStatus;      /* Child terminates now */
}

static void             /* Handler for child termination signal */
grimReaper(int sig)
{
	int savedErrno;

	/* UNSAFE: This handler uses non-async-signal-safe functions
	   (printf(), strsignal(); see Section 21.1.2) */

	savedErrno = errno;         /* In case we change 'errno' here */

	PrnTs("Caught signal %d (%s)", sig, strsignal(sig));

	errno = savedErrno;
}

static void
usageError(char *progName)
{
	fprintf(stderr, "Usage: %s <arg> [child-sleep-seconds]\n", progName);
#define fpe(str) fprintf(stderr, "        " str)
	fpe("'arg' can contain the following letters:\n");
	fpe("    d - share file descriptors (CLONE_FILES)\n");
	fpe("    f - share file-system information (CLONE_FS)\n");
	fpe("    s - share signal dispositions (CLONE_SIGHAND)\n");
	fpe("    v - share virtual memory (CLONE_VM)\n");
	fpe("    T - a POSIX thread (CLONE_TTHREAD ... total 9 flags)\n");
	fpe(" or:\n");
	fpe("    0 - no extra flags to clone()\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	const int STACK_SIZE = 65536;       /* Stack size for cloned child */
	char *stack;                        /* Start of stack buffer area */
	char *stackTop;                     /* End of stack buffer area */
	int flags;                          /* Flags for cloning child */
	ChildParams cp = {};                /* Passed to child function */
	const mode_t START_UMASK = S_IWOTH; /* Initial umask setting */
	struct sigaction sa;
	char *p;
	int status;
	ssize_t s;
	pid_t pid;

	setbuf(stdout, NULL);
	
	/* Set up an argument structure to be passed to cloned child, and
	   set some process attributes that will be modified by child */

	cp.exitStatus = 22;                 /* Child will exit with this status */

	umask(START_UMASK);                 /* Initialize umask to some value */
	cp.umask = S_IWGRP;                 /* Child sets umask to this value */

	cp.fd = open("/dev/null", O_RDWR);  /* Child will close this fd */
	if (cp.fd == -1)
		errExit("open");

	cp.signal = SIGTERM;                /* Child will change disposition */
	if (signal(cp.signal, SIG_IGN) == SIG_ERR)  
		errExit("signal");

	if (pipe(cp.pipefd) == -1)
		errExit("pipe()");
	
	/* Initialize clone flags using command-line argument (if supplied) */

	flags = 0;
	if (argc == 1) 
		usageError(argv[0]);

	for (p = argv[1]; *p != '\0'; p++) {
		if      (*p == '0') flags = 0;
		else if (*p == 'd') flags |= CLONE_FILES;
		else if (*p == 'f') flags |= CLONE_FS;
		else if (*p == 's') flags |= CLONE_SIGHAND;
		else if (*p == 'v') flags |= CLONE_VM;
		else if (*p == 'T')
		{
			// Chj: TLPI p609 refers to this.
			flags = CLONE_THREAD | CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_SIGHAND;
//					| CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM;
		}
		else usageError(argv[0]);
	}

	if(argc > 2)
	{
		// Chj: Let child sleep for some seconds, so that
		// we can inspect in in /proc/TID/status , or attach a debugger.
		cp.sleep_seconds = getInt(argv[2], GN_NONNEG, "sleep-seconds");
	}

	/* Allocate stack for child */

	stack = (char*)mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED)
		errExit("mmap");

	stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */

	/* Establish handler to catch child termination signal */

	if (CHILD_SIG != 0) {
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = grimReaper;
		if (sigaction(CHILD_SIG, &sa, NULL) == -1)
			errExit("sigaction");
	}

	PrnTs("Parent: PID=%ld PPID=%ld", (long)getpid(), (long)getppid());

	/* Create child; child commences execution in childFunc() */

	int tid = clone(childFunc, stackTop, flags | CHILD_SIG, &cp);
	if (tid == -1)
		errExit("clone");

	PrnTs("Parent: clone() returns child-tid: %d", tid);

	/* Parent falls through to here. Wait for child; __WCLONE option is
	   required for child notifying with signal other than SIGCHLD. */

	pid = waitpid(-1, &status, (CHILD_SIG != SIGCHLD) ? __WCLONE : 0);
	if (pid == -1)
	{
		// This occurs when CLONE_THREAD. We cannot waitpid() on a thread.
		// so wait for the pipe instead.
		int c = 0;
		int err = read(cp.pipefd[STDIN_FILENO], &c, 1);
		if(err==-1)
			errExit("read() pipe");

		PrnTs("Parent: Known child's done by pipe.");
	}
	else
	{
		PrnTs("Parent: waitpid() returns pid=%ld", (long) pid);
		printWaitStatus("    Status: ", status);
	}
	/* Check whether changes made by cloned child have affected parent */

	printf("Parent - checking process attributes:\n");
	if (umask(0) != START_UMASK)
		printf("    umask has CHANGED\n");
	else
		printf("    umask has not changed\n");

	s = write(cp.fd, "Hello world\n", 12);
	if (s == -1 && errno == EBADF)
		printf("    write() on file descriptor %d FAILED, bcz FD closed by child\n", cp.fd);
	else if (s == -1)
		printf("    write() on file descriptor %d failed (%s)\n",
				cp.fd, strerror(errno));
	else
		printf("    write() on file descriptor %d succeeded\n", cp.fd);

	if (sigaction(cp.signal, NULL, &sa) == -1)
		errExit("sigaction");
	if (sa.sa_handler != SIG_IGN)
		printf("    signal disposition has CHANGED\n");
	else
		printf("    signal disposition has not changed\n");

	if (cp.sleep_seconds == CHILD_TWEAKED_GVAR)
		printf("    global var has CHANGED\n");
	else
		printf("    global var has not changed\n");

	exit(EXIT_SUCCESS);
}
