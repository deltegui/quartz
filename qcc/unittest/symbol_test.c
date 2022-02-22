#include <stdarg.h>
#include <string.h>
#include "./common.h"
#include "../symbol.h"
#include "../typechecker.h"
#include "../debug.h"
#include "../type.h"

#define TABLE(...) do {\
    SymbolTable table;\
    init_type_pool();\
    init_symbol_table(&table);\
    __VA_ARGS__\
    printf(\
        "SymbolTable size: %d, capacity: %d, load factor: %f\n",\
        table.table.size,\
        table.table.capacity,\
        (double)table.table.size / (double)table.table.capacity);\
    free_symbol_table(&table);\
    free_type_pool();\
} while (false)

#define SCOPED_TABLE(...) do {\
    ScopedSymbolTable table;\
    init_type_pool();\
    init_scoped_symbol_table(&table);\
    __VA_ARGS__\
    free_scoped_symbol_table(&table);\
    free_type_pool();\
} while (false)

#define SET(...) do {\
    SymbolSet* set = create_symbol_set();\
    init_type_pool();\
    __VA_ARGS__\
    free_type_pool();\
} while (false)

typedef struct {
    const char* str;
    int length;
    int line;
    Type type;
} symbol_t;

static void assert_key(SymbolName* first, SymbolName* second) {
    assert_int_equal(SYMBOL_NAME_LENGTH(*first), SYMBOL_NAME_LENGTH(*second));
    assert_int_equal(
        memcmp(
            SYMBOL_NAME_START(*first),
            SYMBOL_NAME_START(*second),
            SYMBOL_NAME_LENGTH(*first)),
        0);
    assert_int_equal(SYMBOL_NAME_HASH(*first), SYMBOL_NAME_HASH(*second));
}

static void assert_entry(Symbol* first, Symbol* second) {
    assert_non_null(first);
    assert_non_null(second);
    assert_key(&first->name, &second->name);
    assert_int_equal(first->line, second->line);
    assert_int_equal(first->column, second->column);
    assert_true(type_equals(first->type, second->type));
}

#define SYM_LENGTH 17

symbol_t sym[] = {
    {
        .str = "a",
        .length = 1,
        .line = 12,
        .type = TYPE_STRING
    },
    {
        .str = "b",
        .length = 1,
        .line = 12,
        .type = TYPE_STRING
    },
    {
        .str = "c",
        .length = 1,
        .line = 15,
        .type = TYPE_NUMBER
    },
    {
        .str = "d",
        .length = 1,
        .line = 16,
        .type = TYPE_BOOL
    },
    {
        .str = "e",
        .length = 1,
        .line = 17,
        .type = TYPE_STRING
    },
    {
        .str = "f",
        .length = 1,
        .line = 20,
        .type = TYPE_STRING
    },
    {
        .str = "g",
        .length = 1,
        .line = 19,
        .type = TYPE_STRING
    },
    {
        .str = "h",
        .length = 1,
        .line = 22,
        .type = TYPE_STRING
    },
    {
        .str = "i",
        .length = 1,
        .line = 25,
        .type = TYPE_STRING
    },
    {
        .str = "j",
        .length = 1,
        .line = 28,
        .type = TYPE_STRING
    },
    {
        .str = "k",
        .length = 1,
        .line = 29,
        .type = TYPE_STRING
    },
    {
        .str = "l",
        .length = 1,
        .line = 34,
        .type = TYPE_STRING
    },
    {
        .str = "momo",
        .length = 4,
        .line = 145,
        .type = TYPE_STRING
    },
    {
        .str = "mamon",
        .length = 5,
        .line = 223,
        .type = TYPE_STRING
    },
    {
        .str = "paquito66",
        .length = 9,
        .line = 123,
        .type = TYPE_STRING
    },
    {
        .str = "z",
        .length = 1,
        .line = 89,
        .type = TYPE_STRING
    },
    {
        .str = "y",
        .length = 1,
        .line = 43,
        .type = TYPE_STRING
    },
    {
        .str = "x",
        .length = 1,
        .line = 45,
        .type = TYPE_STRING
    },
};

static void should_insert_symbols() {
    TABLE({
        SymbolName key = create_symbol_name("hello", 5);
        Symbol entry = create_symbol(key, 1, 0, CREATE_TYPE_STRING());
        symbol_insert(&table, entry);
        Symbol* stored = symbol_lookup(&table, &key);
        assert_entry(&entry, stored);
    });
}

