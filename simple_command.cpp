#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <vector>
#include <algorithm>
#include "types.hpp"
#include "redirect.hpp"
#include "exec.hpp"
#include "parse.hpp"
#include "builtin.hpp"
#include "variables.hpp"

#include "simple_command.hpp"

exitcode_t ChildProcess::collect () noexcept
{
	assert_not_expired();
	expire();

	int wstatus = 0;
	pid_t res = 0;

	/* Errors for waitpid() are only EINTR and invalid arguments.
	 * Arguments should be valid and we account for EINTR by
	 * repeating, so no throwing is needed. */
	do {
		res = waitpid(child, &wstatus, 0);
	} while (res < 0
		|| !(WIFEXITED(wstatus) || WIFSIGNALED(wstatus)));

	if (WIFEXITED(wstatus))
		return WEXITSTATUS(wstatus);
	else
		return EXIT_SIGNALLED;
}

std::unique_ptr<RunningCommand> SimpleCommand::run (const RedirectList &redirs)
const
{
	exitcode_t ec;
	if (handle_builtin(name, args, redirs, ec)){
		/* it is a built-in command */
		std::unique_ptr<RunningCommand> res(new NothingRuns(ec));
		return res;
	}

	/* Use $PATH to find the program */
	std::string full_path;
	if (name[0] == ':'){

		if (name.size() == 1)
			throw "Missing program name";
		full_path = substitute_path(name.c_str() + 1);
		full_path.insert(full_path.begin(), ':');

	} else
		full_path = substitute_path(name.c_str());

	std::unique_ptr<RunningCommand> res(
		new ChildProcess(
		launch(full_path.c_str(), args, redirs)));
	return res;
}

void SimpleCommand::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);

	fprintf(stderr, "A Simple Command '%s' with args", name.c_str());
	for (auto arg: args)
		fprintf(stderr, " '%s'", arg.c_str());
	putc('\n', stderr);
}
