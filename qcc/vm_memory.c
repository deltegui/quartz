#include "vm_memory.h"
#include "common.h"
#include "object.h"
#include "vm.h"

#ifdef GC_DEBUG
#include "debug.h"
#endif

static void collect_garbage();

static void mark();
static void mark_roots();
static void mark_stack();
static void mark_globals();
static void mark_callframes();

static void trace_objects();
static void blacken_object(Obj* obj);

static void sweep();

bool is_gc_running = false;

#define GC_CAN_RUN() (qvm.is_running && !is_gc_running)

void* qvm_realloc(void* ptr, size_t old_size, size_t size) {
    printf("qvm realloc called!\n");
#ifdef STRESS_GC
    if (size > old_size && GC_CAN_RUN()) {
        printf("STRESS GC ");
        collect_garbage();
    }
#else
    if (size > old_size && GC_CAN_RUN()) {
        collect_garbage();
    }
#endif
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

static void collect_garbage() {
#ifdef GC_DEBUG
    printf("-- gc begins\n");
#endif
    is_gc_running = true;
    mark();
    sweep();
    is_gc_running = false;
#ifdef GC_DEBUG
    printf("-- gc ends\n");
#endif
}

static void mark() {
#ifdef GC_DEBUG
    printf("-- gc start of mark phase\n");
#endif
    // Compiler roots is not needed because the GC is not
    // running until the VM is not running (checking is_running
    // field in VM)
    mark_roots();
    trace_objects();
#ifdef GC_DEBUG
    printf("-- gc end of mark phase\n");
#endif
}

static void mark_roots() {
#ifdef GC_DEBUG
    printf("-- gc start marking roots\n");
#endif
    mark_stack();
    mark_globals();
    mark_callframes();
#ifdef GC_DEBUG
    printf("-- gc end marking roots\n");
#endif
}

static void mark_stack() {
#ifdef GC_DEBUG
    printf("\t-- gc start marking stack\n");
#endif
    for (Value* current = qvm.stack; current != qvm.stack_top; current++) {
        mark_value(*current);
    }
#ifdef GC_DEBUG
    printf("\t-- gc end marking stack\n");
#endif
}

static void mark_globals() {
#ifdef GC_DEBUG
    printf("\t-- gc start marking globals\n");
#endif
    mark_table(&qvm.globals);
#ifdef GC_DEBUG
    printf("\t-- gc end marking globals\n");
#endif
}

static void mark_callframes() {
#ifdef GC_DEBUG
    printf("\t-- gc start marking callframes\n");
#endif
    for (int i = 0; i < qvm.frame_count; i++) {
        mark_object((Obj*)qvm.frames[i].func);
    }
#ifdef GC_DEBUG
    printf("\t-- gc end marking callframes\n");
#endif
}

static void trace_objects() {
#ifdef GC_DEBUG
    printf("-- gc start tracing\n");
#endif
    for (int i = 0; i < qvm.gray_stack_size; i++) {
        Obj* current = qvm_pop_gray();
#ifdef GC_DEBUG
    printf("Tracing gray object: ");
    print_object(current);
    printf("\n");
#endif
        blacken_object(current);
    }
#ifdef GC_DEBUG
    printf("-- gc end trancing\n");
#endif
}

static void blacken_object(Obj* obj) {
    switch (obj->kind) {
    case OBJ_STRING:
        break;
    case OBJ_FUNCTION: {
        ObjFunction* fn = OBJ_AS_FUNCTION(obj);
        mark_object((Obj*)fn->name);
        mark_valuearray(&fn->chunk.constants);
        for (int i = 0; i < fn->upvalue_count; i++) {
            Upvalue* current = &fn->upvalues[i];
            if (current->is_closed) {
                mark_object((Obj*)current->closed);
            }
        }
        break;
    }
    case OBJ_CLOSED: {
        ObjClosed* closed = OBJ_AS_CLOSED(obj);
        mark_value(closed->value);
        break;
    }
    }
}

static void sweep() {
#ifdef GC_DEBUG
    printf("-- gc start sweep\n");
#endif
    if (qvm.objects == NULL) {
        return;
    }

    if (qvm.objects->is_marked) {
        qvm.objects->is_marked = false;
    } else {
#ifdef GC_DEBUG
    printf("Sweeping object : ");
    print_object(qvm.objects);
    printf("\n");
#endif
        Obj* next = qvm.objects->next;
        free_object(qvm.objects);
        qvm.objects = next;
    }

    Obj* prev = qvm.objects;
    while (prev != NULL && prev->next != NULL) {
        Obj* current = prev->next;
        if (current->is_marked) {
            current->is_marked = false;
            prev = current;
            continue;
        }
#ifdef GC_DEBUG
    printf("Sweeping object : ");
    print_object(current);
    printf("\n");
#endif
        Obj* next = current->next;
        free_object(current);
        prev->next = next;
    }
#ifdef GC_DEBUG
    printf("-- gc end sweep\n");
#endif
}
