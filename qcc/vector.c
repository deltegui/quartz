#include "vector.h"
#include <stdlib.h>

#define VECTOR_GROW_CAPACITY(params) (params->capacity == 0 ? 2 : params->capacity + 2)

void init_vector(Vector* vect) {
    vect->size = 0;
    vect->capacity = 0;
    vect->elements = NULL;
}

void free_vector(Vector* vect) {
    if (vect->size == 0) {
        return;
    }
    free(vect->elements);
}

void vector_add(Vector* vect, VElement element) {
    if (vect->size + 1 > vect->capacity) {
        vect->capacity = VECTOR_GROW_CAPACITY(vect);
        vect->elements = (VElement*) realloc(vect->elements, sizeof(VElement) * vect->capacity);
    }
    assert(vect->capacity > 0);
    assert(vect->elements != NULL);
    vect->elements[vect->size] = element;
    vect->size++;
}
