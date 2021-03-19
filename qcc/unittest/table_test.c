#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "./common.h"

#include "../vm.h"
#include "../table.h"
#include "../object.h"

static char* read_words_file() {
    FILE* source = fopen("words.txt", "r");
    if (source == NULL) {
        fprintf(stderr, "Error while reading source file: \n");
        return NULL;
    }
    fseek(source, 0, SEEK_END);
    size_t size = ftell(source);
    fseek(source, 0, SEEK_SET);
    // Size of the file plus \0 character
    char* buffer = (char*) malloc(size + 1);
    if (buffer == NULL) {
        fclose(source);
        fprintf(stderr, "Error while allocating file buffer!\n");
        return NULL;
    }
    fread(buffer, 1, size, source);
    buffer[size] = '\0';
    fclose(source);
    return buffer;
}

static void table_load_from_buffer(Table* table, char* buffer, int amount) {
    char* current = buffer;
    char* start = buffer;
    int current_word = 0;
    while (*current != '\0') {
        if (*current == '\n') {
            ObjString* str = copy_string(start, current - start);
            table_set(table, str, NUMBER_VALUE(current_word));
            current_word++;
            start = current;
        }
        if (current_word >= amount) {
            break;
        }
        current++;
    }
}

static void assert_buffer_is_loaded(Table* table, char* buffer, int amount) {
    char* current = buffer;
    char* start = buffer;
    int current_word = 0;
    while (*current != '\0') {
        if (*current == '\n') {
            ObjString* str = copy_string(start, current - start);
            Value val = table_find(table, str);
            assert_false(IS_NIL(val));
            assert_true(AS_NUMBER(val) == current_word);
            current_word++;
            start = current;
        }
        if (current_word >= amount) {
            break;
        }
        current++;
    }
}

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
    Table table;\
    start_test_case(&table);\
    __VA_ARGS__\
    finish_test_case(&table);\
} while (false)

#define TABLE_BENCHMARK(amount_of_words, ...) do {\
    Table table;\
    start_test_case(&table);\
    char* words = read_words_file();\
    if (words == NULL) {\
        fail();\
    }\
    struct timespec time_start, time_end;\
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);\
    table_load_from_buffer(&table, words, amount_of_words);\
    printf(\
        "Table size: %d, capacity: %d, longest distance: %d, load factor: %f\n",\
        table.size,\
        table.capacity,\
        table.max_distance,\
        (double)table.size / (double)table.capacity);\
    __VA_ARGS__\
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);\
    int64_t delta_us = (time_end.tv_sec - time_start.tv_sec) * 1000000 + (time_end.tv_nsec - time_start.tv_nsec) / 1000;\
    printf("Elapsed time: %lu us\n", delta_us);\
    finish_test_case(&table);\
    free(words);\
} while (false)

static void should_insert_and_get_one_element() {
    TABLE_TEST({
        Entry e = create_entry("demo", 5);
        table_set(&table, e.key, e.value);
        Value val = table_find(&table, e.key);
        assert_true(IS_NUMBER(val));
        assert_float_equal(AS_NUMBER(val), 5, 1);
    });
}

static void benchmark_insert_large_amount_of_elements() {
    TABLE_BENCHMARK(466550, {
        assert_int_equal(table.size, 466550);
        assert_buffer_is_loaded(&table, words, 466550);
    });
}

static void should_return_nil_if_the_key_is_not_found() {
    TABLE_BENCHMARK(367000, {
        const char* word = "las cosas buenas son dificiles de conseguir";
        Value result = table_find(&table, copy_string(word, 4));
        assert_true(IS_NIL(result));
    });
}

static void should_substitute_old_key() {
    TABLE_TEST({
        Entry first = create_entry("demo", 5);
        table_set(&table, first.key, first.value);
        Entry second = create_entry("demo", 10);
        table_set(&table, second.key, second.value);
        Value val = table_find(&table, first.key);
        assert_true(IS_NUMBER(val));
        assert_int_equal(table.size, 1);
        assert_float_equal(AS_NUMBER(val), 10, 1);
    });
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_substitute_old_key),
        cmocka_unit_test(should_return_nil_if_the_key_is_not_found),
        cmocka_unit_test(benchmark_insert_large_amount_of_elements),
        cmocka_unit_test(should_insert_and_get_one_element)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}