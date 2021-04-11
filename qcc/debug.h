#ifndef QUARTZ_DEBUG_H
#define QUARTZ_DEBUG_H

// General debug functions, for all parts of the compiler.
// Include this around a ifdef DEBUG / endif.

#include "common.h"
#include "chunk.h"  // for chunk_print
#include "lexer.h"  // for token_print
#include "values.h" // for valuearray_print and vlaues
#include "stmt.h"   // for astprint
#include "symbol.h" // for symbol_table_print

void scoped_symbol_table_print(ScopedSymbolTable* table);
void chunk_print(Chunk* chunk);
void token_print(Token token);
void valuearray_print(ValueArray* values);
void ast_print(Stmt* ast);
void stack_print(Value* stack_top, Value* stack);
void opcode_print(uint8_t op);

#endif
