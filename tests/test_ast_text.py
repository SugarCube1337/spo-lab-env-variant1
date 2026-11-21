#!/usr/bin/env python3
"""Быстрая проверка текстового просмотра AST без компиляции парсера."""

from pathlib import Path
import os
import subprocess
import sys
import tempfile


DOT = r'''digraph AST {
  graph [rankdir=LR];
  n100 [label="Program"];
  n100 -> n200 [label="функция"];
  n200 [label="Function\nmain"];
  n200 -> n300 [label="тело"];
  n300 [label="ExpressionStatement"];
  n300 -> n400 [label="выражение"];
  n400 [label="Binary\n="];
  n400 -> n500 [label="левый"];
  n500 [label="Identifier\nx"];
  n400 -> n600 [label="правый"];
  n600 [label="Binary\n+"];
  n600 -> n700 [label="левый"];
  n700 [label="Integer\n2"];
  n600 -> n800 [label="правый"];
  n800 [label="Binary\n*"];
  n800 -> n900 [label="левый"];
  n900 [label="Integer\n3"];
  n800 -> na00 [label="правый"];
  na00 [label="Integer\n4"];
}
'''


def run(*args, output_encoding=None):
    env = os.environ.copy()
    if output_encoding:
        env["PYTHONIOENCODING"] = output_encoding
    return subprocess.run(
        [sys.executable, "tools/ast_text.py", *map(str, args)],
        capture_output=True, text=True, encoding="utf-8", check=False,
        env=env,
    )


def main():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "tree.dot"
        path.write_text(DOT, encoding="utf-8")
        full = run(path)
        assert full.returncode == 0, full.stderr
        assert "Program" in full.stdout
        assert "Function main" in full.stdout
        assert "Binary +" in full.stdout
        assert "└── Binary *" in full.stdout

        # Имитация Windows, где дочерний Python может выбрать CP866.
        # Просмотрщик обязан всё равно выдавать UTF-8.
        windows_full = run(path, output_encoding="cp866")
        assert windows_full.returncode == 0, windows_full.stderr
        assert "└── Binary *" in windows_full.stdout

        expressions = run("--expressions", path)
        assert expressions.returncode == 0, expressions.stderr
        assert expressions.stdout.startswith("Выражение 1:\nBinary =")
        assert "├── Identifier x" in expressions.stdout
        assert "└── Binary +" in expressions.stdout
        assert "└── Binary *" in expressions.stdout

        missing = run(Path(tmp) / "missing.dot")
        assert missing.returncode != 0 and "Ошибка" in missing.stderr

        bad = Path(tmp) / "bad.dot"
        bad.write_text("digraph AST {\n}\n", encoding="utf-8")
        invalid = run(bad)
        assert invalid.returncode != 0 and "узлов AST" in invalid.stderr

    print("PASS: text AST viewer (tree, expressions and errors)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
