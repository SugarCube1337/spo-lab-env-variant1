#include "dot.h"

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

/* Экранируем текст, чтобы кавычки и переносы не ломали DOT. */
static void write_quoted(FILE *out, const char *s)
{
    size_t start = 0;
    size_t i;
    for (i = 0; s[i] != '\0'; ++i) {
        const char *esc = NULL;
        if (s[i] == '\\') esc = "\\\\";
        else if (s[i] == '"') esc = "\\\"";
        else if (s[i] == '\n') esc = "\\n";
        else if (s[i] == '\r') esc = "\\r";
        if (esc) {
            (void)fwrite(s + start, 1, i - start, out);
            fputs(esc, out);
            start = i + 1;
        }
    }
    (void)fwrite(s + start, 1, i - start, out);
}

static void write_node(FILE *out, const AstNode *node);

/* List — это техническая цепочка. На картинке соединяем её элементы напрямую. */
static void write_child(FILE *out, const AstNode *parent,
                        const AstNode *child, const char *role)
{
    if (!child) return;
    if (child->kind == AST_LIST) {
        write_child(out, parent, child->left, role);
        write_child(out, parent, child->right, role);
        return;
    }
    fprintf(out, "  n%" PRIxPTR " -> n%" PRIxPTR " [label=\"%s\"];\n",
            (uintptr_t)parent, (uintptr_t)child, role);
    write_node(out, child);
}

/* Подписи связей описывают смысл потомков, а не просто left/right. */
static void write_node(FILE *out, const AstNode *node)
{
    if (!node) return;
    fprintf(out, "  n%" PRIxPTR " [label=\"%s", (uintptr_t)node,
            ast_kind_name(node->kind));
    if (node->text) {
        fputs("\\n", out);
        write_quoted(out, node->text);
    }
    fputs("\"];\n", out);

    switch (node->kind) {
        case AST_PROGRAM:
            write_child(out, node, node->left, "функция");
            write_child(out, node, node->right, "функция");
            break;
        case AST_FUNCTION:
            write_child(out, node, node->left, "тип");
            if (node->right && node->right->kind == AST_FUNCTION_PART) {
                write_child(out, node, node->right->left, "параметр");
                write_child(out, node, node->right->right, "тело");
            } else {
                write_child(out, node, node->right, "сигнатура");
            }
            break;
        case AST_ARRAY_TYPE:
            write_child(out, node, node->left, "элемент");
            break;
        case AST_PARAMETER:
            write_child(out, node, node->left, "тип");
            write_child(out, node, node->right, "имя");
            break;
        case AST_BLOCK:
            write_child(out, node, node->left, "оператор");
            write_child(out, node, node->right, "оператор");
            break;
        case AST_DECLARATION:
            write_child(out, node, node->left, "тип");
            write_child(out, node, node->right, "переменная");
            break;
        case AST_VARIABLE:
            write_child(out, node, node->left, "имя");
            write_child(out, node, node->right, "значение");
            break;
        case AST_EXPRESSION_STMT:
            write_child(out, node, node->left, "выражение");
            break;
        case AST_IF:
            write_child(out, node, node->left, "условие");
            if (node->right && node->right->kind == AST_IF_BRANCHES) {
                write_child(out, node, node->right->left, "если да");
                write_child(out, node, node->right->right, "иначе");
            }
            break;
        case AST_WHILE:
            write_child(out, node, node->left, "условие");
            write_child(out, node, node->right, "тело");
            break;
        case AST_DO_WHILE:
            write_child(out, node, node->left, "тело");
            write_child(out, node, node->right, "условие");
            break;
        case AST_BINARY:
            write_child(out, node, node->left, "левый");
            write_child(out, node, node->right, "правый");
            break;
        case AST_UNARY:
            write_child(out, node, node->left, "операнд");
            break;
        case AST_CALL:
            write_child(out, node, node->left, "функция");
            write_child(out, node, node->right, "аргумент");
            break;
        case AST_INDEX:
            write_child(out, node, node->left, "массив");
            write_child(out, node, node->right, "индекс");
            break;
        case AST_LIST:
        case AST_FUNCTION_PART:
        case AST_IF_BRANCHES:
            write_child(out, node, node->left, "элемент");
            write_child(out, node, node->right, "элемент");
            break;
        case AST_BREAK:
        case AST_TYPE:
        case AST_IDENTIFIER:
        case AST_INTEGER:
        case AST_HEX:
        case AST_BITS:
        case AST_BOOL:
        case AST_STRING:
        case AST_CHAR:
            break;
    }
}

/* AST в памяти не меняем. Упрощаем только картинку DOT. */
int ast_write_dot(FILE *out, const AstNode *root)
{
    if (!out || !root) return 0;
    /* Чёрно-белый граф: стандартные линии и рамки Graphviz. */
    fputs("digraph AST {\n"
          "  graph [rankdir=LR, nodesep=0.28, ranksep=0.65];\n"
          "  node [shape=box, fontname=\"DejaVu Sans\", fontsize=11];\n"
          "  edge [fontname=\"DejaVu Sans\", fontsize=9];\n", out);
    write_node(out, root);
    fputs("}\n", out);
    return !ferror(out);
}
