#ifndef __strerror_fillbuf_h
#define __strerror_fillbuf_h

#include <string.h>

static void strerror_fillbuf(int errnum, char* buf, int buflen)
{
#if 0
	// _sys_nerr and _sys_errlist[] is no longer provided since glibc-2.32 .

	if (err < 0 || err >= _sys_nerr || _sys_errlist[err] == NULL) {
		snprintf(buf, buflen, "Unknown error %d", err);
	}
	else {
		strncpy(buf, _sys_errlist[err], buflen - 1);
		buf[buflen - 1] = '\0';          /* Ensure null termination */
	}
#else
	// Note: We have to use strerror_r()'s return value, bcz 
	// strerror_r(1,...) will not fill our buf[]
	snprintf(buf, buflen, "%s", strerror_r(errnum, buf, buflen));
#endif
}

#endif