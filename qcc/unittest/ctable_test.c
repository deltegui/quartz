#include <stdarg.h>
#include "./common.h"
#include "../ctable.h"

#define CTABLE_TEST(type, ...) do {\
    CTable table;\
    init_ctable(&table, sizeof(type));\
    __VA_ARGS__\
    free_ctable(&table);\
} while(false)

struct CharEntry {
    CTableEntry;
    char* start;
    int length;
};

void ctable_can_insert_and_search_for_elements() {
    CTableKey a_key = create_ctable_key("a", 1);
    CTableKey b_key = create_ctable_key("b", 1);

    CTABLE_TEST(char*, {
        ctable_set(&table, a_key, "hola soy a");
        ctable_set(&table, b_key, "hola soy b");

        CTableEntry* a = ctable_find(&table, &a_key);
        assert_true(memcmp(a->value, "hola soy a", 10) == 0);

        CTableEntry* b = ctable_find(&table, &b_key);
        assert_true(memcmp(b->value, "hola soy b", 10) == 0);
    });
}

/*void ctable_can_insert_values() {
    CTableKey a_key = create_ctable_key("a", 1);
    CTableKey b_key = create_ctable_key("b", 1);

    CTABLE_TEST(int, {
        ctable_set(&table, a_key, 55);
        ctable_set(&table, b_key, 69);

        CTableEntry* a = ctable_find(&table, &a_key);
        assert_true(a->value == 55);

        CTableEntry* b = ctable_find(&table, &b_key);
        assert_true(b->value == 69);
    });
}*/

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(ctable_can_insert_and_search_for_elements)
        //cmocka_unit_test(ctable_can_insert_values),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}