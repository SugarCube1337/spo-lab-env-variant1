#ifndef SPO_PARSER_H
#define SPO_PARSER_H

#include <stddef.h>
#include "ast.h"
#include "diagnostics.h"

typedef enum {
    PARSE_OK,
    PARSE_INVALID_SOURCE,
    PARSE_OUT_OF_MEMORY
} ParseStatus;

typedef struct {
    ParseStatus status;
    AstNode *root;          /* корень дерева; живёт до parse_result_free() */
    AstNode *owned_nodes;   /* список всех созданных узлов, в том числе при неудачном разборе */
    DiagnosticList diagnostics;
} ParseResult;

/* Разбираем готовый текст. Файлы и сообщения в консоль здесь не трогаем. */
ParseResult parse_source_n(const char *source, size_t length);
ParseResult parse_source(const char *source);
void parse_result_free(ParseResult *result);

#endif
