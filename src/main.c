#include "parser.h"
#include "dot.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Читаем файл кусками. Если места мало, увеличиваем буфер. */
static char *read_source(FILE *file, size_t *length)
{
    char chunk[BUFSIZ];
    char *data = NULL;
    size_t size = 0;
    size_t capacity = 0;
    size_t count;

    while ((count = fread(chunk, 1, sizeof(chunk), file)) != 0) {
        size_t needed;
        char *next;
        if (size > SIZE_MAX - count - 1) { free(data); return NULL; }
        needed = size + count + 1;
        if (needed > capacity) {
            size_t new_capacity = capacity ? capacity : sizeof(chunk);
            while (new_capacity < needed) {
                if (new_capacity > SIZE_MAX / 2) { new_capacity = needed; break; }
                new_capacity *= 2;
            }
            next = realloc(data, new_capacity);
            if (!next) { free(data); return NULL; }
            data = next;
            capacity = new_capacity;
        }
        memcpy(data + size, chunk, count);
        size += count;
    }
    if (ferror(file)) { free(data); return NULL; }
    if (!data) {
        data = malloc(1);
        if (!data) return NULL;
    }
    data[size] = '\0';
    *length = size;
    return data;
}

int main(int argc, char **argv)
{
    FILE *in = NULL;
    FILE *out = NULL;
    char *source = NULL;
    size_t length = 0;
    ParseResult result = {0};
    int status = EXIT_FAILURE;
    int wrote_output = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s INPUT.spo OUTPUT.dot\n", argv[0]);
        return EXIT_FAILURE;
    }
    in = fopen(argv[1], "rb");
    if (!in) {
        fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }
    source = read_source(in, &length);
    if (fclose(in) != 0 || !source) {
        fprintf(stderr, "%s: cannot read file\n", argv[1]);
        free(source);
        return EXIT_FAILURE;
    }

    /* Вся работа с синтаксисом спрятана в модуле parser. */
    result = parse_source_n(source, length);
    if (result.status != PARSE_OK) {
        if (result.diagnostics.count != 0) {
            diagnostics_print(stderr, &result.diagnostics, argv[1]);
        } else {
            fprintf(stderr, "%s: parse failed without diagnostics\n", argv[1]);
        }
        goto done;
    }

    out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "%s: %s\n", argv[2], strerror(errno));
        goto done;
    }
    wrote_output = 1;
    if (!ast_write_dot(out, result.root) || fclose(out) != 0) {
        out = NULL;
        fprintf(stderr, "%s: cannot write DOT file\n", argv[2]);
        goto done;
    }
    out = NULL;
    status = EXIT_SUCCESS;

done:
    /* Здесь закрываем файлы и освобождаем память при любом исходе. */
    if (out) (void)fclose(out);
    if (status != EXIT_SUCCESS && wrote_output) (void)remove(argv[2]);
    parse_result_free(&result);
    free(source);
    return status;
}
