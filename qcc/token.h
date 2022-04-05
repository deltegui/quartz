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
    TOKEN_LEFT_BRAKET,
    TOKEN_RIGHT_BRAKET,
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

    // Two character tokens
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
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NIL,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_NEW,
    TOKEN_TYPEDEF,
    TOKEN_IMPORT,
    TOKEN_CLASS,
    TOKEN_PUBLIC,
    TOKEN_SELF,
    TOKEN_CAST,

    TOKEN_TYPE_ANY,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_VOID,
    TOKEN_TYPE_NIL
} TokenKind;

typedef struct {
    const char* path;
    int path_length;
    const char* source;
} FileImport;

typedef struct {
    TokenKind kind;
    const char* start;
    int length;
    uint32_t line;
    uint32_t column;
    FileImport ctx;
} Token;

#define VECTOR_AS_TOKENS(vect) VECTOR_AS(vect, Token)
#define VECTOR_ADD_TOKEN(vect, token) VECTOR_ADD(vect, token, Token)

#endif
