#include <stdarg.h>
#include <string.h>
#include "./common.h"
#include "../symbol.h"
#include "../typechecker.h"
#include "../debug.h"

#define TABLE(...) do {\
    SymbolTable table;\
    init_symbol_table(&table);\
    __VA_ARGS__\
    printf(\
        "SymbolTable size: %d, capacity: %d, load factor: %f\n",\
        table.size,\
        table.capacity,\
        (double)table.size / (double)table.capacity);\
    free_symbol_table(&table);\
} while (false)

#define SCOPED_TABLE(...) do {\
    ScopedSymbolTable table;\
    init_scoped_symbol_table(&table);\
    __VA_ARGS__\
    free_scoped_symbol_table(&table);\
} while (false)

typedef struct {
    const char* str;
    int length;
    int declaration_line;
    Type type;
} symbol_t;

static void assert_key(SymbolName* first, SymbolName* second) {
    assert_int_equal(memcmp(first->str, second->str, first->length), 0);
    assert_int_equal(first->length, second->length);
    assert_int_equal(first->hash, second->hash);
}

static void assert_entry(Symbol* first, Symbol* second) {
    assert_non_null(first);
    assert_non_null(second);
    assert_key(&first->name, &second->name);
    assert_int_equal(first->declaration_line, second->declaration_line);
    assert_int_equal(first->type, second->type);
}

#define SYM_LENGTH 17

symbol_t sym[] = {
    {
        .str = "a",
        .length = 1,
        .declaration_line = 12,
        .type = STRING_TYPE
    },
    {
        .str = "b",
        .length = 1,
        .declaration_line = 12,
        .type = STRING_TYPE
    },
    {
        .str = "c",
        .length = 1,
        .declaration_line = 15,
        .type = NUMBER_TYPE
    },
    {
        .str = "d",
        .length = 1,
        .declaration_line = 16,
        .type = BOOL_TYPE
    },
    {
        .str = "e",
        .length = 1,
        .declaration_line = 17,
        .type = STRING_TYPE
    },
    {
        .str = "f",
        .length = 1,
        .declaration_line = 20,
        .type = STRING_TYPE
    },
    {
        .str = "g",
        .length = 1,
        .declaration_line = 19,
        .type = STRING_TYPE
    },
    {
        .str = "h",
        .length = 1,
        .declaration_line = 22,
        .type = STRING_TYPE
    },
    {
        .str = "i",
        .length = 1,
        .declaration_line = 25,
        .type = STRING_TYPE
    },
    {
        .str = "j",
        .length = 1,
        .declaration_line = 28,
        .type = STRING_TYPE
    },
    {
        .str = "k",
        .length = 1,
        .declaration_line = 29,
        .type = STRING_TYPE
    },
    {
        .str = "l",
        .length = 1,
        .declaration_line = 34,
        .type = STRING_TYPE
    },
    {
        .str = "momo",
        .length = 4,
        .declaration_line = 145,
        .type = STRING_TYPE
    },
    {
        .str = "mamon",
        .length = 5,
        .declaration_line = 223,
        .type = STRING_TYPE
    },
    {
        .str = "paquito66",
        .length = 9,
        .declaration_line = 123,
        .type = STRING_TYPE
    },
    {
        .str = "z",
        .length = 1,
        .declaration_line = 89,
        .type = STRING_TYPE
    },
    {
        .str = "y",
        .length = 1,
        .declaration_line = 43,
        .type = STRING_TYPE
    },
    {
        .str = "x",
        .length = 1,
        .declaration_line = 45,
        .type = STRING_TYPE
    },
};

static void should_insert_symbols() {
    TABLE({
        SymbolName key = create_symbol_name("hello", 5);
        Symbol entry = create_symbol(key, 1, STRING_TYPE);
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
                symbol->declaration_line,
                symbol->type);
            symbol_insert(&table, entry);
        }
        for (int i = 0; i < SYM_LENGTH; i++) {
            symbol_t* symbol = &sym[i];
            SymbolName key = create_symbol_name(symbol->str, symbol->length);
            Symbol expected = create_symbol(
                key,
                symbol->declaration_line,
                symbol->type);
            Symbol* actual = symbol_lookup(&table, &key);
            assert_entry(&expected, actual);
        }
    });
}

