#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

namespace Quartz {

const char* token_type_str(TokenType type) {
	switch (type) {
	case TokenType::End: return "End";
	case TokenType::Error: return "Error";
	case TokenType::Plus: return "Plus";
	case TokenType::Minus: return "Minus";
	case TokenType::Star: return "Star";
	case TokenType::Slash: return "Slash";
	case TokenType::Percent: return "Percent";
	case TokenType::LeftParen: return "LeftParen";
	case TokenType::RightParen: return "RightParen";
	case TokenType::Dot: return "Dot";
	case TokenType::Integer: return "Integer";
	case TokenType::Float: return "Float";
	default: return "Unknown";
	}
}

void print_token(Token token) {
	char* substr = (char*)malloc(token.length + 1);
	if (substr == NULL) {
		fprintf(stderr, "Error while allocating memory for token value\n");
		return;
	}
	memcpy(substr, token.start, token.length);
	substr[token.length] = '\0';
	fprintf(stderr, "Token{\n\tType: %s,\n\tLine: %d,\n\tValue: %s,\n\tLength: %d,\n}\n", token_type_str(token.type), token.line, substr, token.length);
	free(substr);
}

}