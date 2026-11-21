# Сборка и проверка нативной Windows-версии

Уже работающую сборку WSL не трогаем. Windows-версия собирается **другим компилятором** из терминала **MSYS2 UCRT64** и попадает в `build/windows/spo-parser.exe`.

## 1. Открой терминал MSYS2 UCRT64

Лучше открыть обычную папку проекта в локальном VS Code на Windows (не в Remote-WSL). В терминале должны выполняться:

```bash
echo "$MSYSTEM"        # UCRT64
which gcc              # /ucrt64/bin/gcc
gcc -dumpmachine       # x86_64-w64-mingw32 или аналогично
```

Если инструменты не установлены, **в MSYS2 UCRT64**:

```bash
pacman -S --needed base-devel git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-graphviz mingw-w64-ucrt-x86_64-python
```

Проверить `make --version`, `dot -V` и `python3 --version`. Если доступна только команда `python`, используй `make PYTHON=python check`.

## 2. Установи PackCC для Windows

PackCC — генератор парсера на C. Его собираем один раз нативным `gcc`. В MSYS2 UCRT64:

```bash
mkdir -p "$HOME/tools/packcc"
git clone https://github.com/Leandros/PackCC.git "$HOME/tools/packcc/source"
gcc -O2 -o "$HOME/tools/packcc/packcc.exe" "$HOME/tools/packcc/source/src/packcc.c"
"$HOME/tools/packcc/packcc.exe" -h
```

Последняя команда может напечатать справку или сообщение о параметрах, главное — чтобы сам `.exe` запускался. Если PackCC уже есть, укажи его через `make PACKCC=/путь/к/packcc.exe`.

## 3. Собери и проверь Windows `.exe`

Из **корня проекта** в том же терминале:

```bash
make doctor-windows
make verify-windows
```

`verify-windows` выполняет сборку, все C-тесты, CLI-тесты и генерацию DOT, TXT, SVG для примеров. Никакой Valgrind здесь не запускается; его проверка остаётся в WSL (`make memcheck`).

Если нужна только сборка:

```bash
make
```

Должен появиться **`build/windows/spo-parser.exe`**. Проверь, что это не Linux-бинарник:

```bash
objdump -f build/windows/spo-parser.exe
```

В заголовке ожидается формат PE (`pei-x86-64`).

## 4. Запусти программу нативно

В MSYS2 UCRT64:

```bash
./build/windows/spo-parser.exe examples/teacher/all.spo build/windows/teacher.dot
dot -Tsvg build/windows/teacher.dot -o build/windows/teacher.svg
```

После этого открой PowerShell в **той же папке проекта** и запусти готовую программу **без WSL и без терминала MSYS2**:

```powershell
.\build\windows\spo-parser.exe examples\teacher\all.spo build\windows\teacher-powershell.dot
Test-Path .\build\windows\teacher-powershell.dot
```

Ожидается `True`. Это проверяет, что программа запускается как приложение Windows, а не через WSL. Graphviz в PowerShell не нужен — `.dot` уже есть.

## Частые проблемы

- **`make: command not found`:** не установлен пакет `base-devel` либо выбран PowerShell вместо MSYS2 для сборки.
- **`gcc` из `/usr/bin/` и не Windows:** открыт терминал MSYS, а нужен **MSYS2 UCRT64**.
- **`.../packcc.exe: No such file or directory`:** закончи шаг 2 или укажи `PACKCC=/другой/путь/packcc.exe`.
- **`dot: command not found`:** установи `mingw-w64-ucrt-x86_64-graphviz`.
- **Нет `spo-parser.exe`:** проверь ошибки сборки выше; `make` не создаёт бинарник при неуспешной генерации или компиляции.
- **`.exe` не запускается в PowerShell:** сохрани полный текст ошибки; дополнительно смотри `objdump -p build/windows/spo-parser.exe`, блок `DLL Name`.

Windows-сборка и Linux-сборка используют общие исходники, но разные каталоги `build/windows/` и `build/linux/`. Команда `make clean` удаляет **всю** папку `build/`.

## Альтернатива: собрать `.exe` прямо из WSL

Если не хочется ставить PackCC второй раз, можно использовать **Linux-PackCC** и кросс-компилятор MinGW в WSL:

```bash
sudo apt update
sudo apt install gcc-mingw-w64-x86-64
make OS=Windows_NT CC=x86_64-w64-mingw32-gcc \
  PACKCC="$HOME/tools/packcc/packcc" all
file build/windows/spo-parser.exe
```

Это **кросс-сборка**, а не проверка запуска. Для подтверждения работы запусти `build/windows/spo-parser.exe` из обычного Windows PowerShell по инструкции выше. `make verify-windows` следует выполнять именно в MSYS2 UCRT64, где тестовые Windows-EXE можно запускать напрямую.
