#ifndef QUARTZ_TOKEN_H_
#define QUARTZ_TOKEN_H_

#include "common.h"
#include "vector.h"

typedef enum {
    // Special tokens
    TOKEN_END,
    TOKEN_ERROR,

    // Single character tokens
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_DOT,
    TOKEN_BANG,
    TOKEN_EQUAL,
    TOKEN_LOWER,
    TOKEN_GREATER,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,

    // Two character tokens
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LOWER_EQUAL,
    TOKEN_GREATER_EQUAL,

    // Multi-character tokens
    TOKEN_RETURN,
    TOKEN_FUNCTION,
    TOKEN_VAR,
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_STRING,
    TOKEN_PRINT,
    TOKEN_IDENTIFIER,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_TYPEDEF,

    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_VOID,
    TOKEN_TYPE_NIL
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    uint8_t length;
    uint32_t line;
    uint32_t column;
} Token;

#define VECTOR_AS_TOKENS(vect) VECTOR_AS(vect, Token)
#define VECTOR_ADD_TOKEN(vect, token) VECTOR_ADD(vect, token, Token)

#endif
