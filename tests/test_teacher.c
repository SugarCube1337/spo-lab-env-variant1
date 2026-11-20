#include "parser.h"

#include <stdio.h>
#include <string.h>

/* Проверяем реальные связи AST для трёх выражений из примера преподавателя. */
static int binary(const AstNode *node, const char *op)
{
    return node && node->kind == AST_BINARY && node->text &&
           strcmp(node->text, op) == 0;
}

static int identifier(const AstNode *node, const char *name)
{
    return node && node->kind == AST_IDENTIFIER && node->text &&
           strcmp(node->text, name) == 0;
}

static int number(const AstNode *node, const char *value)
{
    return node && node->kind == AST_INTEGER && node->text &&
           strcmp(node->text, value) == 0;
}

static const AstNode *statement_at(const AstNode *program, size_t index)
{
    const AstNode *functions;
    const AstNode *function;
    const AstNode *block;
    const AstNode *statements;
    size_t i;
    if (!program || program->kind != AST_PROGRAM) return NULL;
    functions = program->left;
    if (!functions || functions->kind != AST_LIST) return NULL;
    function = functions->left;
    if (!function || function->kind != AST_FUNCTION ||
        !function->text || strcmp(function->text, "main") != 0 ||
        !function->left || function->left->kind != AST_TYPE ||
        !function->left->text || strcmp(function->left->text, "int") != 0 ||
        !function->right || function->right->kind != AST_FUNCTION_PART)
        return NULL;
    block = function->right->right;
    if (!block || block->kind != AST_BLOCK) return NULL;
    statements = block->left;
    for (i = 0; i < index; ++i) {
        if (!statements || statements->kind != AST_LIST) return NULL;
        statements = statements->right;
    }
    if (!statements || statements->kind != AST_LIST ||
        !statements->left || statements->left->kind != AST_EXPRESSION_STMT)
        return NULL;
    return statements->left->left;
}

static int first_expression(const AstNode *root)
{
    const AstNode *assign = statement_at(root, 0);
    const AstNode *sum;
    const AstNode *first_sum;
    const AstNode *product;
    if (!binary(assign, "=") || !identifier(assign->left, "x")) return 0;
    sum = assign->right;
    if (!binary(sum, "+") || !identifier(sum->right, "d")) return 0;
    first_sum = sum->left;
    if (!binary(first_sum, "+") || !identifier(first_sum->left, "a")) return 0;
    product = first_sum->right;
    return binary(product, "*") && identifier(product->left, "b") &&
           identifier(product->right, "c");
}

static int second_expression(const AstNode *root)
{
    const AstNode *assign = statement_at(root, 1);
    const AstNode *sum;
    const AstNode *left;
    const AstNode *right;
    if (!binary(assign, "=") || !identifier(assign->left, "x")) return 0;
    sum = assign->right;
    if (!binary(sum, "+")) return 0;
    left = sum->left;
    right = sum->right;
    return binary(left, "*") && binary(right, "*") &&
           identifier(left->left, "a") && identifier(left->right, "b") &&
           identifier(right->left, "c") && identifier(right->right, "d");
}

static int third_expression(const AstNode *root)
{
    const AstNode *assign = statement_at(root, 2);
    const AstNode *sum;
    const AstNode *nested;
    const AstNode *call;
    const AstNode *arguments;
    size_t i;
    static const char *values[] = { "1", "2", "3" };
    if (!binary(assign, "=")) return 0;
    sum = assign->left;
    nested = assign->right;
    if (!binary(sum, "+") || !identifier(sum->left, "z") ||
        !number(sum->right, "4") || !binary(nested, "=") ||
        !number(nested->right, "3")) return 0;
    call = nested->left;
    if (!call || call->kind != AST_CALL || !identifier(call->left, "f")) return 0;
    arguments = call->right;
    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        if (!arguments || arguments->kind != AST_LIST ||
            !number(arguments->left, values[i])) return 0;
        arguments = arguments->right;
    }
    return arguments == NULL;
}

int main(void)
{
    static const char source[] =
        "int main() {\n"
        "  x = a + b * c + d;\n"
        "  x = a * b + c * d;\n"
        "  z + 4 = f(1, 2, 3) = 3;\n"
        "}\n";
    ParseResult result = parse_source(source);
    int ok = result.status == PARSE_OK && result.root != NULL &&
             result.diagnostics.count == 0;

    if (!ok) {
        fprintf(stderr, "FAIL: пример преподавателя не разобран (status=%d)\n",
                (int)result.status);
        diagnostics_print(stderr, &result.diagnostics, "teacher");
    } else {
        if (!first_expression(result.root)) {
            fputs("FAIL: первое дерево: a + b*c + d\n", stderr);
            ok = 0;
        }
        if (!second_expression(result.root)) {
            fputs("FAIL: второе дерево: a*b + c*d\n", stderr);
            ok = 0;
        }
        if (!third_expression(result.root)) {
            fputs("FAIL: третье дерево: z+4 = f(1,2,3) = 3\n", stderr);
            ok = 0;
        }
    }
    parse_result_free(&result);
    if (!ok) return 1;
    puts("PASS: teacher arithmetic AST (3 expressions and assignment associativity)");
    return 0;
}
