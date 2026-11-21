# СПО — лабораторная №1, вариант 1

Парсер на **C + PackCC**: разбирает текст, строит AST и сохраняет его в Graphviz DOT. Сам код не выполняет.

## Сборка и проверка (WSL)

```bash
make -j4             # собрать парсер
make -j4 check       # тесты и деревья DOT/TXT/SVG
make memcheck        # проверка памяти (Valgrind)
```

Результаты `make check` находятся в `build/linux/`: `basic.*`, `extended.*` и `demo/*` (`.dot`, `.txt`, `.svg`).

## Свой пример

Создай файл `examples/own.spo`, например:

```c
int main() {
    int x = 2 + 3 * 4;
    x = (2 + 3) * 4;
}
```

```bash
make example INPUT=examples/own.spo OUTPUT=build/linux/own
cat build/linux/own.txt
```

Результаты: `build/linux/own.dot` (Graphviz), `own.txt` (дерево в терминале), `own.svg` (изображение). В VS Code можно открыть `.svg` или `.dot` через Graphviz Preview.

## Другие команды

```bash
make demo             # небольшие примеры с деревьями
make demo-errors      # примеры синтаксических ошибок
make visualize        # дерево basic.spo
make verify-windows   # проверка сборки в MSYS2 UCRT64 (Windows)
```

## Важно

- Правила языка и приоритеты операций: `grammar/language.peg`.
- API парсера: `include/parser.h`, `src/parser.c`; AST: `include/ast.h`.
- CLI `src/main.c` принимает **входной файл** и **выходной `.dot`** через аргументы, ошибки печатает в `stderr`. Сам модуль анализа ничего не печатает.
- Парсер проверяет **синтаксис**, а не смысл программы. `return` не добавлен, поскольку его нет в грамматике варианта.
- Не запускай `make clean` при каждом изменении: обычный `make` пересобирает только изменённые файлы.
