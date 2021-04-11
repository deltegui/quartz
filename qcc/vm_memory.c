#include "vm_memory.h"
#include "common.h"
#include "object.h"
#include "vm.h"

static void free_object(Obj* obj) {
    switch (obj->kind) {
    case STRING_OBJ: {
        ObjString* str = AS_STRING_OBJ(obj);
        FREE(ObjString, obj);
        break;
    }
    case FUNCTION_OBJ: {
        ObjFunction* func = AS_FUNCTION(obj);
        free_chunk(&func->chunk);
        FREE(ObjFunction, obj);
        break;
    }
    }
}

void free_objects() {
    Obj* current = qvm.objects;
    Obj* next = NULL;
    while (current != NULL) {
        next = current->next;
        free_object(current);
        current = next;
    }
}

void* qvm_realloc(void* ptr, size_t old_size, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}