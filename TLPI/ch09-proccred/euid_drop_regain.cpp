/* euid_drop_regain.cpp

[2024-02-25] Chj: Demonstrate using seteuid(1000) to drop root privilege
and then seteuid(0) to regain root privilege.

To see the effect, you should make this exe set-UID-root.

*/
#include <unistd.h>
#include <fcntl.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

int
main(int argc, char *argv[])
{
	int err = 0;
	uid_t ruid, euid, suid;

	if (getresuid(&ruid, &euid, &suid) == -1)
		errExit("getresuid");

	printf("On program start...\n");
	print_my_PXIDs();
	printf("\n");

	printf("Enter to do seteuid(%d).   (This will drop privilege.)\n", ruid);
	getchar();
	err = seteuid(ruid);
	assert(!err);
	print_my_PXIDs();
	printf("\n");

	printf("Enter to seteuid(0).       (This will regain privilege.)\n");
	getchar();
	err = seteuid(0);
	if (err)
	{
		err = errno;
		printf("seteuid(0) returns errno: %d (%s)\n", err, strerror(err));
		printf("  -- perhaps you did not make this executable set-UID-root.\n");
	}

	print_my_PXIDs();
	printf("\n");

	printf("Enter to quit.\n");
	getchar();
	exit(0);
}
