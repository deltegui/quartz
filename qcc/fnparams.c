#include "fnparams.h"
#include <stdlib.h>

#define PARAMS_GROW_CAPACITY(params) (params->capacity == 0 ? 2 : params->capacity + 2)

void init_param_array(ParamArray* params) {
    params->size = 0;
    params->capacity = 0;
    params->params = NULL;
}

void free_param_array(ParamArray* params) {
    if (params->params == NULL) {
        return;
    }
    free(params->params);
}

void param_array_add(ParamArray* params, Param param) {
    if (params->size + 1 > params->capacity) {
        params->capacity = PARAMS_GROW_CAPACITY(params);
        params->params = (Param*) realloc(params->params, sizeof(Param) * params->capacity);
    }
    assert(params->capacity > 0);
    assert(params->params != NULL);
    params->params[params->size] = param;
    params->size++;
}
