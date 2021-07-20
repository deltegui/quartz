#ifndef QUARTZ_VECTOR_H_
#define QUARTZ_VECTOR_H_

#include "lexer.h"
#include "type.h"

struct _Expr;

typedef struct {
    union {
        Token identifier;
        Type type;
        struct _Expr* expr;
    };
} VElement;

typedef struct {
    int size;
    int capacity;
    VElement* elements;
} Vector;

#define VECTOR_ADD_TYPE(vect, type) vector_add(vect, ((VElement){ .type = type }));
#define VECTOR_ADD_TOKEN(vect, token) vector_add(vect, ((VElement){ .identifier = token }));
#define VECTOR_ADD_EXPR(vect, e) vector_add(vect, ((VElement){ .expr = e }))

void init_vector(Vector* vect);
void free_vector(Vector* vect);
void vector_add(Vector* vect, VElement element);

#endif
