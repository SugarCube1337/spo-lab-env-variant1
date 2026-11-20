#ifndef SPO_AST_H
#define SPO_AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM, AST_LIST, AST_FUNCTION, AST_FUNCTION_PART,
    AST_TYPE, AST_ARRAY_TYPE, AST_PARAMETER, AST_BLOCK, AST_DECLARATION,
    AST_VARIABLE, AST_EXPRESSION_STMT, AST_IF, AST_IF_BRANCHES,
    AST_WHILE, AST_DO_WHILE, AST_BREAK,
    AST_IDENTIFIER, AST_INTEGER, AST_HEX, AST_BITS, AST_BOOL,
    AST_STRING, AST_CHAR, AST_BINARY, AST_UNARY, AST_CALL, AST_INDEX
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *text;                       /* имя, оператор или значение литерала */
    struct AstNode *left;            /* первый дочерний узел */
    struct AstNode *right;           /* второй дочерний узел */
    struct AstNode *allocated_next;  /* цепочка для освобождения памяти */
} AstNode;

struct ParseSession;
AstNode *ast_make(struct ParseSession *session, AstKind kind, const char *text,
                  AstNode *left, AstNode *right);
const char *ast_kind_name(AstKind kind);
void ast_print(FILE *out, const AstNode *node, unsigned depth);
void ast_free_allocations(AstNode *allocations);

#endif