static void should_return_null_if_symbol_does_not_exist() {
    TABLE({
        SymbolName alberto = create_symbol_name("alberto", 7);
        Symbol* entry = symbol_lookup(&table, &alberto);
        assert_null(entry);

        SymbolName manolo = create_symbol_name("manolo", 6);
        Symbol manolo_entry = create_symbol(manolo, 1, UNKNOWN_TYPE);
        symbol_insert(&table, manolo_entry);
        entry = symbol_lookup(&table, &alberto);
        assert_null(entry);
    });
}

static void scoped_symbol_should_insert_and_lookup() {
    SymbolName a = create_symbol_name("a", 1);
    SymbolName b = create_symbol_name("b", 1);
    SymbolName c = create_symbol_name("c", 1);
    SymbolName d = create_symbol_name("d", 1);
    SymbolName e = create_symbol_name("e", 1);
    Symbol sym_a = create_symbol(a, 1, NUMBER_TYPE);
    Symbol sym_b = create_symbol(b, 2, NUMBER_TYPE);
    Symbol sym_c = create_symbol(c, 3, NUMBER_TYPE);
    Symbol sym_d = create_symbol(d, 4, NUMBER_TYPE);
    Symbol sym_e = create_symbol(e, 5, NUMBER_TYPE);

    SCOPED_TABLE({
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

static void scoped_symbol_should_insert_globals() {
    SymbolName a = create_symbol_name("a", 1);
    SymbolName b = create_symbol_name("b", 1);
    Symbol sym_a = create_symbol(a, 1, NUMBER_TYPE);
    Symbol sym_b = create_symbol(b, 2, NUMBER_TYPE);
    SCOPED_TABLE({
        scoped_symbol_insert(&table, sym_a);
        scoped_symbol_insert(&table, sym_b);
        Symbol* result_a = scoped_symbol_lookup(&table, &a);
        Symbol* result_b = scoped_symbol_lookup(&table, &b);
        assert_entry(&sym_a, result_a);
        assert_entry(&sym_b, result_b);
    });
}

static void scoped_symbol_should_insert_locals() {
    SymbolName a = create_symbol_name("a", 1);
    SymbolName b = create_symbol_name("b", 1);
    Symbol sym_a = create_symbol(a, 1, NUMBER_TYPE);
    Symbol sym_b = create_symbol(b, 2, NUMBER_TYPE);

    SCOPED_TABLE({
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

static void could_open_previous_scope_and_modify_it() {
    SymbolName a = create_symbol_name("a", 1);
    SymbolName b = create_symbol_name("b", 1);
    SymbolName c = create_symbol_name("c", 1);
    Symbol sym_a = create_symbol(a, 1, NUMBER_TYPE);
    Symbol sym_b = create_symbol(b, 2, NUMBER_TYPE);
    Symbol sym_c = create_symbol(c, 3, NUMBER_TYPE);

    SCOPED_TABLE({
        scoped_symbol_insert(&table, sym_a);
        symbol_create_scope(&table);
            scoped_symbol_insert(&table, sym_b);
        symbol_end_scope(&table);
        symbol_open_prev_scope(&table);
            scoped_symbol_insert(&table, sym_c);
        symbol_end_scope(&table);

        symbol_reset_scopes(&table);

        assert_non_null(scoped_symbol_lookup(&table, &a));
        assert_null(scoped_symbol_lookup(&table, &b));
        assert_null(scoped_symbol_lookup(&table, &c));
        symbol_start_scope(&table);
            assert_non_null(scoped_symbol_lookup(&table, &a));
            assert_non_null(scoped_symbol_lookup(&table, &b));
            assert_non_null(scoped_symbol_lookup(&table, &c));
        symbol_end_scope(&table);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(could_open_previous_scope_and_modify_it),
        cmocka_unit_test(scoped_symbol_should_insert_locals),
        cmocka_unit_test(scoped_symbol_should_insert_globals),
        cmocka_unit_test(scoped_symbol_should_insert_and_lookup),
        cmocka_unit_test(should_return_null_if_symbol_does_not_exist),
        cmocka_unit_test(should_insert_symbols),
        cmocka_unit_test(should_insert_sixteen_elements)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
