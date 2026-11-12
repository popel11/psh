#ifndef INPUT_HPP
#define INPUT_HPP 1

#include <stdio.h>
#include <string>

/* Read a line from stdin. If conneceted to a terminal, print a prompt
 * before reading. */
extern std::string input_line ();

/* Read the file at PATH into a string. */
extern std::string input_file (const char *path);

#endif
