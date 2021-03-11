#include "object.h"
#include <string.h>
#include "vm_memory.h"

ObjString* new_string(Token* token) {
    char* buffer = ALLOC(char, token->length + 1); // @todo Should we use alloc here?
    memcpy(buffer, token->start, token->length);
    buffer[token->length] = '\0';

    ObjString* obj_str = ALLOC(ObjString, 1);
    obj_str->length = token->length;
    obj_str->cstr = buffer;
    return obj_str;
}