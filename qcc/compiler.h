#ifndef QUARTZ_COMPILER_H_
#define QUARTZ_COMPILER_H_

#include "chunk.h"
#include "object.h"
#include "parser.h"
#include "expr.h"
#include "stmt.h"

typedef enum {
    COMPILATION_ERROR,
    PARSING_ERROR,
    TYPE_ERROR,
    COMPILATION_OK,
} CompilationResult;

CompilationResult compile(const char* source, ObjFunction** result);

#endif
