#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>
#include <list>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"
#include "simple_command.hpp"

#include "pipeline.hpp"

/* Internal class returned by Pipeline::run() */
class RunningPipeline: public RunningCommand {
public:
	std::list<std::unique_ptr<RunningCommand>> children;

	RunningPipeline () { }

	void add_child (std::unique_ptr<RunningCommand> child)
	{
		children.push_back(std::move(child));
	}

	exitcode_t collect () noexcept {
		exitcode_t exitcode, partial_exitcode;

		assert_not_expired();
		expire();

		exitcode = EXIT_SUCCESS;
		while (children.size() > 0){
			auto child = children.begin();

			partial_exitcode = child->get()->collect();
			if (partial_exitcode != EXIT_SUCCESS)
				exitcode = partial_exitcode;

			children.erase(child);
		}

		return exitcode;
	}
};

static void owned_pipe2 (OwnedFile &rd, OwnedFile &wr, int flags)
{
	fd_t fds[2];

	if (pipe2(fds, flags) != 0)
		throw strerror(errno);

	rd.reset(fds[0]);
	wr.reset(fds[1]);
}

/* Compund commands (e. g. and/or) must run in a sub-process to not block
 * pipes. */
static std::unique_ptr<RunningCommand> run_in_subshell (const Command *what,
		const RedirectList &redirs)
{
	pid_t pid = fork();
	switch (pid){
	case -1:
		throw strerror(errno);
		break;
	case 0: /* child */
		exitcode_t code;
		try {
			code = what->run(redirs)->collect();
		} catch (const char *err){
			fprintf(stderr, "psh: ERROR: %s.\n", err);
			exit(EXIT_SYS_ERROR);
		}
		exit(code);
		break;
	}

	/* parent */
	std::unique_ptr<RunningCommand> res(new ChildProcess(pid));
	return res;
}

void Pipeline::add_member (std::unique_ptr<Command> member, const Token &op)
{
	/* avoid "unused argument" warning */
	assert(op.was_matched_against(REG_PIPE));
	members.push_back(std::move(member));
}

void Pipeline::add_final_member (std::unique_ptr<Command> member)
{
	members.push_back(std::move(member));
}

std::unique_ptr<RunningCommand> Pipeline::run (const RedirectList &redirs)
const
{
	OwnedFile current_input(CLOSED_FILENO);
	std::unique_ptr<RunningPipeline> res(new RunningPipeline);

	/* all members excecpt the last one will output to a pipe */
	auto last = --members.cend();
	for (auto it = members.cbegin(); it != last; it++){
		RedirectList pipe_redirs(redirs);

		if (current_input.get() != CLOSED_FILENO) /* not first */
			pipe_redirs.add_dup(current_input.get(), STDIN_FILENO);

		/* we need a pipe to the next one */
		OwnedFile rd, wr;
		owned_pipe2(rd, wr, O_CLOEXEC);

		pipe_redirs.add_dup(wr.get(), STDOUT_FILENO);

		if (it->get()->is_compound_command()){
			res->add_child(run_in_subshell(it->get(),
					pipe_redirs));
		} else
			res->add_child(it->get()->run(pipe_redirs));

		current_input.reset(rd.release());
	}

	/* run the last one */
	RedirectList pipe_redirs(redirs);
	pipe_redirs.add_dup(current_input.get(), STDIN_FILENO);
	res->add_child(last->get()->run(pipe_redirs));

	return res;
}

void Pipeline::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);

	fprintf(stderr, "Pipeline with %u members:\n", (unsigned) members.size());
	for (auto it = members.begin(); it != members.end(); it++)
		it->get()->dump(indent_depth + 1);
}

