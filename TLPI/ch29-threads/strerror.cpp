/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 31-1 */

/* strerror.c

   An implementation of strerror() that is not thread-safe.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE                 /* Get '_sys_nerr' and '_sys_errlist'
									   declarations from <stdio.h> */
#endif

#include <stdio.h>
#include <string.h>                 /* Get declaration of strerror() */
#include "strerror_fillbuf.h"

#define MAX_ERROR_LEN 256           /* Maximum length of string
									   returned by strerror() */

static char buf[MAX_ERROR_LEN];     /* Statically allocated return buffer */

char *
strerror(int err)
{
	strerror_fillbuf(err, buf, MAX_ERROR_LEN);
	return buf;
}
