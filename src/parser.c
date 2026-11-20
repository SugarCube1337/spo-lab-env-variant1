#include "parser.h"
#include "parser_internal.h"
#include "language.h"  /* этот файл создаёт PackCC при сборке */

#include <string.h>

int parse_next_byte(ParseSession *session)
{
    if (session->cursor >= session->input_length) return -1;
    return session->input[session->cursor++];
}

void parse_record_error(ParseSession *session)
{
    session->syntax_failed = 1;
}

void parse_trace(ParseSession *session, int event, size_t position, size_t length)
{
    /* Событие 1 в PackCC — совпадение. Запоминаем самую дальнюю позицию разбора. */
    size_t candidate = position;
    if (candidate > session->input_length) candidate = session->input_length;
    if (event == 1 && length <= session->input_length - candidate) {
        candidate += length;
    }
    if (candidate > session->furthest) session->furthest = candidate;
}

void parse_expect(ParseSession *session, size_t offset, const char *message)
{
    if (offset > session->input_length) offset = session->input_length;
    if (session->expected == NULL || offset >= session->expected_at) {
        session->expected_at = offset;
        session->expected = message;
    }
}

static void add_failure(ParseResult *result, const char *source, size_t length,
                        DiagnosticKind kind, size_t offset, const char *message)
{
    if (!diagnostics_add(&result->diagnostics, source, length, kind, offset, message)) {
        result->status = PARSE_OUT_OF_MEMORY;
    }
}

/* Пробельные символы ровно те же, что в правиле _ грамматики. */
static int only_whitespace(const char *source, size_t length)
{
    size_t i;
    for (i = 0; i < length; ++i) {
        const char c = source[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return 0;
    }
    return 1;
}

ParseResult parse_source_n(const char *source, size_t length)
{
    ParseResult result = {0};
    ParseSession session = {0};
    AstNode *parsed = NULL;
    spo_context_t *parser;
    const char *nul;

    result.status = PARSE_INVALID_SOURCE;
    if (!source && length != 0) {
        add_failure(&result, "", 0, DIAG_RESOURCE, 0, "null input buffer");
        return result;
    }
    if (!source) source = "";
    nul = memchr(source, '\0', length);
    if (nul) {
        add_failure(&result, source, length, DIAG_LEXICAL,
                    (size_t)(nul - source), "NUL byte is not supported in source");
        return result;
    }

    /* sourceItem* допускает пустую программу и только пробелы.
       Создаём её корень сразу: так результат не зависит от поведения
       PackCC на вводе без единого элемента. */
    if (only_whitespace(source, length)) {
        result.root = ast_make(&session, AST_PROGRAM, NULL, NULL, NULL);
        result.owned_nodes = session.allocations;
        if (result.root) {
            result.status = PARSE_OK;
        } else {
            result.status = PARSE_OUT_OF_MEMORY;
            add_failure(&result, source, length, DIAG_RESOURCE, 0,
                        "cannot allocate empty program AST");
        }
        return result;
    }

    session.input = (const unsigned char *)source;
    session.input_length = length;
    parser = spo_create(&session);
    if (!parser) {
        result.status = PARSE_OUT_OF_MEMORY;
        add_failure(&result, source, length, DIAG_RESOURCE, 0, "cannot initialize parser");
        return result;
    }
    (void)spo_parse(parser, &parsed);
    spo_destroy(parser);

    /* Все узлы принадлежат результату: освободим их одним вызовом. */
    result.owned_nodes = session.allocations;
    if (session.memory_failed) {
        result.status = PARSE_OUT_OF_MEMORY;
        add_failure(&result, source, length, DIAG_RESOURCE, session.furthest,
                    "cannot allocate AST node");
    } else if (session.success && !session.syntax_failed && session.root) {
        result.status = PARSE_OK;
        result.root = session.root;
    } else {
        const size_t offset = session.expected ? session.expected_at : session.furthest;
        const char *message = session.expected ? session.expected :
                              "input does not match the grammar";
        add_failure(&result, source, length, DIAG_SYNTAX, offset, message);
    }
    return result;
}

ParseResult parse_source(const char *source)
{
    return parse_source_n(source, source ? strlen(source) : 0);
}

void parse_result_free(ParseResult *result)
{
    if (!result) return;
    ast_free_allocations(result->owned_nodes);
    diagnostics_free(&result->diagnostics);
    *result = (ParseResult){0};
}
