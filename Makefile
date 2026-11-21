CC = gcc
CFLAGS ?= -std=c11 -g3 -O0 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=
PYTHON ?= python3

# Эти два предупреждения отключаем только у кода, созданного PackCC.
# Свой код в src/ продолжаем проверять строго.
GENERATED_CFLAGS := -Wno-unused-parameter -Wno-shadow \
                    -Wno-pedantic -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
  GENERATED_CFLAGS += -Wno-error=implicit-int
endif

ifeq ($(OS),Windows_NT)
  # В MSYS2 UCRT64 у генератора расширение .exe.
  PLATFORM := windows
  EXE := .exe
  PACKCC ?= $(HOME)/tools/packcc/packcc.exe
else
  PLATFORM := linux
  EXE :=
  PACKCC ?= $(HOME)/tools/packcc/packcc
endif

BUILD_DIR := build/$(PLATFORM)
TARGET := $(BUILD_DIR)/spo-parser$(EXE)
DIAG_TEST := $(BUILD_DIR)/test-diagnostics$(EXE)
AST_TEST := $(BUILD_DIR)/test-ast$(EXE)
DOT_TEST := $(BUILD_DIR)/test-dot$(EXE)
PARSER_TEST := $(BUILD_DIR)/test-parser$(EXE)
SPEC_TEST := $(BUILD_DIR)/test-spec$(EXE)
PARSER_GENERATED := $(BUILD_DIR)/language.c $(BUILD_DIR)/language.h
SOURCES := src/main.c src/parser.c src/ast.c src/diagnostics.c src/dot.c
PROJECT_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
GENERATED_OBJECT := $(BUILD_DIR)/language.o
INCLUDE_FLAGS := -I. -Iinclude -Isrc -I$(BUILD_DIR)

