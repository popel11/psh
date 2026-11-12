#ifndef TOKENIZE_HPP
#define TOKENIZE_HPP

#include <stdlib.h>
#include <list>
#include <string>

enum token_type {
	MATCHED,
	LEFT,
};

/* A string is parsed into tokens by searching for regular expressions in it.
 * The matches of each regex are stored as part of the token. */
class Token {
public:
	std::string value;

	/* Indicates whether this token was matched against a regex
	 * or it's a word that remained after a regex matched next to it. */
	enum token_type type;

	/* The regex that this token matched (pointer to a string literal).
	 * Not used in LEFT parts. */
	const char *matched_against;

	/* used to safely test the value of MATCHED_AGAINST
	 * (guarding for LEFT). */
	bool was_matched_against (const char *what) const noexcept;

	/* Constructors for LEFT tokens. */

	Token (const char *_value);
	/* Only use first VALUE_LEN characters of _VALUE.
	 * Used in SectionedString::cut_out. */
	Token (const char *_value, size_t value_len);

	/* Likewise for MATCHED tokens */

	Token (const char *_value, const char *_matched_against);
	Token (const char *_value, size_t value_len,
			const char *_matched_against);

	/* Split the token into a list of MATCHED parts matching REGEX
	 * and LEFT parts between them. If nothing matches, or the token
	 * is already MATCHED, it is returned unchanged. */
	std::list<Token> cut_out (const char *regex) const;

	void dump () const;
};

/* these type aliases make function headers shorter */
typedef typename std::list<Token>::iterator TokensIterator;
typedef typename std::list<Token>::const_iterator TokensConstIterator;

/* Function for parsing shell commands. Given a command or sequence of
 * commands, break it to tokens. */
extern std::list<Token> tokenize (const char *s);

/* Regular expressions used by shell_parse().
 * Must be used in the order listed here, otherwise some of them would match
 * at unwanted places */
#define REG_SINGLE_QUOTES "'[^']*'"
#define REG_DOUBLE_QUOTES "\"[^\"]*\""
//#define REG_WHITESPACE "[[:blank:]]+"
#define REG_WHITESPACE " +"
#define REG_LOOP "->"
#define REG_SEMICOLON ";"
#define REG_ANDOR "&&|\\|\\|"
#define REG_PIPE "\\|"
#define REG_OPEN_BRACKET "\\{"
#define REG_CLOSE_BRACKET "\\}"
#define REG_REDIRECTION "([0-9]*|@)[<>]+&?.*"
// #define REG_BACKGROUND "&" TODO

#endif
