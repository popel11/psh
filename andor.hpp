#ifndef ANDOR_HPP
#define ANDOR_HPP 1

#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

enum AndOrOp {
	AND,
	OR,
};

class AndOr: public ChainableOperator {
public:
	AndOr () { }
	~AndOr () { }
	bool is_compound_command () { return true; }

	void add_member (std::unique_ptr<Command> member,
			const Token &op) /*inval*/;
	void add_final_member (std::unique_ptr<Command> member);

	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

private:
	std::list<std::unique_ptr<Command>> members;
	std::list<enum AndOrOp> operators;
};

#endif
