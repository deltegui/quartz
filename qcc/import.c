#include "import.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ctable.h"
#include "stdlib/stdlib.h"

#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

CTable modules; // Ctable<FileImport>

#define REGISTER_IMPORT(key, import) CTABLE_SET(&modules, key, import, FileImport)
#define FIND_IMPORT(key) ctable_find(&modules, &key)
#define VECTOR_AS_IMPORTS() VECTOR_AS(&modules.data, FileImport)

static bool is_directory(const char* path);
static bool is_import_loaded(const char* path, int len);
static FileImport find_import(const char* path, int lengh);
static void register_import(FileImport import);
static const char* read_file(const char* source_name);
FileImport import_file(const char* path, int length);
static void free_import(FileImport import);

static bool is_directory(const char* path) {
#if defined(WIN32) || defined(_WIN32)
    bool path_access = _access(path, 0);
#else
    bool path_access = access(path, 0);
#endif

    if (path_access == 0) {
        struct stat path_stat;
        stat(path, &path_stat);
        return S_ISDIR(path_stat.st_mode);
    }
    return false;
}

void init_module_system() {
    init_ctable(&modules, sizeof(FileImport));
}

void free_module_system() {
    FileImport* mods = VECTOR_AS_IMPORTS();
    for (uint32_t i = 0; i < modules.data.size; i++) {
        free_import(mods[i]);
    }
    free_ctable(&modules);
}

static bool is_import_loaded(const char* path, int length) {
    CTableKey key = create_ctable_key(path, length);
    CTableEntry* entry = FIND_IMPORT(key);
    return entry != NULL;
}

static FileImport find_import(const char* path, int length) {
    CTableKey key = create_ctable_key(path, length);
    CTableEntry* entry = FIND_IMPORT(key);
    assert(entry != NULL);
    FileImport* mods = VECTOR_AS_IMPORTS();
    return mods[entry->vector_pos];
}

static void register_import(FileImport import) {
    CTableKey key = create_ctable_key(import.path, import.path_length);
    REGISTER_IMPORT(key, import);
}

// Reads a file from "source_name" and returns a string with
// the contents of the file. The ownership of that string is
// up to you, so delete it. It can also return null, meaning
// that was an error.
static const char* read_file(const char* source_name) {
    if (is_directory(source_name)) {
        fprintf(stderr, "Error while reading source file '%s': Is a directory\n", source_name);
        return NULL;
    }
    FILE* source = fopen(source_name, "r");
    if (source == NULL) {
        fprintf(stderr, "Error while reading source file '%s': ", source_name);
        perror(NULL);
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

Import import(const char* path, int length) {
    Import import;
    NativeImport* native_import = import_stdlib(path, length);
    import.is_native = native_import != NULL;
    if (import.is_native) {
        import.file = import_file(path, length);
    } else {
        import.native = *native_import;
    }
    return import;
}

FileImport import_file(const char* path, int length) {
    if (is_import_loaded(path, length)) {
        return find_import(path, length);
    }

    char* cpy_path = (char*) malloc(sizeof(char) * length + 1);
    memcpy(cpy_path, path, length);
    cpy_path[length] = '\0';

    FileImport import;
    import.path = cpy_path;
    import.path_length = length;
    import.source = read_file(cpy_path);
    import.is_already_loaded = true; // We save it as if was loaded.
    register_import(import);
    import.is_already_loaded = false; // Then we say that this is the first time
    return import;
}

static void free_import(FileImport import) {
    free((void*) import.path);
    if (import.source != NULL) {
        free((void*) import.source);
    }
}
