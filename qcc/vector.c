#include "vector.h"
#include <stdlib.h>

#define VECTOR_GROW_CAPACITY(cap) ((cap < 8) ? 8 : cap * 2)

static void* compiler_realloc(void* ptr, size_t old_size, size_t size) {
    return realloc(ptr, size);
}

void init_vector(Vector* const vect, size_t element_size) {
    vect->size = 0;
    vect->capacity = 0;
    vect->element_size = element_size;
    vect->elements = NULL;
    vect->f_realloc = compiler_realloc;
}

void free_vector(Vector* const vect) {
    if (vect->capacity == 0) {
        return;
    }
    vect->f_realloc(
        vect->elements,
        vect->element_size * vect->capacity,
        0);
    init_vector(vect, vect->element_size);
}

uint32_t vector_next_add_position(Vector* const vect) {
    if (vect->size + 1 > vect->capacity) {
        uint32_t old_capacity = vect->capacity;
        vect->capacity = VECTOR_GROW_CAPACITY(vect->capacity);
        vect->elements = (void*) vect->f_realloc(
            vect->elements,
            vect->element_size * old_capacity,
            vect->element_size * vect->capacity);
    }
    assert(vect->capacity > 0);
    assert(vect->elements != NULL);
    return vect->size++;
}