# Набор файлов для показа: DOT (граф), TXT (дерево), SVG (картинка).
# У make есть зависимости: без изменений пересоздавать эти файлы не нужно.
DEMO_NAMES := $(patsubst examples/defense/valid/%.spo,%,$(wildcard examples/defense/valid/*.spo))
OUTPUT_BASES := $(BUILD_DIR)/basic $(BUILD_DIR)/extended \
                $(addprefix $(BUILD_DIR)/demo/,$(DEMO_NAMES))
OUTPUT_FILES := $(addsuffix .dot,$(OUTPUT_BASES)) \
                $(addsuffix .txt,$(OUTPUT_BASES)) \
                $(addsuffix .svg,$(OUTPUT_BASES))
DEMO_BASES := $(addprefix $(BUILD_DIR)/demo/,$(DEMO_NAMES))
DEMO_FILES := $(addsuffix .dot,$(DEMO_BASES)) \
              $(addsuffix .txt,$(DEMO_BASES)) \
              $(addsuffix .svg,$(DEMO_BASES))

# Для своего файла: make example INPUT=examples/own.spo
INPUT ?= examples/basic.spo
OUTPUT ?= $(BUILD_DIR)/custom

# Не оставляем частично созданные TXT/SVG после ошибки.
.DELETE_ON_ERROR:
.DEFAULT_GOAL := all
.PHONY: all check test-unit run visualize visualize-extended demo demo-errors example artifacts debug clean doctor doctor-windows memcheck verify verify-windows strict asan-check

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

# В MinGW новая библиотека уже содержит strnlen(), а PackCC 1.2.0
# по старому шаблону добавляет ещё одну. Убираем только копию генератора.
# -I. требуется потому, что PackCC пишет #include "build/windows/language.h".
$(PARSER_GENERATED) &: grammar/language.peg Makefile | $(BUILD_DIR)
	$(PACKCC) -o $(BUILD_DIR)/language grammar/language.peg
ifeq ($(OS),Windows_NT)
	sed -i '/^static size_t strnlen(const char \*str, size_t maxlen) {/,/^}/d' $(BUILD_DIR)/language.c
endif

# Наши C-файлы: полная проверка предупреждений.
$(BUILD_DIR)/%.o: src/%.c $(wildcard include/*.h) src/parser_internal.h Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

# parser.c использует language.h, поэтому сначала создаём его.
$(BUILD_DIR)/parser.o: $(BUILD_DIR)/language.h

# Файл PackCC компилируем отдельно, чтобы не засорял вывод.
$(GENERATED_OBJECT): $(PARSER_GENERATED) Makefile | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GENERATED_CFLAGS) $(INCLUDE_FLAGS) -c $(BUILD_DIR)/language.c -o $@

$(TARGET): $(PROJECT_OBJECTS) $(GENERATED_OBJECT)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(DIAG_TEST): tests/test_diagnostics.c src/diagnostics.c include/diagnostics.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude tests/test_diagnostics.c src/diagnostics.c -o $@ $(LDFLAGS) $(LDLIBS)

$(AST_TEST): tests/test_ast.c src/ast.c include/ast.h src/parser_internal.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Isrc tests/test_ast.c src/ast.c -o $@ $(LDFLAGS) $(LDLIBS)

$(DOT_TEST): tests/test_dot.c src/dot.c src/ast.c include/ast.h include/dot.h src/parser_internal.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Isrc tests/test_dot.c src/dot.c src/ast.c -o $@ $(LDFLAGS) $(LDLIBS)

$(PARSER_TEST): tests/test_parser.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) tests/test_parser.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT) -o $@ $(LDFLAGS) $(LDLIBS)

$(SPEC_TEST): tests/test_spec.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) tests/test_spec.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT) -o $@ $(LDFLAGS) $(LDLIBS)

test-unit: $(DIAG_TEST) $(AST_TEST) $(DOT_TEST) $(PARSER_TEST) $(SPEC_TEST)
	$(DIAG_TEST)
	$(AST_TEST)
	$(DOT_TEST)
	$(PARSER_TEST)
	$(SPEC_TEST)

# Основная проверка: тесты + готовые картинки и текстовые деревья.
# Файлы создаём только после успешных тестов.
check: all test-unit
	$(PYTHON) tests/test_ast_text.py
	sh tests/check.sh $(TARGET)
	$(MAKE) --no-print-directory artifacts

# Разбор из examples/ в отдельные .dot, .txt, .svg.
# Например: examples/basic.spo -> build/linux/basic.*
$(BUILD_DIR)/%.dot: examples/%.spo $(TARGET)
	@mkdir -p "$(dir $@)"
	$(TARGET) "$<" "$@"

# Для демонстрационных файлов сохраним старый путь build/linux/demo/.
$(BUILD_DIR)/demo/%.dot: examples/defense/valid/%.spo $(TARGET)
	@mkdir -p "$(dir $@)"
	$(TARGET) "$<" "$@"

# Обычное текстовое дерево: Program -> Function -> ...
$(BUILD_DIR)/%.txt: $(BUILD_DIR)/%.dot tools/ast_text.py
	$(PYTHON) tools/ast_text.py "$<" > "$@"

$(BUILD_DIR)/%.svg: $(BUILD_DIR)/%.dot
	dot -Tsvg "$<" -o "$@"

artifacts: $(OUTPUT_FILES)
	@printf 'Готовы DOT, TXT и SVG: %s/ (basic, extended, demo)\n' '$(BUILD_DIR)'

run: $(BUILD_DIR)/basic.dot
	@printf 'DOT готов: %s\n' '$(BUILD_DIR)/basic.dot'

visualize: $(BUILD_DIR)/basic.dot $(BUILD_DIR)/basic.txt $(BUILD_DIR)/basic.svg
	@printf 'Дерево готово: %s/basic.{dot,txt,svg}\n' '$(BUILD_DIR)'

visualize-extended: $(BUILD_DIR)/extended.dot $(BUILD_DIR)/extended.txt $(BUILD_DIR)/extended.svg
	@printf 'Дерево готово: %s/extended.{dot,txt,svg}\n' '$(BUILD_DIR)'

# Все маленькие примеры для защиты (текст + картинка).
demo: $(DEMO_FILES)
	@printf 'Примеры для защиты готовы: %s/demo/\n' '$(BUILD_DIR)'

# Свой пример: OUTPUT задаётся без расширения, например build/linux/mine.
example: all
	@mkdir -p "$(dir $(OUTPUT))"
	$(TARGET) "$(INPUT)" "$(OUTPUT).dot"
	$(PYTHON) tools/ast_text.py "$(OUTPUT).dot" > "$(OUTPUT).txt"
	dot -Tsvg "$(OUTPUT).dot" -o "$(OUTPUT).svg"
	@printf 'Готово: %s.{dot,txt,svg}\n' '$(OUTPUT)'

demo-errors: all
	sh tests/demo-errors.sh $(TARGET) $(BUILD_DIR)/demo

debug: all
	gdb --args $(TARGET) examples/basic.spo $(BUILD_DIR)/basic.dot

# Проверяем инструменты перед финальным прогоном.
doctor:
	@command -v $(CC) >/dev/null || { echo 'Не найден компилятор $(CC)' >&2; exit 1; }
	@command -v dot >/dev/null || { echo 'Не найден Graphviz (dot)' >&2; exit 1; }
	@command -v $(PYTHON) >/dev/null || { echo 'Не найден Python: $(PYTHON)' >&2; exit 1; }
	@command -v valgrind >/dev/null || { echo 'Не найден Valgrind' >&2; exit 1; }
	@test -x '$(PACKCC)' || { echo 'Не найден PackCC: $(PACKCC)' >&2; exit 1; }
	@printf 'OK: GCC, Graphviz, Python, Valgrind и PackCC доступны\n'

# Windows проверяем отдельно: Valgrind предназначен для Linux.
doctor-windows:
	@test '$(MSYSTEM)' = 'UCRT64' || { echo 'Открой терминал MSYS2 UCRT64' >&2; exit 1; }
	@command -v $(CC) >/dev/null || { echo 'Не найден GCC' >&2; exit 1; }
	@command -v dot >/dev/null || { echo 'Не найден Graphviz (dot)' >&2; exit 1; }
	@command -v $(PYTHON) >/dev/null || { echo 'Не найден Python: $(PYTHON)' >&2; exit 1; }
	@test -x '$(PACKCC)' || { echo 'Не найден PackCC: $(PACKCC)' >&2; exit 1; }
	@$(CC) -dumpmachine | grep -q 'w64-mingw32' || { echo 'Это не компилятор Windows UCRT64' >&2; exit 1; }
	@printf 'OK: MSYS2 UCRT64, GCC, Graphviz, Python и PackCC доступны\n'

verify-windows: doctor-windows check
	@printf 'ГОТОВО: native Windows .exe, тесты и графы собраны\n'

# Valgrind проверяет и успешный, и неуспешный разбор.
memcheck: all $(PARSER_TEST)
	sh tests/memcheck.sh $(TARGET) $(PARSER_TEST)

# Строгая компиляция в отдельный каталог (не трогаем обычный build/linux).
strict:
	$(MAKE) BUILD_DIR=build/$(PLATFORM)-strict CFLAGS='$(CFLAGS) -Werror' check

# Дополнительно проверяем AddressSanitizer/UndefinedBehaviorSanitizer.
# Если WSL выдаст проблемы с ASan, сам результат тестов через Valgrind важнее.
asan-check:
	$(MAKE) BUILD_DIR=build/$(PLATFORM)-asan CFLAGS='$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='$(LDFLAGS) -fsanitize=address,undefined' check

# Один запуск для подготовки доказательств работоспособности.
verify: doctor check memcheck visualize-extended
	@printf 'ГОТОВО: тесты, Valgrind и SVG завершены успешно\n'

clean:
	rm -rf build
