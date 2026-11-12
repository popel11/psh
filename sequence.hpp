#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP 1

#include <memory>
#include <list>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

/* A list of commands to be executed sequentially, separated by ';'. */
class Sequence: public ChainableOperator {
public:
	Sequence () { }
	~Sequence () { }

	bool is_compound_command () { return true; }

	void add_member (std::unique_ptr<Command> member, const Token &op);
	void add_final_member (std::unique_ptr<Command> member);

	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

private:
	std::list<std::unique_ptr<Command>> members;
};

#endif
