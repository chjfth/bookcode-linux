/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 27-8 */

/* simple_system.c

   A simple implementation of system(3) that excludes signal manipulation.

   See also system.c.
*/
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "print_wait_status.h"
#include "tlpi_hdr.h"

int
simple_system(char *command)
{
	int status;
	pid_t childPid;

	switch (childPid = fork()) {
	case -1: /* Error */
		return -1;

	case 0: /* Child */
		execl("/bin/sh", "sh", "-c", command, (char *) NULL);
		_exit(127);                     /* Failed exec */

	default: /* Parent */
		if (waitpid(childPid, &status, 0) == -1)
			return -1;
		else
			return status;
	}
}

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

		status = simple_system(str);
		printf("simple_system() returned: status=0x%04x (%d,%d)\n",
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
