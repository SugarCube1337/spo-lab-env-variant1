#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Обходим дерево, чтобы проверить именно структуру, а не подпись картинки. */
static const AstNode *find_node(const AstNode *node, AstKind kind, const char *text)
{
    const AstNode *found;
    if (!node) return NULL;
    if (node->kind == kind && (!text || (node->text && strcmp(node->text, text) == 0))) {
        return node;
    }
    found = find_node(node->left, kind, text);
    return found ? found : find_node(node->right, kind, text);
}

static void test_precedence(void)
{
    ParseResult result = parse_source("main() { int x = 2 + 3 * 4; }");
    const AstNode *plus;
    assert(result.status == PARSE_OK);
    assert(result.root);
    plus = find_node(result.root, AST_BINARY, "+");
    assert(plus && plus->left && plus->right);
    assert(plus->left->kind == AST_INTEGER && strcmp(plus->left->text, "2") == 0);
    assert(plus->right->kind == AST_BINARY && strcmp(plus->right->text, "*") == 0);
    parse_result_free(&result);
}

static void test_controls_and_arrays(void)
{
    const char *source =
        "int f(int[] a) { "
        "int x = 0b101; "
        "if (x > 0) { while (x > 0) { x = x - 1; break; } } "
        "else { do { x = 1; } while (x < 2); } "
        "a[0] = x; "
        "}";
    ParseResult result = parse_source(source);
    const AstNode *if_node;
    assert(result.status == PARSE_OK);
    assert(find_node(result.root, AST_ARRAY_TYPE, "[]"));
    assert(find_node(result.root, AST_BITS, "0b101"));
    assert(find_node(result.root, AST_WHILE, NULL));
    assert(find_node(result.root, AST_DO_WHILE, NULL));
    assert(find_node(result.root, AST_BREAK, NULL));
    assert(find_node(result.root, AST_INDEX, NULL));
    if_node = find_node(result.root, AST_IF, NULL);
    assert(if_node && if_node->right && if_node->right->kind == AST_IF_BRANCHES);
    assert(if_node->right->left && if_node->right->right);
    parse_result_free(&result);
}

static void test_literals_and_operators(void)
{
    ParseResult result = parse_source(
        "main() { "
        "string s = \"a\\\\n\"; char c = 'x'; bool b = true; "
        "int z = (0xA | 0b10) << 1; "
        "b = !false || z >= 2 && z != 3; "
        "}");
    assert(result.status == PARSE_OK);
    assert(find_node(result.root, AST_STRING, NULL));
    assert(find_node(result.root, AST_CHAR, "'x'"));
    assert(find_node(result.root, AST_BOOL, "true"));
    assert(find_node(result.root, AST_HEX, "0xA"));
    assert(find_node(result.root, AST_BITS, "0b10"));
    assert(find_node(result.root, AST_UNARY, "!"));
    assert(find_node(result.root, AST_BINARY, "||"));
    assert(find_node(result.root, AST_BINARY, "&&"));
    assert(find_node(result.root, AST_BINARY, "<<"));
    parse_result_free(&result);
}

static void test_errors(void)
{
    ParseResult result = parse_source("main() { int x = ; }");
    assert(result.status == PARSE_INVALID_SOURCE);
    assert(result.diagnostics.count >= 1);
    assert(result.diagnostics.items[0].line == 1);
    assert(result.diagnostics.items[0].column > 0);
    parse_result_free(&result);

    result = parse_source("main() { foo(1,); }");
    assert(result.status == PARSE_INVALID_SOURCE);
    parse_result_free(&result);
}

int main(void)
{
    test_precedence();
    test_controls_and_arrays();
    test_literals_and_operators();
    test_errors();
    puts("PASS: parser AST, priority, control flow and errors");
    return 0;
}
