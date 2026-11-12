#include <unistd.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <list>
#include <string>
#include "types.hpp"
#include "redirect.hpp"

#include "exec.hpp"

static pid_t exec_redirected (std::vector<const char*> command,
		const RedirectList &redirs)
{
	pid_t pid;
	std::list<std::string> added_args;

	pid = fork();
	if (pid == 0){ /* child */
		try {
			added_args = redirs.run();
			for (auto arg: added_args)
				command.push_back(arg.c_str());

			command.push_back(NULL); /* execv() requires that */

		} catch (const char *err){
			fprintf(stderr, "psh: ERROR: %s\n", err);
			exit(EXIT_SHELL_ERROR);
		}

		execv(command[0], (char *const*) command.data());

		perror("psh: execv");
		exit(EXIT_SHELL_ERROR);
	}

	if (pid < 0)
		throw strerror(errno);

	return pid;
}

pid_t launch (const char *program, const std::vector<std::string> &args,
		const RedirectList &redirs)
{
	/* create an array of pointers for execv() */
	std::vector<const char*> cmd_array;

	if (program[0] == ':') /* do not run as restricted */
		cmd_array.push_back(program+1);
	else { /* run as restricted */
		cmd_array.push_back("/bin/restrict");
		cmd_array.push_back(program);
	}

	for (auto it = args.begin(); it != args.end(); it++)
		cmd_array.push_back(it->c_str());

	return exec_redirected(cmd_array, redirs);
}
