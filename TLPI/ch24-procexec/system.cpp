/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 27-9 */

/* system.c

   An implementation of system(3).

   Chj: Name it fine_system() .
*/
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

int
fine_system(const char *command)
{
	sigset_t blockMask, origMask;
	struct sigaction saIgnore, saOrigQuit, saOrigInt, saDefault;
	pid_t childPid;
	int status, savedErrno;

	if (command == NULL)                // Is a shell available?          (1)
		return fine_system(":") == 0;

	/* The parent process (the caller of system()) blocks SIGCHLD
	   and ignore SIGINT and SIGQUIT while the child is executing.
	   We must change the signal settings prior to forking, to avoid
	   possible race conditions. This means that we must undo the
	   effects of the following in the child after fork(). */

	sigemptyset(&blockMask);            /* Block SIGCHLD */
	sigaddset(&blockMask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &blockMask, &origMask);                     // (2)

	saIgnore.sa_handler = SIG_IGN;      /* Ignore SIGINT and SIGQUIT */
	saIgnore.sa_flags = 0;
	sigemptyset(&saIgnore.sa_mask);
	sigaction(SIGINT, &saIgnore, &saOrigInt);                          // (3)
	sigaction(SIGQUIT, &saIgnore, &saOrigQuit);

	switch (childPid = fork()) {
	case -1: /* fork() failed */
		status = -1;
		break;          /* Carry on to reset signal attributes */

	case 0: /* Child: exec command */

		/* We ignore possible error returns because the only specified error
		   is for a failed exec(), and because errors in these calls can't
		   affect the caller of system() (which is a separate process) */

		saDefault.sa_handler = SIG_DFL;
		saDefault.sa_flags = 0;
		sigemptyset(&saDefault.sa_mask);

		if (saOrigInt.sa_handler != SIG_IGN)                           // (4)
			sigaction(SIGINT, &saDefault, NULL);
		if (saOrigQuit.sa_handler != SIG_IGN)
			sigaction(SIGQUIT, &saDefault, NULL);

		sigprocmask(SIG_SETMASK, &origMask, NULL);                     // (5)

		execl("/bin/sh", "sh", "-c", command, (char *) NULL);

		_exit(127);           // We could not exec the shell              (6)

	default: /* Parent: wait for our child to terminate */

		/* We must use waitpid() for this task; using wait() could inadvertently
		   collect the status of one of the caller's other children */

		while (waitpid(childPid, &status, 0) == -1) {                  // (7)
			if (errno != EINTR) {       /* Error other than EINTR */
				status = -1;
				break;                  /* So exit loop */
			}
		}
		break;
	}

	/* Unblock SIGCHLD, restore dispositions of SIGINT and SIGQUIT */

	savedErrno = errno;              // The following may change 'errno'  (8)

	sigprocmask(SIG_SETMASK, &origMask, NULL);                         // (9)
	sigaction(SIGINT, &saOrigInt, NULL);
	sigaction(SIGQUIT, &saOrigQuit, NULL);

	errno = savedErrno;                                                // (10)

	return status;
}


#include <stdio.h>
#include "tlpi_hdr.h"
#include "print_wait_status.h"

#define MAX_CMD_LEN 200

int
main(int argc, char* argv[])
{
	// Chj: This main() is almost the same as that of t_system.c .

	char str[MAX_CMD_LEN];      /* Command to be executed by system() */
	int status;                 /* Status return from system() */

	for (;;) {                  /* Read and execute a shell command */
		printf("Command: ");
		fflush(stdout);
		if (fgets(str, MAX_CMD_LEN, stdin) == NULL)
			break;              /* end-of-file */

		status = fine_system(str);
		printf("fine_system() returned: status=0x%04x (%d,%d)\n",
			(unsigned int)status,
			status >> 8, status & 0xff);

		if (status == -1) {
			errExit("simple_system");
		}
		else {
			if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
				printf("(Probably) could not invoke shell\n");
			else                /* Shell successfully executed command */
				printWaitStatus(NULL, status);
		}

		printf("\n");
	}

	exit(EXIT_SUCCESS);
}
