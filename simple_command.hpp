#ifndef SIMPLE_COMMAND_HPP
#define SIMPLE_COMMAND_HPP 1

#include <sys/types.h>
#include <memory>
#include <vector>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

/* One program or one built-in. */
class SimpleCommand: public Command {
public:
	SimpleCommand (std::string _name): name(_name) { }
	void add_argument (std::string arg)
	{
		args.push_back(arg);
	}

	void add_eval_redir (const char *redir);

	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

	~SimpleCommand () { }

private:
	std::string name;
	std::vector<std::string> args;
};

/* This is also used in pipeline.cpp. */
class ChildProcess: public RunningCommand {
public:
	ChildProcess (pid_t _child)
	{
		child = _child;
	}

	~ChildProcess () { collect(); } /* prevent unwaited-for children */

	exitcode_t collect () noexcept;

private:
	pid_t child;
};

#endif
