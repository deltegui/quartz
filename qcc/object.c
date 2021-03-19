#include "object.h"
#include <string.h>
#include "vm_memory.h"
#include "vm.h"
#include "table.h"

static Obj* alloc_obj(size_t size, ObjType type);
static uint32_t hash_string(const char* chars, int length);

#define ALLOC_OBJ(type, obj_type) (type*) alloc_obj(sizeof(type), obj_type)
#define ALLOC_STR(length) (ObjString*) alloc_obj(sizeof(ObjString) + sizeof(char) * length, STRING_OBJ)

static Obj* alloc_obj(size_t size, ObjType type) {
    Obj* obj = (Obj*) qvm_realloc(NULL, 0, size);
    obj->obj_type = type;
    obj->next = qvm.objects;
    qvm.objects = obj;
    return obj;
}

static ObjString* alloc_string(const char* chars, int length, uint32_t hash) {
    ObjString* obj_str = ALLOC_STR(length + 1);
    obj_str->length = length;
    memcpy((char*)obj_str->cstr, chars, length);
    obj_str->cstr[length] = '\0';
    obj_str->hash = hash;
    return obj_str;
}

static uint32_t hash_string(const char* chars, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= chars[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* copy_string(const char* chars, int length) {
    uint32_t hash = hash_string(chars, length);
    ObjString* interned = table_find_string(&qvm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }
    ObjString* str = alloc_string(chars, length, hash);
    table_set(&qvm.strings, str, NIL_VALUE());
    return str;
}

ObjString* concat_string(ObjString* first, ObjString* second) {
    int concat_length = first->length + second->length;
    char buffer[concat_length];
    memcpy(buffer, first->cstr, first->length);
    memcpy(((char*)buffer) + first->length, second->cstr, second->length);
    return copy_string(buffer, concat_length);
}

void print_object(Obj* obj) {
    switch (obj->obj_type) {
    case STRING_OBJ: {
        printf("'%s'", AS_CSTRING(obj));
        break;
    }
    }
}