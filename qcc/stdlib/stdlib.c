#include "stdlib.h"
#include "../ctable.h"

#include "qstdio.h"

void populate_imports();
void print_loaded_imports();

CTable stdlib_imports;

void init_stdlib() {
    init_ctable(&stdlib_imports, sizeof(NativeImport));
    populate_imports();
#ifdef DEBUG
    print_loaded_imports();
#endif
}

void free_stdlib() {
    free_ctable(&stdlib_imports);
}

NativeImport* import_stdlib(const char* import_name, int length) {
    CTableKey key = create_ctable_key(import_name, length);
    CTableEntry* entry = ctable_find(&stdlib_imports, &key);
    if (entry == NULL) {
        return NULL; // there is no such stdlib import with that name.
    }
    NativeImport* imports = VECTOR_AS(&stdlib_imports.data, NativeImport);
    return &imports[entry->vector_pos];
}

void populate_imports() {
    register_stdio(&stdlib_imports);
}

void print_loaded_imports() {
    NativeImport* imports = VECTOR_AS(&stdlib_imports.data, NativeImport);
    for (uint32_t i = 0; i < stdlib_imports.size; i++) {
        NativeImport current = imports[i];
        printf(
            "[STDLIB] Loaded: '%.*s', with functions: {\n",
            current.length,
            current.name);
        for (int j = 0; j < current.functions_length; j++) {
            printf(
                "[STDLIB] \t'%.*s',\n",
                current.functions[j].length,
                current.functions[j].name);
        }
        printf("[STDLIB] }\n");
    }
}
