# Проверка лабораторной №1 по шагам

Все команды выполнять из корня `spo-lab-env-variant1` **в терминале WSL Ubuntu**. `make check` не заменяет Valgrind, поэтому проверки идут отдельно.

## Шаг 0. Сохранить предыдущую версию

```bash
git status
git add . && git commit -m "chore: checkpoint before final checks"
```

Если нет изменений для коммита, это нормально. После этого скопируй файлы обновления в корень проекта.

## Шаг 1. Проверить инструменты

```bash
make doctor
```

Ожидаем: `OK: GCC, Graphviz, Valgrind и PackCC доступны`.

Если PackCC не в `~/tools/packcc/packcc`, укажи путь явно:

```bash
make PACKCC=/путь/к/packcc doctor
```

## Шаг 2. Чистая сборка

```bash
make clean
make
```

Ожидаем появление `build/linux/spo-parser`. Генерируется `build/linux/language.c` из `grammar/language.peg`, а затем собирается C-программа. Предупреждения из нашего `src/` требуют внимания; для кода PackCC отдельно отключены только `unused-parameter` и `shadow`.

## Шаг 3. Автоматические тесты

```bash
make check
```

Ожидаем строки `PASS:` для AST, диагностики, DOT, парсера, пограничных случаев, арифметических деревьев преподавателя и CLI. Новый `test-spec` содержит **32 корректных и 23 некорректных** примера (количество также печатается самим тестом). Если тест не прошёл, сохрани полный вывод и **не переходи к коммиту финальной версии**.

Что тестируем: пустые файлы, объявления и определения функций, сигнатуры, все виды типов и литералов, приоритеты и ассоциативность операторов, массивы, вызовы, вложенные `if-else`, циклы, неверные токены, диагностические позиции и очистку результата.

## Шаг 4. Получить и проверить граф

```bash
make visualize-extended
```

Ожидаем:

```text
AST готов: build/linux/extended.svg
```

Открой `build/linux/extended.dot` в **Graphviz Interactive Preview**, приблизь ветви `If`, `While`, `DoWhile`, `Binary`, `Call` и `Index`. `extended.svg` можно открыть обычным просмотрщиком.

## Шаг 4а. Подготовить маленькие примеры для защиты

```bash
make demo
make demo-errors
```

Десять графов находятся в `build/linux/demo/*.svg`. Сравни `01-priority.svg` и `02-parentheses.svg`.
Шпаргалка с объяснением правил и указателями на код — `docs/defense.md`.
Пустой пример `10-empty.spo` должен дать один узел `Program`.

## Шаг 4б. Проверить пример преподавателя

```bash
make teacher
```

Готовые чёрно-белые графы `build/linux/teacher/*.svg`. Отдельный `build/linux/test-teacher` проверяет структуру AST каждого выражения. Подробности в `docs/teacher.md`.

## Шаг 5. Проверить ошибки вручную

```bash
./build/linux/spo-parser examples/invalid.spo build/linux/invalid.dot
echo $?
```

Ожидаем: строка с именем файла, номером строки, столбцом и `syntax error`, затем **ненулевой код** (`1` для текущего CLI). Если выходного файла не было до запуска, новый DOT не должен появиться.

## Шаг 6. Valgrind — корректные и ошибочные входы

```bash
make memcheck
```

Команда запускает три проверки Valgrind: большой корректный исходник, некорректный файл и тест API. У успешного разбора ожидается код `0`, у некорректного — `1`: это **ожидаемая ошибка синтаксиса**, а не ошибка памяти.

Для каждой проверки ожидаем:

```text
PASS: Valgrind ... (ошибок памяти и утечек нет)
```

Если хочешь увидеть подробный журнал, отдельно запусти:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --error-exitcode=99 build/linux/spo-parser \
  examples/extended.spo build/linux/valgrind.dot
```

В успешном случае: `in use at exit: 0 bytes`, `ERROR SUMMARY: 0 errors`.

## Шаг 7. Максимально строгая компиляция

```bash
make strict
```

Используется отдельная папка `build/linux-strict`, `-Werror` и тот же набор тестов. Обычный `build/linux` при этом не удаляется.

По желанию можно запустить:

```bash
make asan-check
```

Это дополнительная проверка AddressSanitizer/UndefinedBehaviorSanitizer. Если конкретная конфигурация WSL с ASan работает нестабильно, зафиксируй проблему отдельно; результаты Valgrind не подменяй.

## Шаг 8. Отладка и просмотр AST

Открой `src/main.c` и поставь breakpoint сразу после `result = parse_source_n(source, length);`. Нажми **F5**, добавь `result.root` в Watch и раскрой `left/right`. Посмотри вложенное выражение `2 + 3 * 4`: узел `+` должен содержать узел `*` справа.

## Шаг 9. Сдать код и результаты

```bash
git status
git add .
git commit -m "test: finalize lab1 parser checks"
```

В коммит попадают исходники, тесты, README и документация, но не `build/`. Для отчёта сохраните скриншот графа, скриншот отладки и вывод `make check` / Valgrind.

**Обязательно:** даже после всех зелёных тестов остаётся отдельная задача — проверить и устранить аварийное завершение сгенерированного PackCC при отказе его внутренних выделений памяти. Это не покрывает обычный `make memcheck`, который проверяет только штатное выделение памяти.

## Проверка под Windows

Сборка нативного `.exe` в MSYS2 UCRT64 описана в `docs/windows.md`. Вместо Linux-команды `make verify` там используется `make verify-windows`, без Valgrind.
