#ifndef EXEC_HPP
#define EXEC_HPP 1

#include <sys/types.h>
#include <vector>
#include <string>
#include "redirect.hpp"

/* Create a new process executing PROGRAM.
 * The redirections listed by REDIRS will be performed.
 * PROGRAM must be an absolute path, possibly prefixed by ":".
 * Run as nobody, unless prefixed with the ':' character.
 * Return PID of the child process. */
extern pid_t launch (const char *program, const std::vector<std::string> &args,
		const RedirectList &redirs);

#endif

