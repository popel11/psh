#ifndef REGEX_HPP
#define REGEX_HPP 1

#include <stddef.h>

/* Return true if S matches REGEX, and in that case also set start and end
 * of the match. Used in multiple modules and it doesn't really belong
 * anywhere. */
extern bool match (const char *s, const char *regex, size_t &start,
		size_t &end);

/* Only tell if there's a match, without returning its position. */
inline bool match (const char *s, const char *regex)
{
	size_t _s, _e;
	return match(s, regex, _s, _e);
}

#endif
