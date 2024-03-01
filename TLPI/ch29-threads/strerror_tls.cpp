/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 31-4 */

/* strerror_tls.c

   An implementation of strerror() that is made thread-safe through
   the use of thread-local storage.

   See also strerror_tsd.c.

   Thread-local storage requires: Linux 2.6 or later, NPTL, and
   gcc 3.3 or later.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE                 /* Get '_sys_nerr' and '_sys_errlist'
									   declarations from <stdio.h> */
#endif

#include <stdio.h>
#include <string.h>                 /* Get declaration of strerror() */
#include <pthread.h>
#include "strerror_fillbuf.h"

#define MAX_ERROR_LEN 256           /* Maximum length of string in per-thread
									   buffer returned by strerror() */

static __thread char buf[MAX_ERROR_LEN];
									/* Thread-local return buffer */

char *
strerror(int err)
{
	strerror_fillbuf(err, buf, MAX_ERROR_LEN);

	return buf;
}
