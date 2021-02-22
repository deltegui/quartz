#ifndef QUARTZ_COMPILER_H
#define QUARTZ_COMPILER_H

#include "chunk.h"
#include "expr.h"

void init_compiler();
void free_compiler();
Chunk* compile(Expr* ast);

#endif