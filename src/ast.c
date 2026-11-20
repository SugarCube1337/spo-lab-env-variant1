#include "ast.h"
#include "parser_internal.h"

#include <stdlib.h>
#include <string.h>

AstNode *ast_make(ParseSession *session, AstKind kind, const char *text,
                  AstNode *left, AstNode *right)
{
    AstNode *node = calloc(1, sizeof(*node));
    if (!node) {
        session->memory_failed = 1;
        return NULL;
    }
    node->kind = kind;
    node->left = left;
    node->right = right;
    if (text) {
        size_t length = strlen(text);
        node->text = malloc(length + 1);
        if (!node->text) {
            free(node);
            session->memory_failed = 1;
            return NULL;
        }
        memcpy(node->text, text, length + 1);
    }
    /* Запоминаем каждый узел, чтобы потом освободить даже при ошибке разбора. */
    node->allocated_next = session->allocations;
    session->allocations = node;
    return node;
}

const char *ast_kind_name(AstKind kind)
{
    switch (kind) {
        case AST_PROGRAM:         return "Program";
        case AST_LIST:            return "List";
        case AST_FUNCTION:        return "Function";
        case AST_FUNCTION_PART:   return "SignatureAndBody";
        case AST_TYPE:            return "Type";
        case AST_ARRAY_TYPE:      return "ArrayType";
        case AST_PARAMETER:       return "Parameter";
        case AST_BLOCK:           return "Block";
        case AST_DECLARATION:     return "Declaration";
        case AST_VARIABLE:        return "Variable";
        case AST_EXPRESSION_STMT: return "ExpressionStatement";
        case AST_IF:              return "If";
        case AST_IF_BRANCHES:     return "IfBranches";
        case AST_WHILE:           return "While";
        case AST_DO_WHILE:        return "DoWhile";
        case AST_BREAK:           return "Break";
        case AST_IDENTIFIER:      return "Identifier";
        case AST_INTEGER:         return "Integer";
        case AST_HEX:             return "Hex";
        case AST_BITS:            return "Bits";
        case AST_BOOL:            return "Bool";
        case AST_STRING:          return "String";
        case AST_CHAR:            return "Char";
        case AST_BINARY:          return "Binary";
        case AST_UNARY:           return "Unary";
        case AST_CALL:            return "Call";
        case AST_INDEX:           return "Index";
    }
    return "Unknown";
}

void ast_print(FILE *out, const AstNode *node, unsigned depth)
{
    if (!node) return;
    fprintf(out, "%*s%s", (int)(depth * 2), "", ast_kind_name(node->kind));
    if (node->text) fprintf(out, " [%s]", node->text);
    fputs("\n", out);
    ast_print(out, node->left, depth + 1);
    ast_print(out, node->right, depth + 1);
}

void ast_free_allocations(AstNode *allocations)
{
    while (allocations) {
        AstNode *next = allocations->allocated_next;
        free(allocations->text);
        free(allocations);
        allocations = next;
    }
}
