#include "gtest/gtest.h"

#include "lexer.h"

using namespace Quartz;

typedef struct {
	TokenType type;
	const char* value;
	uint8_t length;
	uint32_t line;
} ExpectedToken;

typedef struct {
	const char* lexer_input;
	ExpectedToken expected;
} LexerTestData;

// Compares a token value with expected value. Since token value
// is a pointer to the input string buffer and it tells you
// the length of the value, we need to create the actual string
// to compare it.
//
// token: Quartz::Token
// expected: char*
#define TOKEN_VALUE_EQ(token, expected)\
	do {\
		char* token_value = (char*) malloc(token.length + 1);\
		memcpy(token_value, token.start, token.length);\
		token_value[token.length] = '\0';\
		EXPECT_TRUE(strcmp(token_value, expected) == 0);\
		free(token_value);\
	} while(false)

// Check if a Quartz::Token is equal to a ExpectedToken
#define TOKEN_EQ(token, expected)\
	EXPECT_EQ(token.line, expected.line);\
	EXPECT_EQ(token.length, expected.length);\
	EXPECT_EQ(token.type, expected.type);\
	TOKEN_VALUE_EQ(token, expected.value)

class SingleTokenTests : public ::testing::TestWithParam<LexerTestData> {};

TEST_P(SingleTokenTests, ShouldCheckSingleTokens) {
	auto params = GetParam();
	auto lexer = Lexer(params.lexer_input);
	auto token = lexer.next_token();
	TOKEN_EQ(token, params.expected);
}

INSTANTIATE_TEST_SUITE_P(
	Lexer,
	SingleTokenTests,
	::testing::Values(
		LexerTestData{
			.lexer_input = "12",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Integer,
				.value = "12",
				.length = 2,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = "12.56",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Float,
				.value = "12.56",
				.length = 5,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = "+",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Plus,
				.value = "+",
				.length = 1,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = "-",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Minus,
				.value = "-",
				.length = 1,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = "*",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Star,
				.value = "*",
				.length = 1,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = "/",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Slash,
				.value = "/",
				.length = 1,
				.line = 1,
			},
		},
		LexerTestData{
			.lexer_input = ".",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::Dot,
				.value = ".",
				.length = 1,
				.line = 1,
			}
		},
		LexerTestData{
			.lexer_input = "(",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::LeftParen,
				.value = "(",
				.length = 1,
				.line = 1,
			}
		},
		LexerTestData{
			.lexer_input = ")",
			.expected = ExpectedToken{
				.type = Quartz::TokenType::RightParen,
				.value = ")",
				.length = 1,
				.line = 1,
			}
		}
	)
);

TEST(Lexer, ShouldReturnTokenEndIfInputIsEmpty) {
	auto lexer = Lexer("");
	EXPECT_EQ(lexer.next_token().type, TokenType::End);
}

TEST(Lexer, ShouldCheckSum) {
	auto lexer = Lexer("55 + 3456.22");

	auto current = lexer.next_token();
	auto expected = ExpectedToken{
		.type = Quartz::TokenType::Integer,
		.value = "55",
		.length = 2,
		.line = 1,
	};
	TOKEN_EQ(current, expected);

	current = lexer.next_token();
	expected = ExpectedToken{
		.type = Quartz::TokenType::Plus,
		.value = "+",
		.length = 1,
		.line = 1,
	};
	TOKEN_EQ(current, expected);

	current = lexer.next_token();
	expected = ExpectedToken{
		.type = Quartz::TokenType::Float,
		.value = "3456.22",
		.length = 7,
		.line = 1,
	};
	TOKEN_EQ(current, expected);

	current = lexer.next_token();
	EXPECT_EQ(current.type, Quartz::TokenType::End);
}