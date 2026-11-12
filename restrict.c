/* restrict 1.0.0
 * by <v.polak06@email.cz> on 2025-05-06
 *
 * Program that executes another program as an unprivileged user, to not give
 * unnecessary privileges to programs.
 *
 * The restricted program has UID and GID set to `restricted` and is in a new
 * process group (to deny it access to controlling terminal) but it inherits
 * all open file descriptors.
 *
 * In layman's terms: it can't access your files, except those opened for it
 * by your shell, and if not run in terminal, it cannot access yoour terminal
 * unless you allow it.
 *
 * TODO bash passes open /dev/udmabuf to its children - is it dangerous?
 *
 * To install:
 *  - check that the TIOCSTI ioctl is disabled, to prevent terminal injections:
 *      `systemd -a | grep dev.tty.legacy_tiocsti`
 *      should be set to false, 0 or no
 *  - create a system user with name "restricted" with the command:
 *      `adduser --system restricted`
 *  - compile this file to a binary executable:
 *      `cc -o restrict restrict.c`
 *  - set owner `root:root` and mode `rwsr-xr-x`
 *      `chown root:root restricted
 *      `chmod 4755 restricted`
 *  - move it to /usr/bin, or wherever
 *
 * If you want to use other username than "restricted", define macro
 * RESTRICTED_NAME as a string literal on compilation:
 *      `cc -o restrict -DRESTRICTED_NAME=\"any-username\" restrict.c`
 *
 * This program is public domain. */
#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef RESTRICTED_NAME
# define RESTRICTED_NAME "restricted"
#endif

/* error handling */
static char *prog_name = "";

#define init(pn) { prog_name = pn; }

#define report(context, msg) \
	fprintf(stderr, "%s: %s: %s.\n", prog_name, (context), (msg));

#define die(context) { report(context, strerror(errno)); exit(1); }
#define die_msg(msg) { report("",      (msg)          ); exit(1); }
#define PLEASE_REPORT "\n\tThat's weird. Please tell me at v.polak06@email.cz"


/* RUNS AS ROOT - get UID and GID of the unprivileged user from /etc/passwd */
static void find_user (uid_t *dummy_uid, gid_t *dummy_gid)
{
	struct passwd *dummy_user = NULL;

	errno = 0; /* to distinguish "search error" from "nonexistent user" */

	dummy_user = getpwnam(RESTRICTED_NAME);
	if (dummy_user == NULL){
		if (errno == 0){ /* user not found */
			die_msg("User '"RESTRICTED_NAME"' does not exist.");
		} else
			die("could not find user '" RESTRICTED_NAME"'");
	}
	*dummy_uid = dummy_user->pw_uid;
	*dummy_gid = dummy_user->pw_gid;
}

/* BEGINS AS ROOT - Change all IDs to `dummy_uid` or `dummy_gid`. */
static void become_user (uid_t dummy_uid, gid_t dummy_gid)
{
	if (setgroups(0, NULL) != 0)
		die("dropping supplementary groups");
	if (setresgid(dummy_gid, dummy_gid, dummy_gid) != 0)
		die("setting group");
	if (setresuid(dummy_uid, dummy_uid, dummy_uid) != 0)
		die("setting user");
}

/* Wrapper for opendir("/proc/self/fd") that skips the "." and ".." files */
static DIR *open_fd_dir ()
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir("/proc/self/fd");
	if (dir == NULL)
		die("accessing /proc/self/fd");

	ent = readdir(dir);
	if (ent == NULL)
		die("readdir");
	if (strcmp(ent->d_name, ".") != 0)
		die_msg("No '.' in /proc/self/fd."PLEASE_REPORT);

	ent = readdir(dir);
	if (ent == NULL)
		die("readdir");
	if (strcmp(ent->d_name, "..") != 0)
		die_msg("No '.' in /proc/self/fd."PLEASE_REPORT);

	return dir;
}

/* Close all open files except stderr. Not for security (the sandboxed program
 * keeps them), but leaving files open in the parent would cause problems with
 * pipes and stuff. */
static void close_all_files ()
{
	DIR *dir;
	struct dirent *fd_entry;
	int open_fd = -1;

	dir = open_fd_dir();

	while (1){
		errno = 0;
		fd_entry = readdir(dir);
		if (fd_entry == NULL){
			if (errno == 0) /* end of list */
				break;
			else /* error*/
				die("readdir");
		}

		if (sscanf(fd_entry->d_name, "%d", &open_fd) != 1){
			die_msg("File in /proc/self/fd not integer."
				PLEASE_REPORT);
		}

		/* keep error reporting and the directory we're parsing */
		if (open_fd == STDERR_FILENO || open_fd == dirfd(dir))
			continue;

		close(open_fd);
		/* We don't care if close() fails, its only purpose is to free
		 * the descriptor. */
	}

	if (closedir(dir) == -1){ /* should never happen */
		die("closedir");
	}
}

/* SIGNAL HANDLING
 * The sandboxed program must be in a fresh session to deny it access
 * to controlling terminal, but that would make it immune to Ctrl+C
 * interrupts, which we want to work.
 * We execute the program in a child process which sets session,
 * the parent remains in the current session and forwards signals to
 * the child. */
static pid_t child = -1;

int forwarded_signals[] = {
	SIGABRT, SIGALRM, SIGINT, SIGQUIT, SIGTSTP, SIGTERM, SIGTTIN,
	SIGTTOU, SIGUSR1, SIGUSR2};

static void forward_signal (int sig)
{
	kill(child, sig);
}

static void set_sig_handlers ()
{
	struct sigaction sa;
	unsigned long i;

	sa.sa_handler = forward_signal;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	for (i = 0; i < (sizeof(forwarded_signals)/sizeof(int)); i++){

		if (sigaction(forwarded_signals[i], &sa, NULL) != 0){
			report("sigaction", strerror(errno));
			/* don't die() - child is running */
		}
	}
}

/* Terminate the caller by raising a specific signal */
static /* noreturn */ void die_by_signal (int sig)
{
	struct sigaction sa;

	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	sigaction(sig, &sa, NULL);

	raise(sig);

	/* There is no way to set termination on a signal unless it's its
	 * default behavior, so the raise() should always end us. */
	die("Not killed by kill signal."PLEASE_REPORT);
}

int main (int argc, char **argv)
{
	uid_t dummy_uid = -1;
	gid_t dummy_gid = -1;
	pid_t child = -1;
	int wstatus = 0;

	init(argv[0]);

	if (argc < 2){
		fprintf(stderr, "%s: No command to execute.\n", argv[0]);
		return 1;
	}

	find_user(&dummy_uid, &dummy_gid);
	become_user(dummy_uid, dummy_gid);

	chdir("/");

	child = fork();
	switch (child){
	case -1:
		die("fork");
	case 0:
		/* child */
		if (setsid() == (pid_t) -1)
			die("setsid");

		execv(argv[1], argv+1);

		die(argv[1]); /* if execv() failed */
	}
	/* parent continues */

	set_sig_handlers();
	close_all_files();

	while (1){
		if (waitpid(child, &wstatus, 0) == -1)
			continue;

		/* after child ended: */
		if (WIFEXITED(wstatus)){
			exit(WEXITSTATUS(wstatus));

		} else if (WIFSIGNALED(wstatus)){
			die_by_signal(WTERMSIG(wstatus));

		} else {
			kill(child, SIGKILL); /* in case it survived */
			die("Not exited, not signalled."PLEASE_REPORT);
		}
	}
}
