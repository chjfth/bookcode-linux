/* query_fsuid.cpp

[2024-02-25] Chj: This program can be used to verify that:
On Ubuntu Linux 20.04, setfsuid(-1)'s return value exactly reflects 
the current process's file-system-user-ID(FSUID).

To see the effect, this program should be run as set-UID-root.

*/
#include <unistd.h>
#include <sys/fsuid.h> // for setfsuid()
#include <fcntl.h>
#include "tlpi_hdr.h"
#include "ugid_functions.h"

static uid_t getfsuid()
{
    return setfsuid(-1);
}

int
main(int argc, char *argv[])
{
    char sinput[40] = {};
	char* pinput = sinput;

	if (argc == 1)
	{
		printf("Input a UID for setfsuid(): ");
		fgets(sinput, sizeof(sinput), stdin);
	}
	else
		pinput = argv[1];
	
    uid_t new_fsuid = (uid_t)strtoul(pinput, nullptr, 0);
    char *pun = userNameFromId(new_fsuid);
	if(pun)
	    printf("Set FSUID to %d (%s)...\n", (int)new_fsuid, pun);
	else
		printf("Set FSUID to %d (not-existing user)...\n", (int)new_fsuid);
	
	uid_t old_fsuid = setfsuid(new_fsuid);
	printf("setfsuid(%d) returns %d (the old FSUID)\n", 
		(int)new_fsuid, (int)old_fsuid);

	printf("\n");
	printf("Query current FSUID with setfsuid(-1) ...\n");

	uid_t queried_fsuid = getfsuid();
	printf("Queried FSUID is: %d\n", (int)queried_fsuid);

	printf("\n");
	
	if(new_fsuid==queried_fsuid)
	{
		printf("Good. Set and Query FSUID success.\n");
	}
	else
	{
		printf("Bad. Set and Query FSUID do NOT match. Possible reason:\n");
		if (!pun)
			printf("The given user ID is invalid.\n");
		else
			printf("The process has no privilege to set that FSUID.\n");
	}

	if(argc == 1)
	{
		char szfile[40] = {};
		snprintf(szfile, sizeof(szfile), "%d.touch", new_fsuid);
		printf("Press Enter to create file \"%s\" and quit.", szfile);
		getchar();

		int fh = open(szfile, O_CREAT, S_IRWXU|S_IRWXG);
		if(fh<0)
		{
			int err = errno;
			printf("Create file fail. errno=%d (%s)\n", err, strerror(err));
		}
		close(fh);
	}
	
	exit(EXIT_SUCCESS);
}
