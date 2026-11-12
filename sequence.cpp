#include <stdio.h>
#include <assert.h>
#include <memory>
#include <list>
#include "tokenize.hpp"
#include "redirect.hpp"
#include "parse.hpp"

#include "sequence.hpp"

void Sequence::add_member (std::unique_ptr<Command> member, const Token &op)
{
	/* avoid "unused argument" warning */
	assert(op.was_matched_against(REG_SEMICOLON));
	members.push_back(std::move(member));
}

void Sequence::add_final_member (std::unique_ptr<Command> member)
{
	members.push_back(std::move(member));
}

std::unique_ptr<RunningCommand> Sequence::run (const RedirectList &redirs)
const
{
	exitcode_t ec;

	for (auto it = members.cbegin(); it != members.cend(); it++){
		std::unique_ptr<RunningCommand> running
				= it->get()->run(redirs);

		ec = running->collect();
	}

	std::unique_ptr<RunningCommand> res(new NothingRuns(ec));
	return res;
}

void Sequence::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);

	fprintf(stderr, "A list of commands:\n");
	for (auto it = members.cbegin(); it != members.cend(); it++)
		it->get()->dump(indent_depth + 1);
}
