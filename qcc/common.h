#ifndef QUARTZ_COMMON_H
#define QUARTZ_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DEBUG

#ifdef DEBUG
#define NDEBUG
#define LEXER_DEBUG
#define PARSER_DEBUG
#endif

#endif