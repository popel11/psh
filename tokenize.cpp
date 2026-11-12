#include <string.h>
#include <string>
#include <assert.h>
#include <errno.h>
#include "regex.hpp"

#include "parse.hpp"

Token::Token (const char *_value, size_t value_len,
		const char *_matched_against):
		value(_value, value_len), matched_against(_matched_against)
{
	type = MATCHED;
}
Token::Token (const char *_value, const char *_matched_against):
		Token(_value, strlen(_value), _matched_against) { }

Token::Token (const char *_value, size_t value_len):
		Token(_value, value_len, "")
{
	type = LEFT;
}
Token::Token (const char *_value): Token(_value, strlen(_value)) { }

bool Token::was_matched_against (const char *what) const noexcept
{
	if (type == LEFT)
		return false;
	return strcmp(what, matched_against) == 0;
}
// TODO tests
std::list<Token> Token::cut_out (const char *regex) const
{
	std::list<Token> parts;
	size_t so, eo;
	const char *ptr;

	/* don't cut alredy matched parts, e. g. don't search for
	 * pipe "|" in or operator "||" */
	if (type == MATCHED){
		parts.push_back(*this);
		return parts;
	}

	ptr = value.c_str();
	while (match(ptr, regex, so, eo)){
		if (so != 0){
			/* the match is not at the start */
			Token new_part(ptr, so);
			parts.push_back(new_part);
		}

		Token new_part(ptr + so, eo - so, regex);
		parts.push_back(new_part);

		ptr += eo;
	}

	if (*ptr != '\0'){ /* a bit was left after the last match */
		Token new_part(ptr);
		parts.push_back(new_part);
	}

	return parts;
}

void Token::dump () const
{
	switch (type) {
	case MATCHED:
		fprintf(stderr, "Token matched against %s\n", matched_against);
		break;
	case LEFT:
		fprintf(stderr, "Token left after cut: %s\n", value.c_str());
		break;
	}
}

/* List of tokens with methods, to make tokenizing easier */
class SectionedString {
public:
	std::list<Token> parts;

	SectionedString (const char *s)
	{
		Token p(s);
		parts.push_back(p);
	}

	TokensIterator begin () { return parts.begin(); }
	TokensIterator end   () { return parts.end(); }

	/* Split every LEFT token by REGEX. */
	void cut_out (const char *regex);

	/* Concatenate the part at POS with the parts next to it
	 * (previous and next), but only if the surrounding parts are LEFT. */
	void blend (const TokensIterator &pos);
};

void SectionedString::cut_out (const char *regex)
{
	std::list<Token> new_parts;
	std::list<Token> split;

	for (auto it = parts.begin(); it != parts.end(); it++){
		split = it->cut_out(regex);
		for (auto part = split.cbegin(); part != split.cend(); part++)
			new_parts.push_back(*part);
	}

	parts = new_parts;
}

void SectionedString::blend (const TokensIterator &pos) /*inval*/
{
	TokensIterator prev, next;
	std::string blended_part("");

	if (pos != parts.begin()){
		prev = pos;
		prev--;
		if (prev->type == LEFT){
			blended_part += prev->value;
			parts.erase(prev);
		}
	}

	blended_part += pos->value;

	next = pos;
	next++;
	if (next != parts.end() && next->type == LEFT){
		blended_part += next->value;
		parts.erase(next);
	}

	pos->value = blended_part;
	pos->type = LEFT;
}

static void escape (/*inval*/ SectionedString &cmd)
{
	cmd.cut_out("\\\\+");

	for (auto it = cmd.begin(); it != cmd.end(); it++){
		if (!it->was_matched_against("\\\\+"))
			continue;

		if (it->value.size() % 2 == 1){
			/* the back slashes are actually escaping something,
			 * move that character to this part */
			auto next = it;
			next++;

			if (next == cmd.end()){
				throw "Backslash '\' at end of input";
			}

			it->value.push_back(next->value[0]);
			next->value.erase(next->value.begin());
		}
	}
}

static void blend_escapes (/*inval*/ SectionedString &cmd)
{
	for (auto it = cmd.begin(); it != cmd.end(); it++){
		if (!it->was_matched_against("\\\\+"))
			continue;

		assert(it->value.size() % 2 == 0);
		/* erase the foremost backslash from every pair */
		auto c = it->value.begin();
		while (c != it->value.end()){
			c = it->value.erase(c);
			c++;
		}

		cmd.blend(it);
	}
}

static void delete_whitespace (SectionedString &cmd) noexcept
{
	auto it = cmd.begin();
	while (it != cmd.end()){
		if (it->was_matched_against(REG_WHITESPACE))
			it = cmd.parts.erase(it);
		else
			it++;
	}
}

std::list<Token> tokenize (const char *s)
{
	SectionedString cmd(s);

	escape(cmd);

	cmd.cut_out(REG_SINGLE_QUOTES);
	cmd.cut_out(REG_DOUBLE_QUOTES);
	cmd.cut_out(REG_WHITESPACE);
	cmd.cut_out(REG_LOOP);
	cmd.cut_out(REG_SEMICOLON);
	cmd.cut_out(REG_ANDOR);
	cmd.cut_out(REG_PIPE);
	cmd.cut_out(REG_OPEN_BRACKET);
	cmd.cut_out(REG_CLOSE_BRACKET);
	cmd.cut_out(REG_REDIRECTION);
//	cmd.cut_out(REG_BACKGROUND);

	blend_escapes(cmd);

	/* blend the quotes back */
	for (auto it = cmd.begin(); it != cmd.end(); it++){
		if (it->type == LEFT)
			continue;

		if (strcmp(it->matched_against, REG_DOUBLE_QUOTES) == 0
			|| strcmp(it->matched_against, REG_SINGLE_QUOTES) == 0)
		{
			/* strip the quote characters */
			it->value.erase(0, 1);
			it->value.erase(it->value.size()-1, 1);

			cmd.blend(it);
		}
	}

	/* Whitespaces were only relevant as separators of tokens, they are
	 * no longer needed */
	delete_whitespace(cmd);

	return cmd.parts;
}
