#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <vector>
#include "types.hpp"
#include "redirect.hpp"
#include "variables.hpp"

#include "builtin.hpp"

static exitcode_t cd (const std::vector<std::string> &args)
{
	if (args.size() < 1)
		throw "cd: Missing directory name";
	if (args.size() > 1)
		throw "cd: Only one path needed, don't provide more arguments.";

	if (chdir(args[0].c_str()) == -1)
		throw strerror(errno);

	return EXIT_SUCCESS;
}

static void /* noreturn */ shell_exit (const std::vector<std::string> &args)
{
	if (args.size() == 0)
		exit(EXIT_SUCCESS);

	if (args.size() > 1){
		fprintf(stderr, "psh: exit: More than 1 argument. Exitting anyway.\n");
	}

	exitcode_t code = 0;
	if (sscanf(args.begin()->c_str(), "%d", &code) != 1){
		fprintf(stderr, "psh: exit: Invalid exit code, defaulting to %d.\n",
				EXIT_FAILURE);
		code = EXIT_FAILURE;
	}

	exit(code);
}

static void explain (const std::vector<std::string> &args)
{
	for (auto arg: args){
		if (arg == "point of life")
			printf("Fourty two\n");
		else if (arg == "why not give up")
			printf("'cause there will be a day we'll meet again.\n");
		else if (arg == "throwing knives")
			printf("All you need is intuition, good timing, and a ton of frickin' luck.\n");
		else if (arg == "turtle" || arg == "turtles")
			printf("A swimming thing in a hard shell. Harmless. Also found in Minecraft.\n");

		else
			printf("There is no such thing known to man. Try asking the Ents instead.\n");
	}
}

static exitcode_t read (const std::vector<std::string> &args,
		const RedirectList &redirs)
{
	char c = 0;
	std::string line;
	ssize_t res;

	const char *varname = (args.size() > 0) ? args.begin()->c_str()
			: "REPLY";

	fd_t in = redirs.what_dupes_to(STDIN_FILENO);
	if (in == CLOSED_FILENO){
		fprintf(stderr, "read: Can only read from pipe or terminal.\n");
		return EXIT_FAILURE;
	}

	do {
		res = read(in, &c, 1);
		if (res < 0){
			fprintf(stderr, "read: %s.\n", strerror(errno));
			return EXIT_FAILURE;
		}
		line.push_back(c);
	} while (res > 0 && c != '\n');
	line.pop_back();

	var_set(varname, line.c_str());
	// TODO read -s
	// TODO read 1 line/all
	return EXIT_SUCCESS;
}

bool handle_builtin (const std::string &command,
		const std::vector<std::string> &args,
		const RedirectList &redirs,
		exitcode_t &exitcode)
{
	if (command == "exit")
		shell_exit(args);
	if (command == "cd"){
		exitcode = cd(args);
		return true;
	}
	if (command == "explain"){
		explain(args);
		exitcode = EXIT_SUCCESS;
		return true;
	}
	if (command == "read"){
		exitcode = read(args, redirs);
		return true;
	}

	/* Ok, so... I know it's a weird use of exceptions, but "break" and
	 * "continue" must immediatelly interrupt any compound command they're
	 * in, and throwing is a great way to accomplish that. I accept
	 * responsibility for any problems this will cause in the future. */
	if (command == "break")
		throw (int) SHELL_BREAK;
	if (command == "continue")
		throw (int) SHELL_CONTINUE;

	return false; /* not a builtin */
}
