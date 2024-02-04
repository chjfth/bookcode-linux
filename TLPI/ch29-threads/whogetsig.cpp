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

bool g_is_block_childsig = false;
int g_sigord = 0; // the signal number

void block_or_unblock_signal(int signo, bool isblock)
{
    sigset_t blockset = {};
    sigemptyset(&blockset);
    sigaddset(&blockset, signo);

    int err = pthread_sigmask(isblock ? SIG_BLOCK : SIG_UNBLOCK, &blockset, nullptr);
    if (err != 0)
        errExitEN(err, "pthread_sigmask(SIG_BLOCK)");
}

static void*
threadFunc(void* arg)
{
    (void)arg;
    PrnTs("Child thread: PID=%ld, TID=%ld", (long)getpid(), (long)gettid());

    if (g_is_block_childsig)
    {
        block_or_unblock_signal(g_sigord, true);
        const char* signame = strsigname(g_sigord);
        printf("==== Now child thread blocks %s. ====\n", signame);
    }

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
        "whogetsig v1.0\n"
        "This program installs a signal-handler in main(), for a specific signal,\n"
        "and creates a child thread. The signal-handler prints gettid() value,\n"
        "so that we can know which thread is delivered that signal.\n"
        "\n"
        "You need to assign a parameter to tell which signal is involved.\n"
        "For example:\n"
        "    whogetsig 14\n"
        "    whogetsig -14\n"
        "    whogetsig --14\n"
        "    whogetsig -14 20\n"
        "\n"
        "* 14 is the signal-number of SIGALRM.\n"
        "* If one leading \"-\" is present, then that signal is deliberately blocked\n"
        "  (masked) for the main thread, but not masked off for child thread.\n"
        "* If two leading \"-\" is present, then that signal is blocked for both\n"
        "  main thread and child thread.\n"
        "* Second parameter(20) tells main thread to wait 20 seconds before unblocking\n"
        "  the signal, and then quit the whole program.\n"
        "\n"
        "We'll see that, kill() and tgkill() behave differently on signal sending.\n"
        "* kill() sends the signal process-wide, any thread not-blocking that signal\n"
        "  will be elected by the system to run the signal-handler.\n"
        "* tgkill() sends the signal thread-specific, only the target TID can handle it.\n"
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
    if (*signum == '-') {
        is_block_mainsig = true;
        signum++;
    }

	if(*signum == '-')	{ // second '-'
        g_is_block_childsig = true;
        signum++;
	}

    int sigord = (int)strtoul(signum, nullptr, 0);
    g_sigord = sigord;
    const char *signame = strsigname(sigord);
    if (!signame)
        errExit("strsigname()");

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
        block_or_unblock_signal(sigord, true);
        printf("==== Now main thread blocks %s for %d seconds. ====\n", signame, block_seconds);
    }

	printf("You can do kill() or tgkill() from another terminal,\n", signame, (long)maintid);
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
        block_or_unblock_signal(sigord, false);
    }
	
	// For simplicity, we omit pthread_join().
	
    exit(EXIT_SUCCESS);
}
