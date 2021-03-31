#ifndef QUARTZ_TYPECHECKER_H
#define QUARTZ_TYPECHECKER_H

#include "stmt.h"

typedef enum {
    NUMBER_TYPE,
    BOOL_TYPE,
    NIL_TYPE,
    STRING_TYPE,
    UNKNOWN_TYPE,
} Type;

bool typecheck(Stmt* ast);

#endif