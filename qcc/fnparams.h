#ifndef QUARTZ_FNPARAMS
#define QUARTZ_FNPARAMS

#include "lexer.h"
#include "type.h"

typedef struct {
    union {
        Token identifier;
        Type type;
    };
} Param;

typedef struct {
    int size;
    int capacity;
    Param* params;
} ParamArray;

#define PARAM_ARRAY_ADD_TYPE(params, type) param_array_add(params, ((Param){ .type = type }));
#define PARAM_ARRAY_ADD_TOKEN(params, token) param_array_add(params, ((Param){ .identifier = token }));

void init_param_array(ParamArray* params);
void free_param_array(ParamArray* params);
void param_array_add(ParamArray* params, Param param);

#endif
