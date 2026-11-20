#!/bin/sh
set -eu

# Проверяем успешный и ошибочный разбор; код ошибки от парсера ожидаем.
binary=$1
parser_test=$2
if ! command -v valgrind >/dev/null 2>&1; then
  echo 'Не установлен Valgrind: sudo apt install valgrind' >&2
  exit 1
fi

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

run_case() {
  title=$1
  expected_status=$2
  shift 2
  set +e
  valgrind --leak-check=full --show-leak-kinds=all \
    --errors-for-leak-kinds=all --error-exitcode=99 "$@" \
    >"$tmpdir/$title.stdout" 2>"$tmpdir/$title.stderr"
  actual_status=$?
  set -e
  if [ "$actual_status" -ne "$expected_status" ]; then
    echo "FAIL: Valgrind ($title), exit=$actual_status, expected=$expected_status" >&2
    cat "$tmpdir/$title.stderr" >&2
    exit 1
  fi
  if ! grep -Fq 'ERROR SUMMARY: 0 errors' "$tmpdir/$title.stderr"; then
    echo "FAIL: Valgrind ($title): есть ошибки" >&2
    cat "$tmpdir/$title.stderr" >&2
    exit 1
  fi
  if ! grep -Fq 'All heap blocks were freed -- no leaks are possible' "$tmpdir/$title.stderr"; then
    echo "FAIL: Valgrind ($title): есть неосвобождённая память" >&2
    cat "$tmpdir/$title.stderr" >&2
    exit 1
  fi
  echo "PASS: Valgrind $title (ошибок памяти и утечек нет)"
}

run_case valid 0 "$binary" examples/extended.spo "$tmpdir/valid.dot"
run_case invalid 1 "$binary" examples/invalid.spo "$tmpdir/invalid.dot"
run_case unit 0 "$parser_test"
