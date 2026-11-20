#include "ast.h"
#include "dot.h"
#include "parser_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    ParseSession ctx = {0};
    AstNode *n = ast_make(&ctx, AST_INTEGER, "10", NULL, NULL);
    AstNode *items = ast_make(&ctx, AST_LIST, NULL, n, NULL);
    AstNode *root = ast_make(&ctx, AST_PROGRAM, NULL, items, NULL);
    char data[8192];
    FILE *out = tmpfile();
    size_t bytes;

    assert(root && items && n && out);
    assert(ast_write_dot(out, root));
    rewind(out);
    bytes = fread(data, 1, sizeof(data) - 1, out);
    data[bytes] = '\0';
    assert(strstr(data, "digraph AST") != NULL);
    assert(strstr(data, "Integer\\n10") != NULL);
    assert(strstr(data, " -> ") != NULL);
    assert(strstr(data, "label=\"List\"") == NULL); /* служебные списки скрыты */
    assert(strstr(data, "shape=box") != NULL);
    assert(strstr(data, "fillcolor") == NULL);
    assert(strstr(data, "fontcolor") == NULL);
    assert(strstr(data, "color=") == NULL);
    assert(strstr(data, "rankdir=LR") != NULL);
    assert(fclose(out) == 0);
    /* Проверяем новые узлы: на рисунке ветви if подписаны по смыслу. */
    {
        ParseSession more = {0};
        AstNode *condition = ast_make(&more, AST_BOOL, "true", NULL, NULL);
        AstNode *yes = ast_make(&more, AST_BREAK, NULL, NULL, NULL);
        AstNode *no = ast_make(&more, AST_EXPRESSION_STMT, NULL,
                               ast_make(&more, AST_HEX, "0xA", NULL, NULL), NULL);
        AstNode *branches = ast_make(&more, AST_IF_BRANCHES, NULL, yes, no);
        AstNode *if_stmt = ast_make(&more, AST_IF, NULL, condition, branches);
        FILE *second = tmpfile();
        assert(condition && yes && no && branches && if_stmt && second);
        assert(ast_write_dot(second, if_stmt));
        rewind(second);
        bytes = fread(data, 1, sizeof(data) - 1, second);
        data[bytes] = '\0';
        assert(strstr(data, "If") != NULL);
        assert(strstr(data, "если да") != NULL);
        assert(strstr(data, "иначе") != NULL);
        assert(strstr(data, "IfBranches") == NULL);
        assert(fclose(second) == 0);
        ast_free_allocations(more.allocations);
    }
    ast_free_allocations(ctx.allocations);
    puts("PASS: Graphviz DOT serialization");
    return 0;
}
