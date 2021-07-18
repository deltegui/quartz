#ifndef QUARTZ_TYPECHECKER_H
#define QUARTZ_TYPECHECKER_H

#include "stmt.h"
#include "symbol.h"

bool typecheck(Stmt* ast, ScopedSymbolTable* symbols);

#endif
