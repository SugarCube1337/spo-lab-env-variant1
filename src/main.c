#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    size_t value;
    struct Node *next;
} Node;

static int parse_count(const char *input, size_t *result)
{
    if (input == NULL || input[0] == '\0' || input[0] == '-') {
        return 0;
    }

    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(input, &end, 10);
    if (errno != 0 || end == input || *end != '\0' ||
        value == 0 || value > SIZE_MAX) {
        return 0;
    }

    *result = (size_t)value;
    return 1;
}

static void free_list(Node *head)
{
    while (head != NULL) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

int main(int argc, char **argv)
{
    size_t count = 0;
    if (argc != 2 || !parse_count(argv[1], &count)) {
        fprintf(stderr, "Usage: %s <positive-node-count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Node *head = NULL;
    Node *tail = NULL;
    for (size_t i = 0; i < count; ++i) {
        Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            fputs("Failed to allocate a list node\n", stderr);
            free_list(head);
            return EXIT_FAILURE;
        }
        node->value = i;
        node->next = NULL;

        if (tail != NULL) {
            tail->next = node;
        } else {
            head = node;
        }
        tail = node;
    }

    /* Breakpoint here: inspect head -> next -> next in the debugger. */
    for (const Node *current = head; current != NULL; current = current->next) {
        printf("Node: %zu\n", current->value);
    }

    free_list(head);
    return EXIT_SUCCESS;
}
