#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "PrnTs.h"

/*
 * [2023-12-31] Jimm Chen:
 * PrnTs(): A printf-like function that prints timestamp prefix as well.
 */

#ifndef __int64
#define __int64 long long
#endif


char* now_timestr(char buf[], int bufchars, bool ymd = false)
{
	if (bufchars <= 0)
		return nullptr;

	buf[0] = '\0';
	
	timeval tv_abs;
	int err = gettimeofday(&tv_abs, NULL);
	if (err)
		return nullptr;

	time_t etime = tv_abs.tv_sec;

	struct tm stm = {};
	if (!localtime_r(&etime, &stm))
		return nullptr;

	int millisec = (int)(tv_abs.tv_usec / 1000);


	buf[0] = '['; buf[1] = '\0'; buf[bufchars - 1] = '\0';

	int pfxlen = 0;
	if (ymd) {
		pfxlen = (int)strlen(buf);
		snprintf(buf+pfxlen, bufchars-pfxlen, "%04d-%02d-%02d ",
			stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday);
	}

	pfxlen = (int)strlen(buf);
	snprintf(buf+pfxlen, bufchars-pfxlen, "%02d:%02d:%02d.%03d]",
		stm.tm_hour, stm.tm_min, stm.tm_sec, millisec);

	return buf;
}

__int64 GetTickCount64()
{
	timeval tv_abs;
	gettimeofday(&tv_abs, NULL);

	return (__int64)tv_abs.tv_sec * 1000 + tv_abs.tv_usec / 1000;
}


void PrnTs(const char* fmt, ...)
{
	static __int64 s_prev_msec = GetTickCount64();

	__int64 now_msec = GetTickCount64();

	char buf[2000] = { 0 };

	// Print timestamp to show that time has elapsed for more than one second.
	int delta_msec = (int)(now_msec - s_prev_msec);
	if (delta_msec >= 1000)
	{
		printf(".\n");
	}

	char timebuf[40] = {};
	now_timestr(timebuf, ARRAYSIZE(timebuf));

	snprintf(buf, sizeof(timebuf), "%s(+%3u.%03us) ",
		timebuf,
		delta_msec / 1000, delta_msec % 1000);

	int prefixlen = (int)strlen(buf);

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf+prefixlen, ARRAYSIZE(buf)-prefixlen-1, // -1 for trailing \n
		fmt, args);

	va_end(args);

	// add trailing \n
	int slen = (int)strlen(buf);
	buf[slen] = '\n';
	buf[slen + 1] = '\0';

	printf("%s", buf);

	s_prev_msec = now_msec;
}
