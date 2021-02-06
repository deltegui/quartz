#ifndef QUARTZ_LEXER_H
#define QUARTZ_LEXER_H

#include <stdint.h>

namespace Quartz {

enum class TokenType {
	// Special tokens
	End,
	Error,

	// Single character tokens
	Plus,
	Minus,
	Star,
	Slash,
	Percent,
	LeftParen,
	RightParen,
	Dot,

	// Multi-character tokens
	Integer,
	Float,
};

typedef struct {
	TokenType type;
	const char* start;
	uint8_t length;
	uint32_t line;
} Token;

void print_token(Token token);

class Lexer {
	const char* start;
	const char* current;
	int line;

	void skip_whitespaces();
	void advance();
	bool match_next(char next);
	char peek();
	bool is_at_end();
	bool is_numeric();
	bool match(char current);
	Token scan_number();
	Token create_token(TokenType type);

public:
	Lexer(const char* buffer);
	Token next_token();
};

}

#endif