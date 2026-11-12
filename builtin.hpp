/* Handling of built-in commands (used in simple_command.cpp) */
#ifndef BUILTIN_HPP
#define BUILTIN_HPP 1

#include <string>
#include <vector>
#include "types.hpp"
#include "redirect.hpp"

/* See if COMMAND is a built-in. If yes, execute it, store exit code
 * into EXITCODE and return true. If not, return false. */
extern bool handle_builtin (const std::string &command,
		const std::vector<std::string> &args,
		const RedirectList &redirs,
		/*strong*/ exitcode_t &exitcode);

#endif
