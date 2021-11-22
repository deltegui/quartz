#ifndef QUARTZ_STDLIB_H_
#define QUARTZ_STDLIB_H_

#include "../native.h"

void init_stdlib();
void free_stdlib();
NativeImport* import_stdlib(const char* import_name, int length);

#endif
