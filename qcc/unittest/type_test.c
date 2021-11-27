#include <stdarg.h>
#include "./common.h"
#include "../type.h"

#define TYPE_POOL(...) do {\
    init_type_pool();\
    __VA_ARGS__;\
    free_type_pool();\
} while (false)

static void simple_types_share_same_pointer() {
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

static void simple_type_should_be_equal() {
    TYPE_POOL({
        Type* a = CREATE_TYPE_BOOL();
        Type* b = CREATE_TYPE_BOOL();
        assert_true(type_equals(a, b));
    });
}

static void simple_type_should_be_not_equal() {
    TYPE_POOL({
        Type* a = CREATE_TYPE_BOOL();
        Type* b = CREATE_TYPE_NUMBER();
        assert_false(type_equals(a, b));
    });
}

static void complex_types_should_be_equal() {
    TYPE_POOL({
        Type* a = create_type_function();
        VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(a), CREATE_TYPE_NUMBER());
        VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(a), CREATE_TYPE_BOOL());
        TYPE_FN_RETURN(a) = CREATE_TYPE_VOID();

        Type* b = create_type_function();
        VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(b), CREATE_TYPE_NUMBER());
        VECTOR_ADD_TYPE(&TYPE_FN_PARAMS(b), CREATE_TYPE_BOOL());
        TYPE_FN_RETURN(b) = CREATE_TYPE_VOID();

        assert_true(type_equals(a, b));
    });
}

static void two_typealias_should_be_equal() {
    TYPE_POOL({
        Type* a = create_type_alias("Hola", 4, CREATE_TYPE_NUMBER());
        Type* b = CREATE_TYPE_NUMBER();
        assert_true(type_equals(a, b));
    });
}

static void two_objects_should_be_equals() {
    TYPE_POOL({
        Type* a = create_type_object("Human", 5);
        Type* b = create_type_object("Human", 5);
        assert_true(type_equals(a, b));
    });
}

static void two_objects_are_different() {
    TYPE_POOL({
        Type* a = create_type_object("Human", 5);
        Type* b = create_type_object("Animal", 6);
        assert_false(type_equals(a, b));
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(complex_types_should_be_equal),
        cmocka_unit_test(simple_type_should_be_not_equal),
        cmocka_unit_test(simple_type_should_be_equal),
        cmocka_unit_test(creating_other_types_should_use_pool),
        cmocka_unit_test(simple_types_share_same_pointer),
        cmocka_unit_test(two_typealias_should_be_equal),
        cmocka_unit_test(two_objects_should_be_equals),
        cmocka_unit_test(two_objects_are_different)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
