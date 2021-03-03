#include <stdarg.h>
#include "./common.h"

#include "../compiler.h"
#include "../chunk.h"

#define CHUNK(...) do {\
    __VA_ARGS__\
    chunk_write(&my, OP_RETURN, -1);\
} while (false)

static inline void assert_value_equal(Value expected, Value other) {
    switch (expected.type) {
    case VALUE_INTEGER:
        assert_int_equal(AS_INTEGER(expected), AS_INTEGER(other));
        break;
    case VALUE_FLOAT:
        assert_float_equal(AS_FLOAT(expected), AS_FLOAT(other), 4);
        break;
    case VALUE_BOOL:
        assert_int_equal(AS_BOOL(expected), AS_BOOL(other));
        break;
    }
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
    compile(source, &compiled);
    assert_chunk(expected, &compiled);
    free_chunk(&compiled);
}

static void emit_constant(Chunk* chunk, Value value, int line) {
    uint8_t index = valuearray_write(&chunk->constants, value);
    chunk_write(chunk, OP_CONSTANT, 1);
    chunk_write(chunk, index, line);
}

static void should_emit_binary() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        emit_constant(&my, INTEGER_VALUE(2), 1);
        emit_constant(&my, INTEGER_VALUE(2), 1);
        chunk_write(&my, OP_ADD, 1);
    });
    assert_compiled_chunk("2+2", &my);
    free_chunk(&my);
}

static void should_emit_complex_calc() {
    Chunk my;
    init_chunk(&my);
    CHUNK({
        emit_constant(&my, INTEGER_VALUE(5), 1);
        emit_constant(&my, INTEGER_VALUE(4), 1);
        chunk_write(&my, OP_ADD, 1);
        emit_constant(&my, INTEGER_VALUE(2), 1);
        chunk_write(&my, OP_MUL, 1);
    });
    assert_compiled_chunk("(5+4)*2", &my);
    free_chunk(&my);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(should_emit_binary),
        cmocka_unit_test(should_emit_complex_calc)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
