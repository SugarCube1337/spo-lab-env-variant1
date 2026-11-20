#ifndef SPO_PARSER_INTERNAL_H
#define SPO_PARSER_INTERNAL_H

#include <stddef.h>
#include "ast.h"

typedef struct ParseSession {
    const unsigned char *input;
    size_t input_length;
    size_t cursor;
    size_t furthest;
    size_t expected_at;
    const char *expected;
    AstNode *allocations;
    AstNode *root;
    int success;
    int syntax_failed;
    int memory_failed; /* пока ловим нехватку памяти только при создании AST */
} ParseSession;

int parse_next_byte(ParseSession *session);
void parse_record_error(ParseSession *session);
void parse_trace(ParseSession *session, int event, size_t position, size_t length);
void parse_expect(ParseSession *session, size_t offset, const char *message);

#endif
