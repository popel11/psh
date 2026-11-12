#include <regex.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

bool match (const char *s, const char *regex, size_t &start, size_t &end)
{
	regex_t rg;
	regmatch_t found;

	if (regcomp(&rg, regex, REG_EXTENDED) != 0)
		throw strerror(errno);

	bool res = (regexec(&rg, s, 1, &found, 0) == 0);
	if (res){
		start = found.rm_so;;
		end = found.rm_eo;
	}

	regfree(&rg);
	return res;
}
