#ifndef SPO_DOT_H
#define SPO_DOT_H

#include <stdio.h>
#include "ast.h"

/* 1 — файл записан, 0 — ошибка записи. */
int ast_write_dot(FILE *out, const AstNode *root);

#endif
