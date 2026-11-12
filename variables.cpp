#include <map>
#include <string>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include "parse.hpp"

#include "variables.hpp"

static std::map<std::string, std::string> variables;

/* A string is a valid variable name iff it matches this regex */
#define REG_VARNAME "[0-9a-zA-Z_]+|\\?"

static void sanitize_name (const char *name)
{
	regex_t r;

	if (regcomp(&r, REG_VARNAME, REG_EXTENDED) != 0)
		throw strerror(errno);

	if (regexec(&r, name, 0, NULL, 0) != 0)
		throw "Invalid variable name";

	regfree(&r);
}

static bool is_environ (const char *name)
{
	return name[0] == '_';
}

/* Throws if NAME is not a valid variable name. */
void var_set (const char *name, const char *value)
{
	sanitize_name(name);

	if (is_environ(name)){
		if (setenv(name+1, value, true) == -1)
			throw strerror(errno);
	} else {
		variables.insert_or_assign(name, value);
	}
}

void var_set (const char *name, int value)
{
	char num[21]; /* big enough to represent 2**64 */

	sprintf(num, "%d", value);
	var_set(name, num);
}

/* Return a pointer to immutable string with the variable's value.
 * For a nonexistent variable, return an empty string. */
static const char *var_get (const char *name)
{
	if (is_environ(name)){
		if (getenv(name+1) == NULL)
			return "";
		else
			return getenv(name+1);
	}

	auto ref = variables.find(name);
	if (ref == variables.end())
		return "";
	return ref->second.c_str();
}

void var_unset (const char *name)
{
	/* Unset it in both local and environment. It should only be in
	 * one of these, but just in case. */
	if (is_environ(name))
		unsetenv(name+1);

	auto ref = variables.find(name);
	if (ref != variables.end()){
		variables.erase(ref);
	}
}

std::string var_expand_all (const char *in)
{
	Token t(in);
	std::string res("");

	auto parts = t.cut_out("\\$(" REG_VARNAME ")");

	for (auto it = parts.begin(); it != parts.end(); it++){

		if (it->type == LEFT)
			continue;

		/* +1 skips the dollar sign */
		it->value = var_get(it->value.c_str()+1);
	}

	for (auto part: parts)
		res += part.value;

	return res;
}

std::string substitute_path (const char *progname)
{
	struct stat unused;
	Token path_str(var_get("_PATH"));

	if (progname[0] == '/'){ /* absolute path - don't substitute */
		std::string absolute(progname);
		return absolute;
	}

	auto paths = path_str.cut_out(":");

	for (auto it = paths.begin(); it != paths.end(); it++){
		if (it->type == MATCHED)
			continue;

		it->value.append("/");
		it->value.append(progname);
		if (stat(it->value.c_str(), &unused) == 0)
			return it->value;
	}

	throw "Command not found";
}
