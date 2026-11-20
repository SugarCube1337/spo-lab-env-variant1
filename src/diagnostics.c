#include "diagnostics.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void locate(const char *source, size_t offset, size_t *line, size_t *column)
{
    size_t i;
    *line = 1;
    *column = 1;
    for (i = 0; i < offset; ++i) {
        if (source[i] == '\r') {
            ++*line;
            *column = 1;
            if (i + 1 < offset && source[i + 1] == '\n') ++i;
        } else if (source[i] == '\n') {
            ++*line;
            *column = 1;
        } else {
            ++*column;
        }
    }
}

int diagnostics_add(DiagnosticList *list, const char *source, size_t length,
                    DiagnosticKind kind, size_t offset, const char *message)
{
    char *copy;
    Diagnostic *resized;
    size_t new_capacity;
    Diagnostic *entry;

    if (offset > length) offset = length;
    copy = malloc(strlen(message) + 1);
    if (!copy) return 0;
    strcpy(copy, message);

    /* Место закончилось — расширяем массив ошибок. */
    if (list->count == list->capacity) {
        if (list->capacity > SIZE_MAX / 2) { free(copy); return 0; }
        new_capacity = list->capacity ? list->capacity * 2 : 4;
        if (new_capacity > SIZE_MAX / sizeof(*list->items)) {
            free(copy);
            return 0;
        }
        resized = realloc(list->items, new_capacity * sizeof(*list->items));
        if (!resized) { free(copy); return 0; }
        list->items = resized;
        list->capacity = new_capacity;
    }

    entry = &list->items[list->count++];
    entry->kind = kind;
    entry->offset = offset;
    locate(source, offset, &entry->line, &entry->column);
    entry->message = copy;
    return 1;
}

void diagnostics_free(DiagnosticList *list)
{
    size_t i;
    for (i = 0; i < list->count; ++i) free(list->items[i].message);
    free(list->items);
    *list = (DiagnosticList){0};
}

void diagnostics_print(FILE *out, const DiagnosticList *list, const char *filename)
{
    size_t i;
    const char *where = filename ? filename : "<input>";
    for (i = 0; i < list->count; ++i) {
        const Diagnostic *d = &list->items[i];
        const char *kind = d->kind == DIAG_LEXICAL ? "lexical error" :
                           d->kind == DIAG_SYNTAX ? "syntax error" : "resource error";
        fprintf(out, "%s:%zu:%zu: %s: %s\n", where, d->line, d->column,
                kind, d->message);
    }
}
