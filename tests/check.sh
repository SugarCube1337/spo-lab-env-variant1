#!/bin/sh
set -eu
binary=$1
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT HUP INT TERM

ok() {
  src=$1
  if ! "$binary" "$src" "$dir/tree.dot"; then
    echo "FAIL: rejected valid $src" >&2
    exit 1
  fi
  if ! grep -q 'digraph AST' "$dir/tree.dot"; then
    echo "FAIL: DOT missing" >&2
    exit 1
  fi
  if command -v dot >/dev/null 2>&1; then
    dot -Tsvg "$dir/tree.dot" -o "$dir/tree.svg"
    test -s "$dir/tree.svg"
  fi
}

bad() {
  name=$1
  content=$2
  needle=$3
  printf '%s\n' "$content" > "$dir/$name.spo"
  if "$binary" "$dir/$name.spo" "$dir/$name.dot" >"$dir/out" 2>"$dir/err"; then
    echo "FAIL: accepted invalid $name" >&2
    exit 1
  fi
  if ! grep -Fq "$needle" "$dir/err"; then
    echo "FAIL: wrong diagnostic $name" >&2
    cat "$dir/err" >&2
    exit 1
  fi
  test ! -e "$dir/$name.dot" || { echo "FAIL: wrote DOT for invalid $name" >&2; exit 1; }
}

# Пустой и состоящий из пробелов файл допустимы по sourceItem*.
: > "$dir/empty.spo"
ok "$dir/empty.spo"
if ! grep -Fq 'Program' "$dir/tree.dot"; then
  echo 'FAIL: no AST_PROGRAM for empty input' >&2
  exit 1
fi
printf '  \n\t\r\n' > "$dir/whitespace.spo"
ok "$dir/whitespace.spo"

# Старый пример должен разбираться без изменений.
ok examples/basic.spo
if ! grep -Fq 'Binary\n+' "$dir/tree.dot"; then
  echo 'FAIL: AST missing addition' >&2
  exit 1
fi
if ! grep -Fq 'Function\nmain' "$dir/tree.dot"; then
  echo 'FAIL: AST missing function' >&2
  exit 1
fi

# Новый пример: типы, литералы, массивы, ветвления и циклы.
ok examples/extended.spo
for label in 'ArrayType' 'Hex\n0x2A' 'Bits\n0b101' 'Bool\ntrue' \
             'String' 'Char' 'If' 'While' 'DoWhile' 'Break' \
             'Index' 'Unary\n!' 'Binary\n<<' 'Binary\n||'; do
  if ! grep -Fq "$label" "$dir/tree.dot"; then
    echo "FAIL: extended AST missing $label" >&2
    exit 1
  fi
done

# Проверяем, что у графа нет цветной темы.
if grep -Eq 'fillcolor|fontcolor|color=' "$dir/tree.dot"; then
  echo 'FAIL: в DOT осталась цветная тема' >&2
  exit 1
fi

# Заведомо неправильные программы.
bad missing_expr 'int main() { int x = ; }' "expected expression after '='"
bad top_level 'int x = 1;' 'syntax error'
bad unclosed 'int main() { sum(1, 2; }' 'syntax error'
bad junk 'int main() {} @' 'syntax error'
bad trailing_arg 'main() { sum(1,); }' 'syntax error'
bad trailing_param 'int f(int a,);' 'syntax error'
bad malformed_hex 'main() { int x = 0xG; }' 'syntax error'
bad bad_char 'main() { char c = '\''ab'\''; }' 'syntax error'
bad missing_semicolon 'main() { int x = 1 }' 'syntax error'

printf 'PASS: parser + CLI + DOT tests (variant 1 expanded)\n'
