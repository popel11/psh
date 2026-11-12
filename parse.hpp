/* This file defines the abstract types used by the parser in parse.cpp. */
#ifndef PARSE_HPP
#define PARSE_HPP 1

#include <assert.h>
#include <memory>
#include <list>
#include "types.hpp"
#include "tokenize.hpp"
#include "redirect.hpp"

/* A command, possibly running in a sub-process, that was not yet
 * waited for. */
/*abstract*/ class RunningCommand: public LifetimeGuarded {
public:
	virtual exitcode_t collect () noexcept = 0; /* should expire */

	~RunningCommand () { assert_expired(); }
};

/* Any type of command, be it a single program, a pipeline, a bracket...
 * See simple_command.hpp, sequence.hpp, andor.hpp, pipeline.hpp. */
/*abstract*/ class Command {
public:
	/* Whether this->run() does some potentially blocking operation.
	 * When such command is member of a pipeline, it is run in sub-process.
	 */
	virtual bool is_compound_command () { return false; }

	/* Run the command, without waiting for it to finish
	 * (beause it may be member of a pipeline).
	 * Should expire on that. */
	virtual std::unique_ptr<RunningCommand> run (
			const RedirectList &redirs) const = 0;

	/* Print out the object to stderr. Used for debugging. */
	virtual void dump (unsigned indent_depth) const = 0;

	virtual ~Command () { }
};

/* A type of command that is made of any number of sub-commands separated by
 * an operator. See pipeline.hpp, sequence.hpp, andor.hpp. */
/*abstract*/ class ChainableOperator: public Command {
public:
	virtual void add_member (std::unique_ptr<Command> member,
			const Token &op) = 0;
	virtual void add_final_member (std::unique_ptr<Command> member) = 0;

	virtual ~ChainableOperator () { }
};

/* Some Commands (see e. g. sequence.hpp) finish all processes in their run()
 * method, so they use this dummy running class. */
class NothingRuns: public RunningCommand {
public:
	NothingRuns (exitcode_t _ec): ec(_ec) { }

	exitcode_t collect () noexcept
	{ assert_not_expired(); expire();
		return ec;
	}

private:
	exitcode_t ec;
};

extern std::unique_ptr<Command> shell_parse (const std::list<Token> &tokens);

#endif
