#include <signal.h>
#include <string.h>
#include <errno.h>

#include "signal.hpp"

bool got_sigint = false;

static void forward_signal (int sig)
{
	/* kill(0) sends signal to all processes in the group, including self,
	 * so this handler gets called again. */
	static bool killed = false;

	if (!killed){
		killed = true; /* prevent an infinite loop */
		got_sigint = true;
		kill(0, sig); /* forward it to the entire group */
	} else {
		killed = false;
	}
}

void init_signals ()
{
	struct sigaction sa;

	sa.sa_handler = forward_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) != 0)
		throw strerror(errno);
}
