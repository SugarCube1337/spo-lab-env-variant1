# SPO Labs — Variant 1: environment smoke test

This is an **environment check**, not an implementation of Lab 1.

## WSL / Ubuntu

Install: `sudo apt update && sudo apt install -y build-essential gdb git flex bison graphviz valgrind`

```sh
make
make check
make run ARGS=5
make debug
make clean
```

Open the folder in VS Code through WSL (`code .`), install Microsoft's C/C++ extension **in WSL**, then press F5. Set a breakpoint at the `for (const Node *current = head; ...)` line and expand `head`, `head->next`, `head->next->next` in VARIABLES or WATCH.

## Native Windows (MSYS2 UCRT64)

In a **UCRT64** shell (not WSL):

```sh
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb make
make
make check
make debug
```

If the MSYS2 update asks you to restart the terminal, do so first.

Separate build outputs avoid mixing executables:
- WSL: `build/linux/spo-lab`
- Windows: `build/windows/spo-lab.exe`

The included VS Code `launch.json` targets **WSL only**. For native Windows GUI debugging, create a separate local Windows workspace/debug configuration using UCRT64's `gdb.exe`, or use a CLion MinGW toolchain.

## Notes

Project sources should live on the Linux filesystem (`~/projects/...`) when building with WSL; clone a separate worktree into Windows for the native build. Track changes with GitLab.

Once tooling is verified, replace the smoke test with modules for lexer/parser/AST, CFG and code generation.
