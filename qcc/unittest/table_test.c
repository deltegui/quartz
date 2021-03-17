#include <stdarg.h>
#include <string.h>
#include "./common.h"

#include "../vm.h"
#include "../table.h"
#include "../object.h"

static Entry create_entry(const char* key, double value) {
    ObjString* str = copy_string(key, strlen(key));
    Value val = NUMBER_VALUE(value);
    return (Entry){
        .key = str,
        .value = val,
        .distance = 0,
    };
}

static void start_test_case(Table* table) {
    init_qvm();
    init_table(table);
}

static void finish_test_case(Table* table) {
    free_table(table);
    free_qvm();
}

#define TABLE_TEST(...) do {\
    Table* table;\
    start_test_case(table);\
    __VA_ARGS__\
    finish_test_case(table);\
} while (false)

static void should_insert_and_get_one_element() {
    TABLE_TEST({
        Entry e = create_entry("demo", 5);
        table_set(table, e.key, e.value);
        Value val = table_find(table, e.key);
        assert_true(IS_NUMBER(val));
        assert_float_equal(AS_NUMBER(val), 5, 1);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_insert_and_get_one_element)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}