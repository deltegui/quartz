#include <stdarg.h>
#include "./common.h"
#include "../compiler.h"
#include "../chunk.h"
#include "../values.h"

#define CHUNK(...) do {\
    __VA_ARGS__\
    chunk_write(&my, OP_RETURN, -1);\
} while (false)

static inline void assert_value_equal(Value expected, Value other) {
    assert_true(value_equals(expected, other));
}

static void assert_chunk(Chunk* expected, Chunk* emitted) {
    assert_int_equal(expected->size, emitted->size);
    for (int i = 0; i < expected->size; i++) {
        assert_int_equal(expected->code[i], emitted->code[i]);
    }
    assert_int_equal(expected->constants.size, emitted->constants.size);
    for (int i = 0; i < expected->constants.size; i++) {
        assert_value_equal(expected->constants.values[i], emitted->constants.values[i]);
    }
}

static void assert_compiled_chunk(const char* source, Chunk* expected) {
    Chunk compiled;
    init_chunk(&compiled);
    CompilationResult result = compile(source, &compiled);
    switch (result) {
    case PARSING_ERROR: printf("PARSING_ERROR!"); break;
    case TYPE_ERROR: printf("TYPE_ERROR!"); break;
    case COMPILATION_ERROR: printf("COMPILATION_ERROR!"); break;
    case COMPILATION_OK: assert_chunk(expected, &compiled); break;
    }
    free_chunk(&compiled);
}

static void emit_value(Chunk* chunk, Value value, int line) {
    uint8_t index = valuearray_write(&chunk->constants, value);
    chunk_write(chunk, index, line);
}

static void emit_constant(Chunk* chunk, Value value, int line) {
    chunk_write(chunk, OP_CONSTANT, line);
    emit_value(chunk, value, line);
}

static void should_emit_binary() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        emit_constant(&my, NUMBER_VALUE(2), 1);
        emit_constant(&my, NUMBER_VALUE(2), 1);
        chunk_write(&my, OP_ADD, 1);
        chunk_write(&my, OP_POP, 1);
    });
    assert_compiled_chunk("2+2;", &my);
    free_chunk(&my);
}

static void should_emit_complex_calc() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        emit_constant(&my, NUMBER_VALUE(5), 1);
        emit_constant(&my, NUMBER_VALUE(4), 1);
        chunk_write(&my, OP_ADD, 1);
        emit_constant(&my, NUMBER_VALUE(2), 1);
        chunk_write(&my, OP_MUL, 1);
        chunk_write(&my, OP_POP, 1);
    });
    assert_compiled_chunk("(5+4)*2;", &my);
    free_chunk(&my);
}

static void should_emit_comparisions() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        emit_constant(&my, NUMBER_VALUE(1), 1);
        emit_constant(&my, NUMBER_VALUE(2), 1);
        chunk_write(&my, OP_EQUAL, 1);
        chunk_write(&my, OP_POP, 1);
    });
    assert_compiled_chunk("1 == 2;", &my);
    free_chunk(&my);
}

static void should_compile_globals() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        uint8_t index = valuearray_write(&my.constants, OBJ_VALUE(copy_string("esto", 4)));
        emit_constant(&my, NUMBER_VALUE(5), 1);
        emit_constant(&my, NUMBER_VALUE(2), 1);
        chunk_write(&my, OP_MUL, 1);
        chunk_write(&my, OP_DEFINE_GLOBAL, 1);
        chunk_write(&my, index, 1);
    });
    assert_compiled_chunk("var esto = 5*2;", &my);
    free_chunk(&my);
}

static void should_compile_globals_with_default_values() {
    Chunk my;
#define DEFAULT_VALUE(code, default_val) do {\
    init_chunk(&my);\
    CHUNK({\
        uint8_t index = valuearray_write(&my.constants, OBJ_VALUE(copy_string("esto", 4)));\
        emit_constant(&my, default_val, 1);\
        chunk_write(&my, OP_DEFINE_GLOBAL, 1);\
        chunk_write(&my, index, 1);\
    });\
    assert_compiled_chunk(code, &my);\
    free_chunk(&my);\
} while(false)

    DEFAULT_VALUE("var esto: Number;", NUMBER_VALUE(0));
    DEFAULT_VALUE("var esto: Bool;", BOOL_VALUE(false));
    DEFAULT_VALUE("var esto: String;", OBJ_VALUE(copy_string("", 0)));

#undef DEFAULT_VALUE
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_compile_globals_with_default_values),
        cmocka_unit_test(should_compile_globals),
        cmocka_unit_test(should_emit_binary),
        cmocka_unit_test(should_emit_complex_calc),
        cmocka_unit_test(should_emit_comparisions)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
