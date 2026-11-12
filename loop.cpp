#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include "redirect.hpp"
#include "parse.hpp"
#include "signal.hpp"

#include "loop.hpp"

Loop::Loop (std::unique_ptr<Command> _condition,
		std::unique_ptr<Command> _body)
{
	condition = std::move(_condition);
	body = std::move(_body);
}

std::unique_ptr<RunningCommand> Loop::run (const RedirectList &redirs) const
{
	std::unique_ptr<RunningCommand> rcond, rbody;
	exitcode_t exitcode = 0;

	while (true){
		rcond.reset(condition->run(redirs).release());

		if (rcond->collect() != EXIT_SUCCESS){
			/* in bash(1), loop with false condition
			 * returns success. */
			exitcode = EXIT_SUCCESS;
			break;
		}

		try {
			rbody.reset(body->run(redirs).release());
			rbody->collect(); /* discard the exit code */
		} catch (int event){
			if (event == SHELL_BREAK){
				exitcode = 0;
				break;
			}
			/* if "continue", just let it run again */
		}

		if (got_sigint){
			exitcode = EXIT_SIGNALLED;
			break;
		}
	}

	std::unique_ptr<RunningCommand> res(
			new NothingRuns(exitcode));
	return res;
}

void Loop::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);
	fprintf(stderr, "While (\n");
	condition->dump(indent_depth + 1);

	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);
	fprintf(stderr, ") do:\n");
	body->dump(indent_depth + 1);
}
