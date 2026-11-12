#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <istream>

#include "input.hpp"

static void print_prompt ()
{
	char *cwd = getcwd(NULL, 0);
	if (cwd != NULL){
		printf("%s", cwd);
		free(cwd);
	} else {
		printf("<err>");
	}

	if (getuid() == 0)
		putchar('#');
	else
		putchar('$');

	putchar(' ');
}

std::string input_line ()
{
	std::string line("");

	if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
		print_prompt();

	do {
		line.push_back(getchar());
	} while (line[line.size()-1] != '\n' && !feof(stdin));

	line.pop_back();
	return line;
}

std::string input_file (const char *path)
{
	std::istream f(path);
	std::string text("");
	int c = 0;

	do {
		f.get(c);
		text.push_back((char) c);
	} while (f.rdstate() != std::ios_base::eofbit);

	text.pop_back();
	return text;
}
