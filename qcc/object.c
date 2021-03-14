#include "object.h"
#include <string.h>
#include "vm_memory.h"
#include "vm.h"

static Obj* alloc_obj(size_t size, ObjType type);

#define ALLOC_OBJ(type, obj_type) (type*) alloc_obj(sizeof(type), obj_type)
#define ALLOC_STR(length) (ObjString*) alloc_obj(sizeof(ObjString) + sizeof(char) * length, STRING_OBJ)

static Obj* alloc_obj(size_t size, ObjType type) {
    Obj* obj = (Obj*) qvm_realloc(NULL, 0, size);
    obj->obj_type = type;
    obj->next = qvm.objects;
    qvm.objects = obj;
    return obj;
}

ObjString* copy_string(const char* str, int length) {
    ObjString* obj_str = ALLOC_STR(length + 1);
    obj_str->length = length;
    memcpy((char*)obj_str->cstr, str, length);
    obj_str->cstr[length] = '\0';
    return obj_str;
}

void print_object(Obj* obj) {
    switch (obj->obj_type) {
    case STRING_OBJ: {
        printf("%s", AS_CSTRING(obj));
        break;
    }
    }
}