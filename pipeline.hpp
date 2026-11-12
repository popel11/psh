#ifndef PIPELINE_HPP
#define PIPELINE_HPP 1

#include <list>
#include <memory>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

class Pipeline : public ChainableOperator {
public:
	Pipeline () { }
	bool is_compound_command () { return true; }

	void add_member (std::unique_ptr<Command> member, const Token &op);
	void add_final_member (std::unique_ptr<Command> member);

	/* Returns instance of an internal class defined in pipeline.cpp */
	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

	~Pipeline () { };

private:
	std::list<std::unique_ptr<Command>> members;
};

#endif
