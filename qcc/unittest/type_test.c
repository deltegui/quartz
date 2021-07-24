#include <stdarg.h>
#include "./common.h"
#include "../type.h"

#define TYPE_POOL(...) do {\
    init_type_pool();\
    __VA_ARGS__;\
    free_type_pool();\
} while (false)

static void simple_types_are_equal() {
    TYPE_POOL({
        Type* first = CREATE_TYPE_UNKNOWN();
        Type* second = CREATE_TYPE_UNKNOWN();
        assert_true(first == second);
    });
}

static void creating_other_types_should_use_pool() {
    TYPE_POOL({
        Type* first = create_type_function();
        Type* second = create_type_function();
        assert_true(first != second);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(creating_other_types_should_use_pool),
        cmocka_unit_test(simple_types_are_equal)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}