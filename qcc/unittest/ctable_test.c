#include <stdarg.h>
#include "./common.h"
#include "../ctable.h"

#define CTABLE_TEST(type, ...) do {\
    CTable table;\
    init_ctable(&table, sizeof(type));\
    __VA_ARGS__\
    free_ctable(&table);\
} while(false)

#define CTABLE_SET_STR(table, key, value) CTABLE_SET(table, key, value, char*)
#define CTABLE_ENTRY_RESOLVE_STR(table, entry, dst) CTABLE_ENTRY_RESOLVE_AS(table, entry, dst, char*)

void ctable_can_insert_and_search_for_elements() {
    CTableKey a_key = create_ctable_key("a", 1);
    CTableKey b_key = create_ctable_key("b", 1);

    CTABLE_TEST(char*, {
        CTABLE_SET_STR(&table, a_key, "hola soy a");
        CTABLE_SET_STR(&table, b_key, "hola soy b");

        CTableEntry* a = ctable_find(&table, &a_key);
        char* a_str;
        CTABLE_ENTRY_RESOLVE_STR(&table, a, &a_str);
        assert_true(memcmp(a_str, "hola soy a", 10) == 0);

        CTableEntry* b = ctable_find(&table, &b_key);
        char* b_str;
        CTABLE_ENTRY_RESOLVE_STR(&table, b, &b_str);
        assert_true(memcmp(b_str, "hola soy b", 10) == 0);
    });
}

#define CTABLE_SET_INT(table, key, value) CTABLE_SET(table, key, value, int)
#define CTABLE_ENTRY_RESOLVE_INT(table, entry, dst) CTABLE_ENTRY_RESOLVE_AS(table, entry, dst, int)

void ctable_can_insert_values() {
    CTableKey a_key = create_ctable_key("a", 1);
    CTableKey b_key = create_ctable_key("b", 1);

    CTABLE_TEST(int, {
        CTABLE_SET_INT(&table, a_key, 55);
        CTABLE_SET_INT(&table, b_key, 69);

        CTableEntry* a = ctable_find(&table, &a_key);
        int a_int;
        CTABLE_ENTRY_RESOLVE_INT(&table, a, &a_int);
        assert_true(a_int == 55);

        CTableEntry* b = ctable_find(&table, &b_key);
        int b_int;
        CTABLE_ENTRY_RESOLVE_INT(&table, b, &b_int);
        assert_true(b_int == 69);
    });
}

void ctable_can_iterate_over_values() {
    CTableKey a_key = create_ctable_key("a", 1);
    CTableKey b_key = create_ctable_key("b", 1);
    CTableKey c_key = create_ctable_key("c", 1);
    CTableKey d_key = create_ctable_key("d", 1);

    CTABLE_TEST(int, {
        CTABLE_SET_INT(&table, a_key, 0);
        CTABLE_SET_INT(&table, b_key, 1);
        CTABLE_SET_INT(&table, c_key, 2);
        CTABLE_SET_INT(&table, d_key, 3);

        int times = 0;
        CTABLE_FOREACH(&table, int, {
            times++;
            assert_true(current == i);
        });
        assert_true(times == 4);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(ctable_can_insert_and_search_for_elements),
        cmocka_unit_test(ctable_can_insert_values),
        cmocka_unit_test(ctable_can_iterate_over_values)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}