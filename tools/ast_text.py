#!/usr/bin/env python3
"""Показывает дерево из Graphviz DOT в терминале; парсер не изменяет."""

import argparse
from collections import defaultdict
from pathlib import Path
import re
import sys

# Считываем только узлы и связи в формате, который создаёт src/dot.c.
NODE = re.compile(r'^\s*(n[0-9A-Fa-f]+)\s+\[label="((?:[^"\\]|\\.)*)"\];\s*$')
EDGE = re.compile(r'^\s*(n[0-9A-Fa-f]+)\s*->\s*(n[0-9A-Fa-f]+)\s+\[label="((?:[^"\\]|\\.)*)"\];\s*$')
ESC = re.compile(r'\\([nrt"\\])')
ESCAPES = {'n': '\n', 'r': '\r', 't': '\t', '"': '"', '\\': '\\'}


def unescape(value):
    return ESC.sub(lambda match: ESCAPES[match.group(1)], value)


def load_dot(path):
    labels = {}
    links = defaultdict(list)
    child_ids = set()
    try:
        lines = Path(path).read_text(encoding='utf-8').splitlines()
    except (OSError, UnicodeError) as exc:
        raise ValueError(f'не могу прочитать {path}: {exc}') from exc

    for line in lines:
        node = NODE.fullmatch(line)
        if node:
            labels[node.group(1)] = unescape(node.group(2)).replace('\n', ' ')
            continue
        edge = EDGE.fullmatch(line)
        if edge:
            parent, child, role = edge.groups()
            links[parent].append((child, unescape(role)))
            child_ids.add(child)

    if not labels:
        raise ValueError('в DOT не нашлось узлов AST')
    roots = [node_id for node_id in labels if node_id not in child_ids]
    if not roots:
        raise ValueError('не удалось определить корень дерева')
    return labels, links, roots


def print_subtree(root, labels, links):
    # Стек позволяет печатать и большие деревья без рекурсивного обхода Python.
    print(labels.get(root, f'? {root}'))
    stack = []
    initial = links.get(root, [])
    for index in range(len(initial) - 1, -1, -1):
        child, _role = initial[index]
        stack.append((child, '', index == len(initial) - 1, frozenset({root})))

    while stack:
        node, prefix, last, ancestors = stack.pop()
        branch = '└── ' if last else '├── '
        text = labels.get(node, f'? {node}')
        if node in ancestors:
            print(prefix + branch + text + ' (цикл в DOT)')
            continue
        print(prefix + branch + text)
        children = links.get(node, [])
        next_prefix = prefix + ('    ' if last else '│   ')
        for index in range(len(children) - 1, -1, -1):
            child, _role = children[index]
            stack.append((child, next_prefix, index == len(children) - 1,
                          ancestors | {node}))


def main():
    # В MSYS2/Windows стандартный вывод может быть CP866,
    # а TXT-файлы и автотесты читают результат как UTF-8.
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

    parser = argparse.ArgumentParser(description='Показать AST текстом по готовому DOT')
    parser.add_argument('dot', help='путь к файлу .dot')
    parser.add_argument('--expressions', '-e', action='store_true',
                        help='показать только выражения внутри ExpressionStatement')
    args = parser.parse_args()
    try:
        labels, links, roots = load_dot(args.dot)
    except ValueError as exc:
        print(f'Ошибка: {exc}', file=sys.stderr)
        return 1

    if args.expressions:
        expr_roots = []
        for node_id, label in labels.items():
            if label == 'ExpressionStatement':
                expr_roots.extend(child for child, _role in links.get(node_id, []))
        if not expr_roots:
            print('В DOT нет операторов ExpressionStatement', file=sys.stderr)
            return 1
        for number, root in enumerate(expr_roots, 1):
            if number > 1:
                print()
            print(f'Выражение {number}:')
            print_subtree(root, labels, links)
    else:
        for index, root in enumerate(roots):
            if index:
                print()
            print_subtree(root, labels, links)
    return 0


if __name__ == '__main__':
    sys.exit(main())
