#include "module.h"
#include <string.h>
#include "ctable.h"

CTable modules; // Ctable<Module>

#define REGISTER_MODULE(key, module) CTABLE_SET(&modules, key, module, Module)
#define FIND_MODULE(key) ctable_find(&modules, &key)
#define VECTOR_AS_MODULES() VECTOR_AS(&modules.data, Module)

static bool is_module_loaded(const char* path, int len);
static Module find_module(const char* path, int lengh);
static void register_module(Module module);
static const char* read_file(const char* source_name);
static void free_module(Module module);

void init_module_system() {
    init_ctable(&modules, sizeof(Module));
}

void free_module_system() {
    Module* mods = VECTOR_AS_MODULES();
    for (uint32_t i = 0; i < modules.data.size; i++) {
        free_module(mods[i]);
    }
    free_ctable(&modules);
}

static bool is_module_loaded(const char* path, int length) {
    CTableKey key = create_ctable_key(path, length);
    CTableEntry* entry = FIND_MODULE(key);
    return entry != NULL;
}

static Module find_module(const char* path, int length) {
    CTableKey key = create_ctable_key(path, length);
    CTableEntry* entry = FIND_MODULE(key);
    assert(entry != NULL);
    Module* mods = VECTOR_AS_MODULES();
    return mods[entry->vector_pos];
}

static void register_module(Module module) {
    CTableKey key = create_ctable_key(module.path, module.path_length);
    REGISTER_MODULE(key, module);
}

// Reads a file from "source_name" and returns a string with
// the contents of the file. The ownership of that string is
// up to you, so delete it. It can also return null, meaning
// that was an error.
static const char* read_file(const char* source_name) {
    FILE* source = fopen(source_name, "r");
    if (source == NULL) {
        fprintf(stderr, "Error while reading source file: \n");
        return NULL;
    }
    fseek(source, 0, SEEK_END);
    size_t size = ftell(source);
    fseek(source, 0, SEEK_SET);
    // Size of the file plus \0 character
    char* buffer = (char*) malloc(size + 1);
    if (buffer == NULL) {
        fclose(source);
        fprintf(stderr, "Error while allocating file buffer!\n");
        return NULL;
    }
    fread(buffer, 1, size, source);
    buffer[size] = '\0';
    fclose(source);
    return buffer;
}

Module module_read(const char* path, int length) {
    if (is_module_loaded(path, length)) {
        return find_module(path, length);
    }

    char* cpy_path = (char*) malloc(sizeof(char) * length + 1);
    memcpy(cpy_path, path, length);
    cpy_path[length] = '\0';

    Module module;
    module.path = cpy_path;
    module.path_length = length;
    module.source = read_file(cpy_path);
    if (module.source == NULL) {
        fprintf(stderr, "File not found: '%.*s'", length, path);
    }
    module.is_already_loaded = true; // We save it as if was loaded.
    register_module(module);
    module.is_already_loaded = false; // Then we say that this is the first time
    return module;
}

static void free_module(Module module) {
    free((void*) module.path);
    if (module.source != NULL) {
        free((void*) module.source);
    }
}
