#include "vector.h"
#include <stdlib.h>

#define VECTOR_GROW_CAPACITY(cap) ((cap < 8) ? 8 : cap * 2)

void init_vector(Vector* vect, size_t element_size) {
    vect->size = 0;
    vect->capacity = 0;
    vect->element_size = element_size;
    vect->elements = NULL;
}

void free_vector(Vector* vect) {
    if (vect->size == 0) {
        return;
    }
    free(vect->elements);
    init_vector(vect, vect->element_size);
}

uint32_t vector_next_add_position(Vector* vect) {
    if (vect->size + 1 > vect->capacity) {
        vect->capacity = VECTOR_GROW_CAPACITY(vect->capacity);
        vect->elements = (void*) realloc(
            vect->elements,
            vect->element_size * vect->capacity);
    }
    assert(vect->capacity > 0);
    assert(vect->elements != NULL);
    return vect->size++;
}