static void should_insert_sixteen_elements() {
    TABLE({
        for (int i = 0; i < SYM_LENGTH; i++) {
            symbol_t* symbol = &sym[i];
            Symbol entry = create_symbol(
                create_symbol_name(symbol->str, symbol->length),
                symbol->line,
                0,
                &symbol->type);
            symbol_insert(&table, entry);
        }
        for (int i = 0; i < SYM_LENGTH; i++) {
            symbol_t* symbol = &sym[i];
            SymbolName key = create_symbol_name(symbol->str, symbol->length);
            Symbol expected = create_symbol(
                key,
                symbol->line,
                0,
                &symbol->type);
            Symbol* actual = symbol_lookup(&table, &key);
            assert_entry(&expected, actual);
            free_symbol(&expected);
        }
    });
}

static void should_return_null_if_symbol_does_not_exist() {
    TABLE({
        SymbolName alberto = create_symbol_name("alberto", 7);
        Symbol* entry = symbol_lookup(&table, &alberto);
        assert_null(entry);

        SymbolName manolo = create_symbol_name("manolo", 6);
        Symbol manolo_entry = create_symbol(manolo, 1, 0, CREATE_TYPE_UNKNOWN());
        symbol_insert(&table, manolo_entry);
        entry = symbol_lookup(&table, &alberto);
        assert_null(entry);
    });
}

static void scoped_symbol_should_insert_and_lookup() {
    SCOPED_TABLE({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName b = create_symbol_name("b", 1);
        SymbolName c = create_symbol_name("c", 1);
        SymbolName d = create_symbol_name("d", 1);
        SymbolName e = create_symbol_name("e", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_b = create_symbol(b, 2, 0, CREATE_TYPE_NUMBER());
        Symbol sym_c = create_symbol(c, 3, 0, CREATE_TYPE_NUMBER());
        Symbol sym_d = create_symbol(d, 4, 0, CREATE_TYPE_NUMBER());
        Symbol sym_e = create_symbol(e, 5, 0, CREATE_TYPE_NUMBER());

        /*
        {
            a, b
            {
                c
                {
                    d
                }
            }
            {
                e
            }
        }
        */
        scoped_symbol_insert(&table, sym_a);
        scoped_symbol_insert(&table, sym_b);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_c);
            symbol_create_scope(&table);
                scoped_symbol_insert(&table, sym_d);
            symbol_end_scope(&table);
        symbol_end_scope(&table);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_e);
        symbol_end_scope(&table);

        symbol_reset_scopes(&table);
        assert_non_null(scoped_symbol_lookup(&table, &a));
        assert_non_null(scoped_symbol_lookup(&table, &b));
        assert_null(scoped_symbol_lookup(&table, &c));
        assert_null(scoped_symbol_lookup(&table, &d));
        assert_null(scoped_symbol_lookup(&table, &e));
        symbol_start_scope(&table);
            symbol_start_scope(&table);
                assert_non_null(scoped_symbol_lookup(&table, &a));
                assert_non_null(scoped_symbol_lookup(&table, &b));
                assert_non_null(scoped_symbol_lookup(&table, &c));
                assert_non_null(scoped_symbol_lookup(&table, &d));
                assert_null(scoped_symbol_lookup(&table, &e));
            symbol_end_scope(&table);
            assert_non_null(scoped_symbol_lookup(&table, &a));
            assert_non_null(scoped_symbol_lookup(&table, &b));
            assert_non_null(scoped_symbol_lookup(&table, &c));
            assert_null(scoped_symbol_lookup(&table, &d));
            assert_null(scoped_symbol_lookup(&table, &e));
        symbol_end_scope(&table);
        assert_non_null(scoped_symbol_lookup(&table, &a));
        assert_non_null(scoped_symbol_lookup(&table, &b));
        assert_null(scoped_symbol_lookup(&table, &c));
        assert_null(scoped_symbol_lookup(&table, &d));
        assert_null(scoped_symbol_lookup(&table, &e));
        symbol_start_scope(&table);
            assert_non_null(scoped_symbol_lookup(&table, &a));
            assert_non_null(scoped_symbol_lookup(&table, &b));
            assert_null(scoped_symbol_lookup(&table, &c));
            assert_null(scoped_symbol_lookup(&table, &d));
            assert_non_null(scoped_symbol_lookup(&table, &e));
        symbol_end_scope(&table);
        assert_non_null(scoped_symbol_lookup(&table, &a));
        assert_non_null(scoped_symbol_lookup(&table, &b));
        assert_null(scoped_symbol_lookup(&table, &c));
        assert_null(scoped_symbol_lookup(&table, &d));
        assert_null(scoped_symbol_lookup(&table, &e));
    });
}

