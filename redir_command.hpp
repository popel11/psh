#ifndef REDIR_COMMAND_HPP
#define REDIR_COMMAND_HPP 1

#include <memory>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

class Redirection: public Command {
public:
	Redirection (std::unique_ptr<Command> _inside)
	{
		inside = std::move(_inside);
	}

	bool is_compound_command () { return inside->is_compound_command(); }

	void add_redir (const char *command);

	std::unique_ptr<RunningCommand> run (const RedirectList &redirs) const;

	void dump (unsigned indent_depth) const;

private:
	std::unique_ptr<Command> inside;
	RedirectList effects;
};

#endif
