#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <pthread.h>
#include <sys/fsuid.h> // setfsuid()
#include "ugid_functions.h"

#include "PrnTs.h"

/*
 * [2023-12-31] Jimm Chen:
 * PrnTs(): A printf-like function that prints timestamp prefix as well.
 */

#ifndef __int64
#define __int64 long long
#endif

static pthread_mutex_t s_prnts_mtx = PTHREAD_MUTEX_INITIALIZER;


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
	int err = pthread_mutex_lock(&s_prnts_mtx);
	if (err != 0)
		abort();

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

	int prefixlen = (int)strlen(timebuf);
	
	snprintf(buf+prefixlen, sizeof(timebuf)-prefixlen, "(+%3u.%03us) ",
		delta_msec / 1000, delta_msec % 1000);

	prefixlen = (int)strlen(buf);

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

	err = pthread_mutex_unlock(&s_prnts_mtx);
	if (err != 0)
		abort();
}

#define MPAIR(sig) { sig, #sig }

#define SIGRTMIN1 (SIGRTMIN+1)
#define SIGRTMIN2 (SIGRTMIN+2)
#define SIGRTMIN3 (SIGRTMIN+3)
#define SIGRTMIN4 (SIGRTMIN+4)

const char* strsigname(int signo)
{
	struct sigmap_st
	{
		int signo;
		const char* name;
	};
	static sigmap_st s_map[] =
	{
		MPAIR(SIGHUP),
		MPAIR(SIGINT),
		MPAIR(SIGQUIT),
		MPAIR(SIGILL),
		MPAIR(SIGTRAP),
		MPAIR(SIGABRT),
		MPAIR(SIGBUS),
		MPAIR(SIGFPE),
		MPAIR(SIGKILL),
		MPAIR(SIGUSR1),
		MPAIR(SIGSEGV),
		MPAIR(SIGUSR2),
		MPAIR(SIGPIPE),
		MPAIR(SIGALRM),
		MPAIR(SIGTERM),
		MPAIR(SIGSTKFLT),
		MPAIR(SIGCHLD),
		MPAIR(SIGCONT),
		MPAIR(SIGSTOP),
		MPAIR(SIGTSTP),
		MPAIR(SIGTTIN),
		MPAIR(SIGTTOU),
		MPAIR(SIGURG),
		MPAIR(SIGXCPU),
		MPAIR(SIGXFSZ),
		MPAIR(SIGVTALRM),
		MPAIR(SIGPROF),
		MPAIR(SIGWINCH),
		MPAIR(SIGIO),
		MPAIR(SIGPWR),
		MPAIR(SIGSYS),
		MPAIR(SIGRTMIN),
		MPAIR(SIGRTMIN1),
		MPAIR(SIGRTMIN2),
		MPAIR(SIGRTMIN3),
		MPAIR(SIGRTMIN4),
	};

	for(int i=0; i<(int)ARRAYSIZE(s_map); i++)
	{
		if (s_map[i].signo == signo)
			return s_map[i].name;
	}

	return nullptr;
}

static uid_t getfsuid()
{
	return setfsuid(-1); // Ubuntu 20.04 ok
}

static uid_t getfsgid()
{
	return setfsgid(-1);
}

void print_my_PXIDs()
{
	uid_t ruid, euid, suid, fsuid;
	gid_t rgid, egid, sgid, fsgid;
	char* p;

	if (getresuid(&ruid, &euid, &suid) == -1)
		errExit("getresuid");
	if (getresgid(&rgid, &egid, &sgid) == -1)
		errExit("getresgid");

	fsuid = getfsuid();
	fsgid = getfsgid();

	p = userNameFromId(ruid);
	printf("RUID=%s (%ld); ", (p == NULL) ? "???" : p, (long)ruid);
	p = userNameFromId(euid);
	printf("EUID=%s (%ld); ", (p == NULL) ? "???" : p, (long)euid);
	p = userNameFromId(suid);
	printf("SSUID=%s (%ld); ", (p == NULL) ? "???" : p, (long)suid);
	p = userNameFromId(fsuid);
	printf("FSUID=%s (%ld); ", (p == NULL) ? "???" : p, (long)fsuid);
	printf("\n");

	p = groupNameFromId(rgid);
	printf("RGID=%s (%ld); ", (p == NULL) ? "???" : p, (long)rgid);
	p = groupNameFromId(egid);
	printf("EGID=%s (%ld); ", (p == NULL) ? "???" : p, (long)egid);
	p = groupNameFromId(sgid);
	printf("SSGID=%s (%ld); ", (p == NULL) ? "???" : p, (long)sgid);
	p = groupNameFromId(fsgid);
	printf("FSGID=%s (%ld); ", (p == NULL) ? "???" : p, (long)fsgid);
	printf("\n");
}

