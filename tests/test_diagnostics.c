#include "diagnostics.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *input = "a\r\nb\nc";
    DiagnosticList list = {0};
    assert(diagnostics_add(&list, input, strlen(input), DIAG_SYNTAX, 0, "first"));
    assert(diagnostics_add(&list, input, strlen(input), DIAG_SYNTAX, 3, "second"));
    assert(diagnostics_add(&list, input, strlen(input), DIAG_LEXICAL, 5, "third"));
    assert(list.count == 3);
    assert(list.items[0].line == 1 && list.items[0].column == 1);
    assert(list.items[1].line == 2 && list.items[1].column == 1);
    assert(list.items[2].line == 3 && list.items[2].column == 1);
    assert(strcmp(list.items[1].message, "second") == 0);
    diagnostics_free(&list);
    assert(list.count == 0 && list.items == NULL);
    puts("PASS: diagnostics unit tests");
    return 0;
}
