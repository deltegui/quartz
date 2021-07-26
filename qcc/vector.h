#ifndef QUARTZ_VECTOR_H_
#define QUARTZ_VECTOR_H_

#include "common.h"

typedef struct {
    uint32_t size;
    uint32_t capacity;
    size_t element_size;
    void* elements;
} Vector;

void init_vector(Vector* const vect, size_t element_size);
void free_vector(Vector* const vect);
uint32_t vector_next_add_position(Vector* const vect);

#define VECTOR_AS(vect, element_type) ((element_type*) (vect)->elements)

#define VECTOR_ADD(vect, element, element_type) do {\
    uint32_t pos = vector_next_add_position(vect);\
    element_type* transformed = (element_type*) (vect)->elements;\
    transformed[pos] = element;\
} while (false)

#endif
