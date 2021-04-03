#ifndef QUARTZ_COMMON_H
#define QUARTZ_COMMON_H

#ifndef DEBUG
#define NDEBUG // disables assertions
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define UINT8_COUNT (UINT8_MAX + 1)

#ifdef DEBUG
#define LEXER_DEBUG
#define PARSER_DEBUG
#define COMPILER_DEBUG
#define VM_DEBUG
#endif

#endif