#ifndef SPO_DIAGNOSTICS_H
#define SPO_DIAGNOSTICS_H

#include <stddef.h>
#include <stdio.h>

typedef enum {
    DIAG_LEXICAL,
    DIAG_SYNTAX,
    DIAG_RESOURCE
} DiagnosticKind;

typedef struct {
    DiagnosticKind kind;
    size_t offset;       /* позиция в байтах, отсчёт с нуля */
    size_t line;         /* номер строки с единицы */
    size_t column;       /* номер столбца с единицы, считаем байты */
    char *message;       /* текст ошибки; память освобождаем сами */
} Diagnostic;

typedef struct {
    Diagnostic *items;
    size_t count;
    size_t capacity;
} DiagnosticList;

/* 1 — запись добавлена, 0 — не хватило памяти. */
int diagnostics_add(DiagnosticList *list, const char *source, size_t length,
                    DiagnosticKind kind, size_t offset, const char *message);
void diagnostics_free(DiagnosticList *list);
void diagnostics_print(FILE *out, const DiagnosticList *list, const char *filename);

#endif
