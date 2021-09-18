#include "vm_memory.h"
#include "common.h"
#include "object.h"
#include "vm.h"

void* qvm_realloc(void* ptr, size_t old_size, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

static void free_object(Obj* obj) {
    switch (obj->kind) {
    case OBJ_STRING: {
        FREE(ObjString, obj);
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction* func = OBJ_AS_FUNCTION(obj);
        free_chunk(&func->chunk);
        FREE(ObjFunction, obj);
        break;
    }
    case OBJ_CLOSED: {
        FREE(ObjClosed, obj);
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
