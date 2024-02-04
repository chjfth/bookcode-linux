#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

int
main(int argc, char* argv[])
{
	if (argc < 4)
	{
		printf("Usage:\n");
		printf("  kill2way kill   <signo> <pid>\n");
		printf("  kill2way tgkill <signo> <tgid> [tid]\n");
		printf("    If [tid] is omitted, it will be the same as tgid.\n");
		printf("Examples:\n");
		printf("  kill2way kill 14 4321\n");
		printf("  kill2way tgkill 14 4321\n");
		printf("  kill2way tgkill 14 4321 4322\n");
		return 1;
	}

	bool use_tgkill = false;
	if (strcmp(argv[1], "tgkill") == 0)
		use_tgkill = true;
	else if(strcmp(argv[1], "kill")!=0)
	{
		printf("Error: First parameter invalid.\n");
		exit(1);
	}
	
	int signo = atoi(argv[2]);
	int tgid = atoi(argv[3]);
	int tid = argc>4 ? atoi(argv[4]) : tgid;
	int& pid = tgid;

	int err = 0;
	if (use_tgkill)
	{
		printf("tgkill(%d, %d, %s)\n", tgid, tid, strsigname(signo));
		err = tgkill(tgid, tid, signo);

		if (err)
			printf("tgkill() fails, errno=%d, %s\n", errno, strerror(errno));
	}
	else
	{
		printf("kill(%d, %s)\n", pid, strsigname(signo));
		err = kill(pid, signo);
		if (err)
			printf("kill() fails, errno=%d, %s\n", errno, strerror(errno));
	}
	
	return err;
}
