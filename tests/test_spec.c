#include "parser.h"

#include <stdio.h>
#include <string.h>

/* Проверяем разные варианты исходной грамматики через публичный API. */
typedef struct {
    const char *name;
    const char *source;
} Case;

static int check_valid(const Case *test)
{
    ParseResult result = parse_source(test->source);
    int ok = result.status == PARSE_OK && result.root != NULL &&
             result.root->kind == AST_PROGRAM && result.diagnostics.count == 0;
    if (!ok) {
        fprintf(stderr, "FAIL (valid): %s, status=%d\n", test->name, (int)result.status);
        diagnostics_print(stderr, &result.diagnostics, test->name);
    }
    parse_result_free(&result);
    return ok;
}

static int check_invalid(const Case *test)
{
    ParseResult result = parse_source(test->source);
    int ok = result.status == PARSE_INVALID_SOURCE && result.root == NULL &&
             result.diagnostics.count > 0;
    if (!ok) fprintf(stderr, "FAIL (invalid): %s, status=%d, errors=%zu\n",
                     test->name, (int)result.status, result.diagnostics.count);
    parse_result_free(&result);
    return ok;
}

/* Ищем первый узел нужного вида для проверки вложенности. */
static const AstNode *find(const AstNode *node, AstKind kind, const char *text)
{
    const AstNode *found;
    if (!node) return NULL;
    if (node->kind == kind && (!text || (node->text && strcmp(node->text, text) == 0)))
        return node;
    found = find(node->left, kind, text);
    return found ? found : find(node->right, kind, text);
}

static int check_structure(void)
{
    ParseResult result;
    const AstNode *outer;
    const AstNode *inner;
    const AstNode *minus;
    const AstNode *assign;
    int ok = 1;

    /* else должен относиться к ближайшему if. */
    result = parse_source("main() { if (a) if (b) x = 1; else x = 2; }");
    outer = find(result.root, AST_IF, NULL);
    inner = outer && outer->right ? outer->right->left : NULL;
    if (result.status != PARSE_OK || !outer || !outer->right ||
        outer->right->kind != AST_IF_BRANCHES || outer->right->right != NULL ||
        !inner || inner->kind != AST_IF || !inner->right ||
        inner->right->kind != AST_IF_BRANCHES || !inner->right->right) {
        fprintf(stderr, "FAIL: nearest-else AST\n");
        ok = 0;
    }
    parse_result_free(&result);

    /* Вычитание левоассоциативно: (10 - 3) - 2. */
    result = parse_source("main() { x = 10 - 3 - 2; }");
    minus = find(result.root, AST_BINARY, "-");
    if (result.status != PARSE_OK || !minus || !minus->left ||
        minus->left->kind != AST_BINARY || !minus->right ||
        minus->right->kind != AST_INTEGER) {
        fprintf(stderr, "FAIL: left-associative subtraction\n");
        ok = 0;
    }
    parse_result_free(&result);

    /* Присваивание правоассоциативно: a = (b = c). */
    result = parse_source("main() { a = b = c; }");
    assign = find(result.root, AST_BINARY, "=");
    if (result.status != PARSE_OK || !assign || !assign->right ||
        assign->right->kind != AST_BINARY ||
        !assign->right->text || strcmp(assign->right->text, "=") != 0) {
        fprintf(stderr, "FAIL: right-associative assignment\n");
        ok = 0;
    }
    parse_result_free(&result);

    return ok;
}

static int check_api_edges(void)
{
    static const char with_nul[] = { 'm', 'a', 'i', 'n', '\0', '(', ')' };
    ParseResult result = parse_source_n(with_nul, sizeof(with_nul));
    int ok = result.status == PARSE_INVALID_SOURCE &&
             result.diagnostics.count == 1 &&
             result.diagnostics.items[0].kind == DIAG_LEXICAL &&
             result.diagnostics.items[0].offset == 4;
    parse_result_free(&result);
    parse_result_free(&result); /* повторный вызов безопасен */
    if (!ok) fprintf(stderr, "FAIL: embedded NUL / free result\n");

    result = parse_source_n(NULL, 3);
    if (result.status == PARSE_OK || result.diagnostics.count == 0) {
        fprintf(stderr, "FAIL: invalid source buffer\n");
        ok = 0;
    }
    parse_result_free(&result);

    result = parse_source("main() {\r\n  int x = ;\r\n}");
    if (result.status == PARSE_OK || result.diagnostics.count == 0 ||
        result.diagnostics.items[0].line != 2) {
        fprintf(stderr, "FAIL: multiline diagnostics\n");
        ok = 0;
    }
    parse_result_free(&result);
    return ok;
}

