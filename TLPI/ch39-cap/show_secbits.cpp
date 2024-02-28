/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 39 */

/* show_secbits.c
*/
#include <linux/securebits.h>
#include <sys/prctl.h>
#include "cap_functions.h"
#include "tlpi_hdr.h"

void p812_make_me_purely_capabase() // Chj adds this
{
	int err = prctl(PR_SET_SECUREBITS,
		// SECBIT_KEEP_CAPS off 
		SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED |
		SECBIT_NOROOT | SECBIT_NOROOT_LOCKED);
	if(err)
	{
		printf("prctl(PR_SET_SECUREBITS,...) error. errno=%d (%s)\n",
			errno, strerror(errno));
		printf("Note: In order to success, you need to sudo, or:\n");
		printf("    sudo setcap \"cap_setpcap = pe\" show_secbits.out\n");
		printf("\n");
	}
	else
	{
		printf("prctl(PR_SET_SECUREBITS,...) success.\n");
	}
}

int
main(int argc, char *argv[])
{
	if (argc > 1)
		p812_make_me_purely_capabase();
	
	printf("Calling prctl(PR_GET_SECUREBITS)...\n");
	int secbits = prctl(PR_GET_SECUREBITS, 0, 0, 0, 0);
	if (secbits == -1)
		errExit("prctl");

	printf("Self-process secbits = 0x%x => ", secbits);

	printSecbits(secbits, false, stdout);
	printf("\n");

	printSecbits(secbits, true, stdout);
	printf("\n");

	if(argc>1)
	{
		printf("\nProgram done. Enter to quit.\n");
		getchar();
	}
	
	exit(EXIT_SUCCESS);
}
