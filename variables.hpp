/* To refer to an environment variable, prefix its name with an underscore.
 * Fo example, "$_foo" refers to an environment var named "foo".
 * Variables not starting with an underscore are local variables. */
#ifndef VARIABLES_HPP
#define VARIABLES_HPP 1

#include <string>

/* Set either locally or into environment, based on the "_" prefix.
 * Throw if NAME is not a valid variable name. */
extern void var_set (const char *name, const char *value);

/* Converts int to string. Useful for $?. */
extern void var_set (const char *name, int value);

extern void var_unset (const char *name);

/* Replace variable identifiers (prefixed with '$') with the
 * value of the variable and return the resulting string (heap-allocated). */
extern std::string var_expand_all (const char *in);

/* Find PROGNAME in one of the directories in $PATH and return the full
 * path. Throws if not found. */
extern std::string substitute_path (const char *progname);

#endif