int main(void)
{
    const Case valid[] = {
        {"empty source", ""},
        {"whitespace only", " \r\n\t "},
        {"prototype", "int sum(int a, int b);"},
        {"optional types", "call(a, b);"},
        {"empty body", "main() {}"},
        {"identifier", "_fn123() { int _x1 = 2; }"},
        {"builtins", "main(){bool b;byte x;int i;uint u;long l;ulong ul;char c;string s;}"},
        {"custom types", "int f(MyType value){ MyType other; }"},
        {"array types", "main(){ int[] a; int[,] b; int[][] c; }"},
        {"declaration list", "main(){ int a, b = 1, c = a + b; }"},
        {"mixed parameters", "f(a, int b, char c);"},
        {"empty call", "main(){ ping(); }"},
        {"nested calls", "main(){ foo(bar(1), baz(2, 3)); }"},
        {"postfix chain", "main(){ foo(1)[0](2); }"},
        {"multiple indices", "main(){ values[0, 1] = 2; }"},
        {"empty index list", "main(){ values[]; }"},
        {"if else", "main(){ if (true) x = 1; else x = 2; }"},
        {"nearest else", "main(){ if (a) if (b) x = 1; else x = 2; }"},
        {"while break", "main(){ while (x) { break; } }"},
        {"do while", "main(){ do {} while (x); }"},
        {"arithmetic", "main(){ x = 1 + 2 * 3 - 4 / 2 % 1; }"},
        {"bitwise logical", "main(){ x = (a && b) || !c & ~d ^ e | f; }"},
        {"comparison shifts", "main(){ x = a >= 2 && b <= 3 || c != d && e == 1; y = x << 1 >> 2; }"},
        {"unary", "main(){ x = ++a + --b - -c + +d + ~e + !f; }"},
        {"assignment chain", "main(){ a = b = c = 1; }"},
        {"hex literals", "main(){ x = 0xDEAD; y = 0Xff; }"},
        {"binary literals", "main(){ x = 0b101; y = 0B001; }"},
        {"string literal", "main(){ string s = \"hello\\\\n\"; }"},
        {"char literal", "main(){ char c = 'Z'; }"},
        {"bool vs identifier", "main(){ bool trueValue = true; bool falseFlag = false; }"},
        {"CRLF", "main() {\r\n int a = 1;\r\n}\r\n"},
        {"all in one", "int sum(int a,int b);main(){int[] a; if(true){a[0]=sum(1,2);}else break;}"}
    };
    const Case invalid[] = {
        {"global variable", "int x = 1;"},
        {"missing initializer", "main(){ int x = ; }"},
        {"missing if condition", "main(){ if () x = 1; }"},
        {"missing while condition", "main(){ while () {} }"},
        {"missing do terminator", "main(){ do {} while (x) }"},
        {"bad hex", "main(){ int x = 0xG; }"},
        {"bad binary", "main(){ int x = 0b102; }"},
        {"unfinished string", "main(){ string s = \"unfinished; }"},
        {"long char", "main(){ char c = 'ab'; }"},
        {"escaped char disallowed by assignment", "main(){ char c = '\\n'; }"},
        {"trailing variable comma", "main(){ int a,; }"},
        {"trailing parameter comma", "foo(int a,);"},
        {"trailing call comma", "main(){ foo(1,); }"},
        {"trailing index comma", "main(){ a[1,]; }"},
        {"unfinished braces", "main(){ int x = (1 + 2; }"},
        {"missing break semicolon", "main(){ if (x) break else break; }"},
        {"missing declaration semicolon", "main(){ int x = 1 }"},
        {"invalid character", "main(){ @; }"},
        {"invalid trailing tokens", "main(){} abc"},
        {"missing rhs", "main(){ x = * 3; }"},
        {"missing logical rhs", "main(){ x = 1 && ; }"},
        {"digit before identifier", "main(){ int x = 123abc; }"},
        {"keyword as variable", "main(){ int if = 1; }"}
    };
    size_t i;
    int ok = 1;
    for (i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i)
        if (!check_valid(&valid[i])) ok = 0;
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        if (!check_invalid(&invalid[i])) ok = 0;
    if (!check_structure()) ok = 0;
    if (!check_api_edges()) ok = 0;
    if (!ok) return 1;
    printf("PASS: %zu valid + %zu invalid grammar cases, AST and API edges\n",
           sizeof(valid) / sizeof(valid[0]), sizeof(invalid) / sizeof(invalid[0]));
    return 0;
}
