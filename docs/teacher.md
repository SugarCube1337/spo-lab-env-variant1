# Пример преподавателя: как показать арифметическое дерево

Исходный пример написан **на другом языке**. В нашем варианте функции записываются иначе:

```c
int main() {
    x = a + b * c + d;
    x = a * b + c * d;
    z + 4 = f(1, 2, 3) = 3;
}
```

Готовый файл: `examples/teacher/all.spo`. Исходник намеренно оставлен без объявлений переменных и `f`: наша лабораторная **проверяет синтаксис**, а не существование имён.

## Запуск в WSL

```bash
make check              # среди тестов есть test-teacher
make teacher            # отдельный DOT + SVG для каждого выражения
```

Результаты: `build/linux/teacher/01-add-multiply.svg`, `02-two-products.svg`, `03-assignment-chain.svg`, `all.svg`. Открывай `.dot` через Graphviz Interactive Preview в VS Code либо `.svg` обычным просмотрщиком.

## Как читать деревья

**1. `x = a + b * c + d;`**

```text
Binary =
├── Identifier x
└── Binary +
    ├── Binary +
    │   ├── Identifier a
    │   └── Binary *
    │       ├── Identifier b
    │       └── Identifier c
    └── Identifier d
```

Умножение сильнее сложения, а два `+` группируются слева: `(a + b * c) + d`.

**2. `x = a * b + c * d;`**

```text
Binary =
├── Identifier x
└── Binary +
    ├── Binary *
    │   ├── Identifier a
    │   └── Identifier b
    └── Binary *
        ├── Identifier c
        └── Identifier d
```

Два узла умножения находятся внутри сложения.

**3. `z + 4 = f(1, 2, 3) = 3;`**

```text
Binary =
├── Binary +
│   ├── Identifier z
│   └── Integer 4
└── Binary =
    ├── Call
    │   ├── Identifier f
    │   └── Integer 1, 2, 3 (три аргумента в AST_LIST)
    └── Integer 3
```

Правило `assignment <- logical_or '=' assignment / logical_or` разбирает присваивания справа налево. Его левая часть не ограничена местом хранения значения, поэтому **синтаксический** парсер принимает даже выражение `z + 4` слева. Это **не означает**, что такое присваивание допустимо при исполнении: семантическую проверку мы не делаем.

## Где показать исходники

| Спросили | Открой | Ищи |
|---|---|---|
| Откуда приоритеты? | `grammar/language.peg` | `additive <-`, `multiplicative <-` |
| Почему `+` группируется слева? | `grammar/language.peg` | `additive <- l:additive` |
| Почему `=` группируется справа? | `grammar/language.peg` | `assignment <- l:logical_or`, `r:assignment` |
| Откуда узлы дерева? | `grammar/language.peg` | `ast_make(auxil, AST_BINARY` |
| Где объявлены виды узлов? | `include/ast.h` | `AstKind`, `AstNode` |
| Где проверена форма дерева? | `tests/test_teacher.c` | `first_expression`, `second_expression`, `third_expression` |
| Где формируется `.dot`? | `src/dot.c` | `write_node`, `ast_write_dot` |

**Что сказать при защите:** «Тест `test-teacher` проверяет не только успешный разбор текста, но и конкретную форму AST: умножение ниже сложения, левоассоциативное сложение и правоассоциативное присваивание». Готовое дерево **не вычисляет** значения выражений.
