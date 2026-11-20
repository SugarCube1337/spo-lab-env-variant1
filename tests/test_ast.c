#include "ast.h"
#include "parser_internal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    ParseSession ctx = {0};
    AstNode *two = ast_make(&ctx, AST_INTEGER, "2", NULL, NULL);
    AstNode *three = ast_make(&ctx, AST_INTEGER, "3", NULL, NULL);
    AstNode *sum = ast_make(&ctx, AST_BINARY, "+", two, three);
    assert(two && three && sum);
    assert(sum->left == two && sum->right == three);
    assert(strcmp(ast_kind_name(sum->kind), "Binary") == 0);
    assert(ctx.memory_failed == 0);
    ast_free_allocations(ctx.allocations);
    puts("PASS: AST ownership/children");
    return 0;
}
