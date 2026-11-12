#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <list>
#include <memory>
#include "types.hpp"
#include "redirect.hpp"
#include "parse.hpp"

#include "andor.hpp"

void AndOr::add_member (std::unique_ptr<Command> member, const Token &op)
/*inval*/
{
	members.push_back(std::move(member));

	if (op.value == "&&")
		operators.push_back(AND);
	else
		operators.push_back(OR);
}

void AndOr::add_final_member (std::unique_ptr<Command> member)
{
	members.push_back(std::move(member));
}

std::unique_ptr<RunningCommand> AndOr::run (const RedirectList &redirs) const
{
	exitcode_t partial_exitcode;

	auto member = members.cbegin();
	auto op = operators.cbegin();

	while (op != operators.cend()){
		std::unique_ptr<RunningCommand> running
				= member->get()->run(redirs);

		partial_exitcode = running->collect();
		switch (*op){
		case AND:
			/* AND breaks the chain on failure */
			if (partial_exitcode != EXIT_SUCCESS){
				std::unique_ptr<RunningCommand> res(
						new NothingRuns(partial_exitcode));
				return res;
			}
			break;
		case OR:
			/* OR breaks the chain on success */
			if (partial_exitcode == EXIT_SUCCESS){
				std::unique_ptr<RunningCommand> res(
						new NothingRuns(EXIT_SUCCESS));
				return res;
			}
		}

		member++;
		op++;
	}

	/* Nothing broke the chain, and we have iterator to the last member */
	std::unique_ptr<RunningCommand> running(member->get()->run(redirs));
	return running;
}

void AndOr::dump (unsigned indent_depth) const
{
	for (unsigned i = 0; i < indent_depth; i++)
		putc('\t', stderr);

	fprintf(stderr, "An and/or list of %zu members:\n", members.size());
	auto member = members.begin();
	auto op = operators.begin();
	while (op != operators.end()){
		(*member)->dump(indent_depth+1);
		for (unsigned i = 0; i < indent_depth+1; i++)
			putc('\t', stderr);
		switch (*op){
		case AND:
			fprintf(stderr, "&&\n");
			break;
		case OR:
			fprintf(stderr, "||\n");
			break;
		}

		member++;
		op++;
	}
	(*member)->dump(indent_depth+1);
}