static void scoped_symbol_should_do_lookups_with_limited_levels() {
    SCOPED_TABLE({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName c = create_symbol_name("c", 1);
        SymbolName d = create_symbol_name("d", 1);
        SymbolName e = create_symbol_name("e", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_c = create_symbol(c, 3, 0, CREATE_TYPE_NUMBER());
        Symbol sym_d = create_symbol(d, 4, 0, CREATE_TYPE_NUMBER());
        Symbol sym_e = create_symbol(e, 5, 0, CREATE_TYPE_NUMBER());

        /*
        {
            a
            {
                c
                {
                    d
                }
            }
            {
                e
            }
        }
        */
        scoped_symbol_insert(&table, sym_a);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_c);
            symbol_create_scope(&table);
                scoped_symbol_insert(&table, sym_d);
            symbol_end_scope(&table);
        symbol_end_scope(&table);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_e);
        symbol_end_scope(&table);

        symbol_reset_scopes(&table);
        symbol_start_scope(&table);
            symbol_start_scope(&table);

                assert_non_null(scoped_symbol_lookup_levels(&table, &a, 2));
                assert_null(scoped_symbol_lookup_levels(&table, &a, 1));
                assert_null(scoped_symbol_lookup_levels(&table, &a, 0));

                assert_non_null(scoped_symbol_lookup_levels(&table, &c, 1));
                assert_null(scoped_symbol_lookup_levels(&table, &c, 0));

                assert_non_null(scoped_symbol_lookup_levels(&table, &d, 0));
                assert_null(scoped_symbol_lookup_levels(&table, &e, 10));

            symbol_end_scope(&table);
        symbol_end_scope(&table);
        symbol_start_scope(&table);

            assert_non_null(scoped_symbol_lookup_levels(&table, &a, 1));
            assert_null(scoped_symbol_lookup_levels(&table, &a, 0));

            assert_null(scoped_symbol_lookup_levels(&table, &c, 100));

            assert_null(scoped_symbol_lookup_levels(&table, &d, 1000));

            assert_non_null(scoped_symbol_lookup_levels(&table, &e, 0));

        symbol_end_scope(&table);
    });
}

