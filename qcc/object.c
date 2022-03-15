#include "object.h"
#include <string.h>
#include "vm_memory.h"
#include "vm.h"
#include "table.h"

static Obj* alloc_obj(size_t size, ObjKind kind, Type* type);
static ObjString* alloc_string(const char* chars, int length, uint32_t hash);

#define ALLOC_OBJ(obj_type, kind, type) (obj_type*) alloc_obj(sizeof(obj_type), kind, type)
#define ALLOC_STR(length) (ObjString*) alloc_obj(sizeof(ObjString) + sizeof(char) * length, OBJ_STRING, CREATE_TYPE_STRING())

static Obj* alloc_obj(size_t size, ObjKind kind, Type* type) {
    Obj* obj = (Obj*) qvm_realloc(NULL, 0, size);
    obj->kind = kind;
    obj->type = type;
    obj->is_marked = false;
    obj->next = qvm.objects;
    qvm.objects = obj;
    return obj;
}

ObjFunction* new_function(const char* name, int length, int upvalues, Type* type) {
    ObjFunction* func = (ObjFunction*) alloc_obj(
        sizeof(ObjFunction) + (sizeof(Upvalue) * upvalues),
        OBJ_FUNCTION,
        type);
    init_chunk(&func->chunk);
    func->arity = 0;
    func->name = copy_string(name, length);
    func->upvalue_count = upvalues;
    for (int i = 0; i < upvalues; i++) {
        func->upvalues[i].is_closed = false;
    }
    return func;
}

void function_close_upvalue(ObjFunction* const function, int upvalue, ObjClosed* closed) {
    function->upvalues[upvalue].is_closed = true;
    function->upvalues[upvalue].closed = closed;
}

void function_open_upvalue(ObjFunction* const function, int upvalue, Value* value) {
    function->upvalues[upvalue].is_closed = false;
    function->upvalues[upvalue].open = value;
}

Value* function_get_upvalue(ObjFunction* const function, int slot) {
    Value* target;
    if (function->upvalues[slot].is_closed) {
        target = &function->upvalues[slot].closed->value;
    } else {
        target = function->upvalues[slot].open;
    }
    return target;
}

ObjArray* new_array(Type* inner) {
    assert(inner != NULL);
    Type* type = create_type_array(inner);
    ObjArray* arr = ALLOC_OBJ(ObjArray, OBJ_ARRAY, type);
    init_valuearray(&arr->elements);
    return arr;
}

ObjClosed* new_closed(Value value) {
    // TODO again, which type should be a ObjClosed (look vm.c too)
    ObjClosed* closed = ALLOC_OBJ(ObjClosed, OBJ_CLOSED, CREATE_TYPE_UNKNOWN());
    closed->value = value;
    return closed;
}

ObjNative* new_native(const char* name, int length, native_fn_t function, Type* type) {
    ObjNative* native = ALLOC_OBJ(ObjNative, OBJ_NATIVE, type);
    native->name = name;
    native->length = length;
    native->arity = type->function.param_types.size;
    native->function = function;
    return native;
}

ObjClass* new_class(const char* name, int length, Type* type) {
    ObjClass* klass = ALLOC_OBJ(ObjClass, OBJ_CLASS, type);
    klass->name = copy_string(name, length);
    init_valuearray(&klass->instance);
    return klass;
}

ObjInstance* new_instance(ObjClass* origin) {
    ObjInstance* instance = ALLOC_OBJ(ObjInstance, OBJ_INSTANCE, origin->obj.type);
    instance->klass = origin;
    init_valuearray(&instance->props);
    stack_push(OBJ_VALUE(instance, instance->obj.type));
    valuearray_deep_copy(&origin->instance, &instance->props);
    stack_pop();
    return instance;
}

ObjBindedMethod* new_binded_method(ObjInstance* instance, Obj* method) {
    ObjBindedMethod* binded = ALLOC_OBJ(ObjBindedMethod, OBJ_BINDED_METHOD, method->type);
    binded->instance = instance;
    binded->method = method;
    return binded;
}

// TODO change the value array to be directly in obj?
Value object_get_property(ObjInstance* obj, uint8_t index) {
    assert(index < obj->props.size);
    return obj->props.values[index];
}

void object_set_property(ObjInstance* obj, uint8_t index, Value val) {
    assert(index < obj->props.size);
    obj->props.values[index] = val;
}

static ObjString* alloc_string(const char* chars, int length, uint32_t hash) {
    ObjString* obj_str = ALLOC_STR(length + 1);
    obj_str->length = length;
    memcpy((char*)obj_str->chars, chars, length);
    obj_str->chars[length] = '\0';
    obj_str->hash = hash;
    return obj_str;
}

uint32_t hash_string(const char* chars, int length) {
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
    stack_push(OBJ_VALUE(str, CREATE_TYPE_STRING())); // We need to GC discover our new string.
    table_set(&qvm.strings, str, NIL_VALUE());
    stack_pop();
    return str;
}

ObjString* concat_string(ObjString* first, ObjString* second) {
    int concat_length = first->length + second->length;
    char buffer[concat_length];
    memcpy(buffer, first->chars, first->length);
    memcpy(((char*)buffer) + first->length, second->chars, second->length);
    return copy_string(buffer, concat_length);
}

void print_object(Obj* const obj) {
    switch (obj->kind) {
    case OBJ_STRING: {
        printf("'%s'", OBJ_AS_CSTRING(obj));
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction* fn = OBJ_AS_FUNCTION(obj);
        printf("<fn '%s' ", OBJ_AS_CSTRING(fn->name));
        TYPE_PRINT(obj->type);
        printf(">");
        break;
    }
    case OBJ_CLOSED: {
        ObjClosed* closed = OBJ_AS_CLOSED(obj);
        printf("<Closed [");
        value_print(closed->value);
        printf("]>");
        break;
    }
    case OBJ_NATIVE: {
        ObjNative* native = OBJ_AS_NATIVE(obj);
        printf(
            "<Native '%.*s' ",
            native->length,
            native->name);
        TYPE_PRINT(obj->type);
        printf(">");
        break;
    }
    case OBJ_CLASS: {
        ObjClass* klass = OBJ_AS_CLASS(obj);
        printf("<Class '%s'>", OBJ_AS_CSTRING(klass->name));
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance* instance = OBJ_AS_INSTANCE(obj);
        printf("<Instance of class '%s'>", OBJ_AS_CSTRING(instance->klass->name));
        break;
    }
    case OBJ_BINDED_METHOD: {
        ObjBindedMethod* binded = OBJ_AS_BINDED_METHOD(obj);
        printf("Binded Method: ");
        print_object(binded->method);
        break;
    }
    case OBJ_ARRAY: {
        ObjArray* arr = OBJ_AS_ARRAY(obj);
        printf("<Array with %d elements: ", arr->elements.size);
        TYPE_PRINT(obj->type);
        printf(">");
        break;
    }
    }
}

bool object_is_kind(Obj* const obj, ObjKind kind) {
    return obj->kind == kind;
}

void mark_object(Obj* const obj) {
    if (obj == NULL) {
        return;
    }
    if (obj->is_marked) {
        return;
    }
#ifdef GC_DEBUG
    printf("   Marked object: ");
    print_object(obj);
    printf("\n");
#endif
    obj->is_marked = true;
    qvm_push_gray(obj);
}
