#include "stdlib.h"
#include "../ctable.h"

#include "qstdio.h"

void populate_imports();

CTable stdlib_imports;

void init_stdlib() {
    init_ctable(&stdlib_imports, sizeof(NativeImport));
    populate_imports();
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