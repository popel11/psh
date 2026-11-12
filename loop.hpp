/* psh's non-standard feature - a loop operator.
 * Normal `while` would be hard to implement, so I made loops like
 * `condition -> loop_body`. Loops have higer priority than pipes, and/ors and
 * semicolons, see parse.cpp. */
#ifndef LOOP_HPP
#define LOOP_HPP 1

#include <memory>
#include "redirect.hpp"
#include "parse.hpp"

class Loop: public Command {
public:
	Loop (std::unique_ptr<Command> condition,
		std::unique_ptr<Command> body);

	bool is_compound_command () { return true; }

	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

private:
	std::unique_ptr<Command> condition;
	std::unique_ptr<Command> body;
};

#endif
