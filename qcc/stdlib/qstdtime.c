#include "qstdtime.h"
#include <time.h>
#include "../values.h"
#include "../native.h"

static Value stdtime_time(int argc, Value* argv);

void register_stdtime(CTable* table) {
    Type* time_type = create_type_function();\
    time_type->function.return_type = CREATE_TYPE_NUMBER();

    NativeFunction time = (NativeFunction) {
        .name = "time",
        .length = 4,
        .function = stdtime_time,
        .type = time_type,
    };

#define FN_LENGTH 1
    static NativeFunction functions[FN_LENGTH];
    functions[0] = time;

    NativeImport stdtime_import = (NativeImport) {
        .name = "stdtime",
        .length = 7,
        .functions = functions,
        .functions_length = FN_LENGTH,
    };
#undef FN_LENGTH
    CTABLE_SET(
        table,
        create_ctable_key(stdtime_import.name, stdtime_import.length),
        stdtime_import,
        NativeImport);
}

static Value stdtime_time(int argc, Value* argv) {
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
}
