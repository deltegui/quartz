#include <stdarg.h>
#include <string.h>
#include "./common.h"
#include "../symbol.h"
#include "../typechecker.h"

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
        Symbol entry = (Symbol){
            .name = create_symbol_name("hello", 5),
            .declaration_line = 1,
            .type = STRING_TYPE
        };
        symbol_insert(&table, entry);
        SymbolName key = create_symbol_name("hello", 5);
        Symbol* stored = symbol_lookup(&table, &key);
        assert_entry(&entry, stored);
    });
}

static void should_insert_sixteen_elements() {
    TABLE({
        for (int i = 0; i < SYM_LENGTH; i++) {
            symbol_t* symbol = &sym[i];
            Symbol entry = (Symbol){
                .name = create_symbol_name(symbol->str, symbol->length),
                .declaration_line = symbol->declaration_line,
                .type = symbol->type,
            };
            symbol_insert(&table, entry);
        }
        for (int i = 0; i < SYM_LENGTH; i++) {
            symbol_t* symbol = &sym[i];
            SymbolName key = create_symbol_name(symbol->str, symbol->length);
            Symbol expected = (Symbol){
                .name = key,
                .declaration_line = symbol->declaration_line,
                .type = symbol->type,
            };
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
        Symbol manolo_entry = (Symbol){
            .name = manolo,
            .declaration_line = 1,
            .type = UNKNOWN_TYPE,
        };
        symbol_insert(&table, manolo_entry);
        entry = symbol_lookup(&table, &alberto);
        assert_null(entry);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_return_null_if_symbol_does_not_exist),
        cmocka_unit_test(should_insert_symbols),
        cmocka_unit_test(should_insert_sixteen_elements)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
