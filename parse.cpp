#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <list>
#include <vector>
#include <memory>
#include "types.hpp"
#include "tokenize.hpp"
#include "redirect.hpp"

#include "simple_command.hpp"
#include "redir_command.hpp"
#include "pipeline.hpp"
#include "andor.hpp"
#include "sequence.hpp"
#include "loop.hpp"

#include "parse.hpp"

/* A command enclosed in brackets is treated as a single token, so we need an
 * array of objects that can be either a token or a bracket */
enum aux_type {
	TOKEN,
	BRACKET,
};
struct TokenOrBracket {
	enum aux_type type;

	Token token;
	std::list<TokenOrBracket> bracket;

	TokenOrBracket (): token("") { type = TOKEN; }

	/* Having this structure copy-constructible aids in creating
	 * a list of them. */
	TokenOrBracket (const TokenOrBracket &src): type(src.type),
			token(src.token), bracket(src.bracket) { }

	bool was_matched_against (const char *regex) const
	{
		if (type == BRACKET)
			return false;
		return token.was_matched_against(regex);
	}

	void dump (unsigned indent_depth) const
	{
		for (unsigned i = 0; i < indent_depth; i++)
			putc('\t', stderr); // TODO duplicity

		switch (type){
		case TOKEN:
			fprintf(stderr, "A token '%s'\n", token.value.c_str());
			break;
		case BRACKET:
			fprintf(stderr, "A bracket:\n");
			for (auto it = bracket.cbegin(); it != bracket.cend(); it++)
				it->dump(indent_depth+1);
		}
	}
};

typedef typename std::list<TokenOrBracket>::const_iterator token_iter;
typedef typename std::list<TokenOrBracket>::iterator token_mut_iter;

/* Return an iterator to the first ocurence of TOK_REGEX in [START, END),
 * or END if none found. */
static token_iter find (const token_iter &start, const token_iter &end,
		const char *tok_regex)
{
	for (auto it = start; it != end; it++){
		if (it->was_matched_against(tok_regex))
		{
			return it;
		}
	}
	return end;
}

/* Return iterators to all occurences of TOK_REGEX in [START, END) */
static std::list<token_iter> find_all (
		const token_iter &start, const token_iter &end,
		const char *tok_regex)
{
	std::list<token_iter> occurences;

	for (auto it = start; it != end; it++){
		if (it->was_matched_against(tok_regex))
		{
			occurences.push_back(it);
		}
	}

	return occurences;
}

/* This function may operate recursively on subsets of its input.
 * To prevent unnecessary copying, the list is given as a pair of
 * iterators. */
static std::unique_ptr<Command> internal_shell_parse (
		const token_iter &start, const token_iter &end)
{
	assert(start != end); /* command must not be empty */

	auto loop_arrows = find_all(start, end, REG_LOOP);
	if (loop_arrows.size() > 0){
		/* Deal with one of them, the others are in sub-expression. */
		auto arrow = *(loop_arrows.begin());

		std::unique_ptr<Command> condition, body;
		condition.reset(internal_shell_parse(start, arrow).release());
		body.reset(internal_shell_parse(++arrow, end).release());

		std::unique_ptr<Command> res(new Loop(
			std::move(condition),
			std::move(body)
		));
		return res;
	}

	std::vector<const char*> to_check = {REG_SEMICOLON, REG_ANDOR, REG_PIPE};
	for (auto op : to_check){
		auto operators = find_all(start, end, op);
		if (operators.size() == 0) /* there is none */
			continue; /* try the lower-priority ones */

		std::unique_ptr<ChainableOperator> res;
		if (strcmp(op, REG_SEMICOLON) == 0)
			res.reset(new Sequence);
		else if (strcmp(op, REG_ANDOR) == 0)
			res.reset(new AndOr);
		else if (strcmp(op, REG_PIPE) == 0)
			res.reset(new Pipeline);

		auto part_start = start;
		for (auto op_it: operators){
			std::unique_ptr<Command> sub = internal_shell_parse(
					part_start, op_it);
			res->add_member(std::move(sub), op_it->token);

			part_start = op_it;
			part_start++;
		}
		/* there must be tokens after the last operator,
		 * otherwise it's missing an operand */
		if (part_start == end)
			throw "Missing operand";

		std::unique_ptr<Command> sub = internal_shell_parse(
				part_start, end);
		res->add_final_member(std::move(sub));

		return res;
	}

	/* Find the redirections, if any */
	auto redir = find(start, end, REG_REDIRECTION);
	if (redir != end){
		std::unique_ptr<Redirection> res(
			new Redirection(
			internal_shell_parse(start, redir))
		);

		for (; redir != end; redir++){
			if (!redir->was_matched_against(REG_REDIRECTION))
				throw "Redirections must be at the end of arguments.";

			res->add_redir(redir->token.value.c_str());
		}

		return res;
	}

	/* It may also be just a bracket. */
	if (start->type == BRACKET){
		auto after = start;
		after++;
		if (after != end)
			throw "Syntax error: words after bracket";

		return internal_shell_parse(start->bracket.cbegin(),
				start->bracket.cend());
	}

	/* No operator was found, and it's just a Simple Command. */
	for (auto it = start; it != end; it++){
		if (it->type != TOKEN || it->token.type != LEFT)
			throw "Syntax error";
	}

	auto it = start;
	std::unique_ptr<SimpleCommand> res(new SimpleCommand(it->token.value));
	it++;
	for (; it != end; it++)
		res->add_argument(it->token.value);

	return res;
}

static token_mut_iter find_closing_bracket (const token_mut_iter &opening,
		const token_mut_iter &end)
{
	unsigned depth = 1;
	auto pos = opening;

	assert(opening->token.was_matched_against(REG_OPEN_BRACKET));

	do {
		pos++;
		if (pos == end)
			throw "Unterminated bracket";

		if (pos->token.was_matched_against(REG_OPEN_BRACKET))
			depth++;
		if (pos->token.was_matched_against(REG_CLOSE_BRACKET))
			depth--;
	} while (depth > 0);

	return pos;
}

static void replace_brackets (/*inval*/ std::list<TokenOrBracket> &tokens)
{
	for (auto it = tokens.begin(); it != tokens.end(); it++){

		/* no need to check it->type, they're all TOKEN */
		if (it->token.was_matched_against(REG_OPEN_BRACKET)){
			auto closing = find_closing_bracket(it, tokens.end());

			/* erase opening bracket*/
			it = tokens.erase(it);

			/* replace closing bracket with contents, erase the
			 * rest */
			std::list<TokenOrBracket> content(it, closing);
			replace_brackets(content);
			closing->type = BRACKET;
			closing->bracket = content;

			tokens.erase(it, closing);
			it = closing;
		}
	}
}

std::unique_ptr<Command> shell_parse (const std::list<Token> &tokens)
{
	TokenOrBracket p;
	std::list<TokenOrBracket> internal_list;

	for (auto it = tokens.cbegin(); it != tokens.cend(); it++){
		p.type = TOKEN;
		p.token = *it;
		internal_list.push_back(p);
	}

	replace_brackets(internal_list);

	return internal_shell_parse(internal_list.cbegin(),
			internal_list.cend());
}
