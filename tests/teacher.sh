#!/bin/sh
set -eu

# Четыре маленьких дерева: три выражения отдельно и все вместе.
binary=$1
outdir=$2
mkdir -p "$outdir"

for source in examples/teacher/0*.spo examples/teacher/all.spo; do
    name=$(basename "$source" .spo)
    "$binary" "$source" "$outdir/$name.dot"
    dot -Tsvg "$outdir/$name.dot" -o "$outdir/$name.svg"
    if [ ! -s "$outdir/$name.svg" ]; then
        echo "FAIL: не удалось построить $outdir/$name.svg" >&2
        exit 1
    fi
    printf 'OK: %s -> %s/%s.svg\n' "$source" "$outdir" "$name"
done
printf 'Готово: 4 чёрно-белых дерева для примера преподавателя.\n'
