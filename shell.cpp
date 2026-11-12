/* Main module. Reads commands in a loop and executes them. */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <string>
#include "redirect.hpp"
#include "tokenize.hpp"
#include "parse.hpp"
#include "input.hpp"
#include "variables.hpp"
#include "signal.hpp"

#define SET_CLOEXEC(fd) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC)

#define VERSION "0.0"

static void run_command (const char *cmd)
{
	got_sigint = false; /* SIGINT may have cancelled the last command */

	auto tokens = tokenize(cmd);
	if (tokens.size() == 0)
		return;

	/* expand variables */
	for (auto it = tokens.begin(); it != tokens.end(); it++)
		it->value = var_expand_all(it->value.c_str());

	std::unique_ptr<Command> to_run = shell_parse(tokens);

	RedirectList std_streams;
	std_streams.add_inherit(STDIN_FILENO);
	std_streams.add_inherit(STDOUT_FILENO);
	std_streams.add_inherit(STDERR_FILENO);

	std::unique_ptr<RunningCommand> to_collect = to_run->run(std_streams);

	var_set("?", to_collect->collect());
}

int main (int argc, char **argv)
{
	std::string command;

	if (argc > 1 && strcmp(argv[1], "--version") == 0){
		fprintf(stderr, "psh version " VERSION "\n");
		fprintf(stderr, "by v.polak06@email.cz\n");
		return EXIT_SUCCESS;
	}

	try {
		init_signals();
	} catch (const char *err) {
		fprintf(stderr, "%s: ERROR: %s.\n", argv[0], err);
		return EXIT_FAILURE;
	}


	/* To avoid leaking open files to sandboxed children, all descriptors
	 * must be close-on-exec. */
	SET_CLOEXEC(STDIN_FILENO);
	SET_CLOEXEC(STDOUT_FILENO);
	SET_CLOEXEC(STDERR_FILENO);

	while (!feof(stdin)) {
		command = input_prompted(stdin, geteuid() == 0 ? "# ": "$ ");
		try {
			run_command(command.c_str());
		} catch (const char *error){
			fprintf(stderr, "%s: Could not execute '%s': %s\n",
					argv[0], command.c_str(), error);
			var_set("?", 127);
		}
	}

	return 0;
}
