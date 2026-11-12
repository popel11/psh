#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"
#include "regex.hpp"

#include "redir_command.hpp"

/* Find the start of operator, source and possibly target FD
 * in COMMAND. */
static void parse_redirect (const char *command, const char* &source,
	const char* &op, const char* &target) noexcept
{
	op = command;
	/* skip the possible descriptor number */
	while (isdigit(*op) || *op == '@')
		op++;

	if (isdigit(command[0]) || command[0] == '@')
		target = command;
	else
		target = NULL;

	source = op;
	while (*source == '<' || *source == '>')
		source++;
}

static void recognize_operator (const char *op, fd_t &default_target,
		int &open_mode)
{
	/* The operators must be ordered from longest to shortest, so no
	 * operator is a substring of any after it. */
	if (strncmp(op, ">>", strlen(">>")) == 0){
		open_mode = O_WRONLY | O_CREAT | O_APPEND;
		default_target = STDOUT_FILENO;

	} else if (strncmp(op, "<>", strlen("<>")) == 0){
		open_mode = O_RDWR | O_CREAT;
		default_target = -1; /* must have an explicit number */

	} else if (strncmp(op, ">", strlen(">")) == 0){
		open_mode = O_WRONLY | O_CREAT;
		default_target = STDOUT_FILENO;

	} else if (strncmp(op, "<", strlen("<")) == 0){
		open_mode = O_RDONLY;
		default_target = STDIN_FILENO;

	} else {
		throw "Invalid redirection operator";
	}
}

void Redirection::add_redir (const char *command)
{
	assert(match(command, REG_REDIRECTION));

	const char *source, *op, *target_str;
	fd_t target = 0;
	int open_mode = 0;

	parse_redirect(command, source, op, target_str);
	recognize_operator(op, target, open_mode);

	if (target_str != NULL && target_str[0] != '@')
		sscanf(target_str, "%d", &target);

	if (match(source, "^&[0-9]+$")){
		fd_t source_fd = 0;
		sscanf(source, "&%d", &source_fd);
		effects.add_dup(source_fd, target);

	} else if (target_str != NULL && target_str[0] == '@'){
		effects.add_open_eval(source, open_mode);

	} else {
		effects.add_open(source, open_mode, target);
	}
}

std::unique_ptr<RunningCommand> Redirection::run (const RedirectList &redirs)
const
{
	RedirectList effected_redirs(redirs);
	effected_redirs.append(effects);

	return inside->run(effected_redirs);
}

void Redirection::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);
	fprintf(stderr, "Redirections:\n");
	effects.dump(indent_depth + 1);

	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);
	fprintf(stderr, "...acting on:\n");
	inside->dump(indent_depth + 1);
}
