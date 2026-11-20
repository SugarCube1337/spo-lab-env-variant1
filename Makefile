CC = gcc
CFLAGS ?= -std=c11 -g3 -O0 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

# Эти два предупреждения отключаем только у кода, созданного PackCC.
# Свой код в src/ продолжаем проверять строго.
GENERATED_CFLAGS := -Wno-unused-parameter -Wno-shadow

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
TEACHER_TEST := $(BUILD_DIR)/test-teacher$(EXE)
PARSER_GENERATED := $(BUILD_DIR)/language.c $(BUILD_DIR)/language.h
SOURCES := src/main.c src/parser.c src/ast.c src/diagnostics.c src/dot.c
PROJECT_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
GENERATED_OBJECT := $(BUILD_DIR)/language.o
INCLUDE_FLAGS := -Iinclude -Isrc -I$(BUILD_DIR)

.DEFAULT_GOAL := all
.PHONY: all check test-unit run visualize visualize-extended demo demo-errors teacher debug clean doctor doctor-windows memcheck verify verify-windows strict asan-check

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(PARSER_GENERATED) &: grammar/language.peg | $(BUILD_DIR)
	$(PACKCC) -o $(BUILD_DIR)/language grammar/language.peg

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

# Отдельный тест формы AST для трёх выражений преподавателя.
$(TEACHER_TEST): tests/test_teacher.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) tests/test_teacher.c $(filter-out $(BUILD_DIR)/main.o,$(PROJECT_OBJECTS)) $(GENERATED_OBJECT) -o $@ $(LDFLAGS) $(LDLIBS)

test-unit: $(DIAG_TEST) $(AST_TEST) $(DOT_TEST) $(PARSER_TEST) $(SPEC_TEST) $(TEACHER_TEST)
	$(DIAG_TEST)
	$(AST_TEST)
	$(DOT_TEST)
	$(PARSER_TEST)
	$(SPEC_TEST)
	$(TEACHER_TEST)

check: all test-unit
	sh tests/check.sh $(TARGET)

run: all
	$(TARGET) examples/basic.spo $(BUILD_DIR)/basic.dot

# Строим чёрно-белый SVG из готового DOT.
# В VS Code лучше открывать сам .dot через интерактивный просмотр.
visualize: run
	dot -Tsvg $(BUILD_DIR)/basic.dot -o $(BUILD_DIR)/basic.svg
	@printf 'AST готов: %s\n' '$(BUILD_DIR)/basic.svg'

# Большой пример: все новые конструкции варианта 1.
visualize-extended: all
	$(TARGET) examples/extended.spo $(BUILD_DIR)/extended.dot
	dot -Tsvg $(BUILD_DIR)/extended.dot -o $(BUILD_DIR)/extended.svg
	@printf 'AST готов: %s\n' '$(BUILD_DIR)/extended.svg'

# Десять небольших графов: удобнее показывать на защите, чем большой extended.svg.
demo: all
	sh tests/demo.sh $(TARGET) $(BUILD_DIR)/demo

# Четыре чёрно-белых дерева по примеру преподавателя.
teacher: all
	sh tests/teacher.sh $(TARGET) $(BUILD_DIR)/teacher

# Два коротких примера синтаксических ошибок — диагностик в stderr.
demo-errors: all
	sh tests/demo-errors.sh $(TARGET) $(BUILD_DIR)/demo

debug: all
	gdb --args $(TARGET) examples/basic.spo $(BUILD_DIR)/basic.dot

# Проверяем инструменты перед финальным прогоном.
doctor:
	@command -v $(CC) >/dev/null || { echo 'Не найден компилятор $(CC)' >&2; exit 1; }
	@command -v dot >/dev/null || { echo 'Не найден Graphviz (dot)' >&2; exit 1; }
	@command -v valgrind >/dev/null || { echo 'Не найден Valgrind' >&2; exit 1; }
	@test -x '$(PACKCC)' || { echo 'Не найден PackCC: $(PACKCC)' >&2; exit 1; }
	@printf 'OK: GCC, Graphviz, Valgrind и PackCC доступны\n'

# Windows проверяем отдельно: Valgrind предназначен для Linux.
doctor-windows:
	@test '$(MSYSTEM)' = 'UCRT64' || { echo 'Открой терминал MSYS2 UCRT64' >&2; exit 1; }
	@command -v $(CC) >/dev/null || { echo 'Не найден GCC' >&2; exit 1; }
	@command -v dot >/dev/null || { echo 'Не найден Graphviz (dot)' >&2; exit 1; }
	@test -x '$(PACKCC)' || { echo 'Не найден PackCC: $(PACKCC)' >&2; exit 1; }
	@$(CC) -dumpmachine | grep -q 'w64-mingw32' || { echo 'Это не компилятор Windows UCRT64' >&2; exit 1; }
	@printf 'OK: MSYS2 UCRT64, GCC, Graphviz и PackCC доступны\n'

verify-windows: doctor-windows check teacher
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
