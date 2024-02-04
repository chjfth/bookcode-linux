/*
 * [2024-02-04] Chj: The program to demonstrate receiving "a process-wide signal",
 * vs receiving a "thread-specific" signal.
 * To do a process-wide sending, sender should call kill().
 * to do a thread-specific sending, sender should call tgkill().
 * There is a side-by-side kill2way.cpp to do these two kills.
 */
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "tlpi_hdr.h"
#include "PrnTs.h"

pid_t g_childtid = 0;

static void*
threadFunc(void* arg)
{
    (void)arg;
    g_childtid = gettid();
    PrnTs("Child thread: PID=%ld, TID=%ld", (long)getpid(), (long)g_childtid);

	for(;;)
		pause();

    PrnTs("Child thread: Will return.");
	return nullptr;
}

void
sighandler(int sig)
{
    PrnTs("In signal handler: TID=%ld", (long)gettid());
}

void  print_help()
{
    printf(
        "This program installs a signal-handler in main(), for a specific signal,\n"
        "and creates a child thread. The signal-handler prints gettid() value,\n"
        "so that we can know which thread is delivered that signal.\n"
        "\n"
        "You need to assign a parameter to tell which signal is involved.\n"
        "For example:\n"
        "    whogetsig 14\n"
        "    whogetsig -14\n"
        "    whogetsig -14 20\n"
        "\n"
        "* 14 is the signal-number of SIGALRM.\n"
        "* If a leading \"-\" is present, then that signal is deliberately blocked\n"
        "  (masked) for the main thread, but not masked off for child thread.\n"
        "* Second parameter(20) tells main thread to wait 20 seconds before unblocking\n"
        "  the signal, and then quit the whole program.\n"
        "\n"
        "We'll see that, for process-wide signal-number like SIGALRM: If you send\n"
        "that signal to main thread and the main thread is blocking that signal, \n"
        "the system will immediately deliver that signal to the child thread.\n"
    );
}

int
main(int argc, char* argv[])
{
    if(argc==1)
    {
        print_help();
        return 1;
    }

    bool is_block_mainsig = false;
    const char* signum = argv[1];
    if (signum[0] == '-') {
        is_block_mainsig = true;
        signum++;
    }

    int sigord = (int)strtoul(signum, nullptr, 0);
    const char *signame = strsigname(sigord);
    if (!signame)
        errExit("strsigname()");

    sigset_t blockset = {};
    sigemptyset(&blockset);
    sigaddset(&blockset, sigord);

    printf("Using signal-number %d (%s)\n", sigord, signame);
	
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sighandler;
    if (sigaction(sigord, &sa, nullptr))
        errExit("sigaction()");

    pthread_t t1 = {};
    int err = pthread_create(&t1, NULL, threadFunc, nullptr);
    if (err != 0)
        errExitEN(err, "pthread_create");

    pid_t maintid = gettid();
    PrnTs("Main  thread: PID=%ld, TID=%ld", (long)getpid(), (long)maintid);

    usleep(100 * 1000); // wait for child threads creation.

    printf("\n");

    int block_seconds = argc>2 ? atoi(argv[2]) : 20;
	
	if(is_block_mainsig)
	{
        err = pthread_sigmask(SIG_BLOCK, &blockset, nullptr);
        if (err != 0)
            errExitEN(err, "pthread_sigmask(SIG_BLOCK)");

        printf("==== Now main thread blocks %s for %d seconds. ====\n", signame, block_seconds);
    }

	printf("You can run `kill -%s %ld` from another terminal,\n", signame, (long)maintid);
    printf("and see which thread(TID) executes the signal handler.\n");
    printf("This program will quit in %d seconds.\n", block_seconds);
    printf("\n");

    time_t time_end = time(nullptr) + block_seconds;
	for(;;)
	{
        int seconds_remain = (int)(time_end - time(nullptr));
        if (seconds_remain > 0)
            sleep(seconds_remain);
        else
            break;

        printf("*\n");
	}

    if (is_block_mainsig)
    {
        PrnTs("Unblocking main thread's %s.", signame);
        err = pthread_sigmask(SIG_UNBLOCK, &blockset, nullptr);
        if (err != 0)
            errExitEN(err, "pthread_sigmask(SIG_UNBLOCK)");
    }
	
	// For simplicity, we omit pthread_join().
	
    exit(EXIT_SUCCESS);
}