static void scoped_symbol_should_insert_globals() {
    SCOPED_TABLE({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName b = create_symbol_name("b", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_b = create_symbol(b, 2, 0, CREATE_TYPE_NUMBER());

        scoped_symbol_insert(&table, sym_a);
        scoped_symbol_insert(&table, sym_b);
        Symbol* result_a = scoped_symbol_lookup(&table, &a);
        Symbol* result_b = scoped_symbol_lookup(&table, &b);
        assert_entry(&sym_a, result_a);
        assert_entry(&sym_b, result_b);
    });
}

static void scoped_symbol_should_insert_locals() {
    SCOPED_TABLE({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName b = create_symbol_name("b", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_b = create_symbol(b, 2, 0, CREATE_TYPE_NUMBER());

        scoped_symbol_insert(&table, sym_a);
        symbol_create_scope(&table);
        scoped_symbol_insert(&table, sym_b);
        symbol_end_scope(&table);

        symbol_reset_scopes(&table);

        Symbol* result_a = scoped_symbol_lookup(&table, &a);
        symbol_start_scope(&table);
        Symbol* result_b = scoped_symbol_lookup(&table, &b);
        symbol_end_scope(&table);

        assert_entry(&sym_a, result_a);
        assert_entry(&sym_b, result_b);
    });
}

static void upvalue_iterator_should_iterate_over_upvalues() {
    SCOPED_TABLE({
        // First create all we need:
        SymbolName a = create_symbol_name("a", 1);
        SymbolName b = create_symbol_name("b", 1);
        SymbolName c = create_symbol_name("c", 1);
        SymbolName d = create_symbol_name("d", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_b = create_symbol(b, 2, 0, CREATE_TYPE_NUMBER());
        Symbol sym_c = create_symbol(c, 3, 0, create_type_function());
        Symbol sym_d = create_symbol(d, 4, 0, CREATE_TYPE_NUMBER());

        // Create the symbol table to match this code:
        /*
        {
            a, b
            {
                fn c() {
                    d
                }
            }
        }
        */
        scoped_symbol_insert(&table, sym_a);
        scoped_symbol_insert(&table, sym_b);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_c);
            symbol_create_scope(&table);
                scoped_symbol_insert(&table, sym_d);
            symbol_end_scope(&table);
        symbol_end_scope(&table);

        // Now we want to emulate that we are in the c
        // var scope.
        symbol_reset_scopes(&table);
        symbol_start_scope(&table);

        // In that scope thell that the var a is closed by fn
        scoped_symbol_upvalue(&table, &sym_c, &sym_a);

        // Iterate over upvalues
        UpvalueIterator it;
        init_upvalue_iterator(&it, &table, 1);

        // The first one should be a
        Symbol* sym_a_upvalue = upvalue_iterator_next(&it);
        assert_non_null(sym_a_upvalue);
        assert_key(&sym_a_upvalue->name, &a);

        // And that's it, it should be empty.
        Symbol* this_is_null = upvalue_iterator_next(&it);
        assert_null(this_is_null);
    });
}

void symbol_set_should_not_repeat_elements() {
    SET({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName a_clone = create_symbol_name("a", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_a_clone = create_symbol(a_clone, 1, 0, CREATE_TYPE_NUMBER());

        symbol_set_add(set, &sym_a);
        symbol_set_add(set, &sym_a_clone);

        Symbol** elements = SYMBOL_SET_GET_ELEMENTS(set);
        int size = SYMBOL_SET_SIZE(set);
        assert_true(size == 1);
        assert_key(&elements[0]->name, &a);
        free_symbol_set(set);

        free_symbol(&sym_a);
        free_symbol(&sym_a_clone);
    });
}

void symbol_set_should_insert_more_than_one() {
    SET({
        SymbolName a = create_symbol_name("a", 1);
        SymbolName bebe = create_symbol_name("bebe", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        Symbol sym_bebe = create_symbol(bebe, 1, 0, CREATE_TYPE_NUMBER());

        symbol_set_add(set, &sym_a);
        symbol_set_add(set, &sym_bebe);

        Symbol** elements = SYMBOL_SET_GET_ELEMENTS(set);
        int size = SYMBOL_SET_SIZE(set);
        assert_true(size == 2);
        assert_key(&elements[0]->name, &a);
        assert_key(&elements[1]->name, &bebe);
        free_symbol_set(set);

        free_symbol(&sym_a);
        free_symbol(&sym_bebe);
    });
}

void symbol_set_should_not_iterate_over_empty_set() {
    SET({
        int size = SYMBOL_SET_SIZE(set);
        assert_true(size == 0);

        int counter = 0;
        SYMBOL_SET_FOREACH(set, {
            counter = i;
        });
        assert_true(counter == 0);

        free_symbol_set(set);
    });
}

void object_symbols_can_be_added() {
    SCOPED_TABLE({
        SymbolName name = create_symbol_name("Human", 5);
        Type* type = create_type_class("Human", 5);
        Symbol cls_sym = create_symbol(name, 1, 0, type);
        assert_true(cls_sym.kind == SYMBOL_CLASS);

        SymbolName a = create_symbol_name("a", 1);
        Symbol sym_a = create_symbol(a, 1, 0, CREATE_TYPE_NUMBER());
        sym_a.visibility = SYMBOL_VISIBILITY_PUBLIC;

        // Create the symbol table to match this code:
        /*
        { <GLOBAL>
            class
            { <CLS BODY> <- HERE YOU MUST UPDATE CLS BODY
                pub var a = 5;
            }
        }
        */

        scoped_symbol_insert(&table, cls_sym);
        Symbol* obj = scoped_symbol_lookup(&table, &name);
        assert_non_null(obj);
        symbol_create_scope(&table);
            // THE CLS MUST POINT TO ITS BODY TABLE
            scoped_symbol_update_class_body(&table, obj);
            scoped_symbol_insert(&table, sym_a);
        symbol_end_scope(&table);

        symbol_reset_scopes(&table);

        Symbol* recover = scoped_symbol_lookup_str(&table, "Human", 5);
        assert_non_null(recover);
        assert_non_null(recover->klass.body);
        symbol_start_scope(&table);
            Symbol* property = scoped_symbol_lookup_str(&table, "a", 1);
            assert_non_null(property);
            assert_true(property->visibility == SYMBOL_VISIBILITY_PUBLIC);
        symbol_end_scope(&table);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(scoped_symbol_should_insert_locals),
        cmocka_unit_test(scoped_symbol_should_insert_globals),
        cmocka_unit_test(scoped_symbol_should_insert_and_lookup),
        cmocka_unit_test(scoped_symbol_should_do_lookups_with_limited_levels),
        cmocka_unit_test(should_return_null_if_symbol_does_not_exist),
        cmocka_unit_test(should_insert_symbols),
        cmocka_unit_test(should_insert_sixteen_elements),
        cmocka_unit_test(upvalue_iterator_should_iterate_over_upvalues),
        cmocka_unit_test(symbol_set_should_not_repeat_elements),
        cmocka_unit_test(symbol_set_should_insert_more_than_one),
        cmocka_unit_test(symbol_set_should_not_iterate_over_empty_set),
        cmocka_unit_test(object_symbols_can_be_added)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
