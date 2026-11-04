# Thmall Shell
 
A terminal emulator window and a small shell, both written from scratch in C on Linux.
 
This is a **learning project**. Nothing here is production software. The goal is to understand how things work by building them, making mistakes, and fixing them.
 
## Why this project exists
 
- Learn software engineering: structure, boundaries, coupling and cohesion.
- Learn how UIs work, and how large UI frameworks (Qt, GTK, Flutter) work under the hood.
- Learn memory management.
- Learn multi-threading.
- Learn system calls.
- Learn design patterns.
## What it is
 
The project has two cooperating parts:
 
- **Terminal (UI):** opens a window, receives keyboard input and shows text.
- **Shell:** parses a command line, runs built-in commands (`cd`, `help`, `exit`) and launches other programs with `fork` + `execvp`. The shell is based on Stephen Brennan's "Write a Shell in C" tutorial.
The two parts run as separate threads inside one process and talk through a shared data structure.
 
## Status
 
The first version was built with a deliberately naive approach in about 2 days. It works as a prototype but has known architectural problems (shared mutable state, implementation code in headers, incorrect synchronization, unclear memory ownership, no shutdown design).
 
The full list is documented in the README of the `thmall-shell/naive-approach` branch. The `main` branch is where the cleaned-up design is built.
 
## Branches
 
| Branch | Purpose |
|--------|---------|
| `main` | Current, evolving version of the whole project |
| `thmall-shell/naive-approach` | Frozen snapshot of the first naive version, kept as a reference for what went wrong and why |
 
## Target architecture
 
```
main.c        wiring and lifecycle only
shell_core.*  parsing, builtins, launching (no threads, no UI)
channel.*     thread-safe byte channel (pipe / socketpair / pty)
ui/           window, input and drawing; only knows about the channel
keys.*        key table defined in the .c file, prefixed enum
```
 
Design rules for the rewrite:
 
- Headers contain declarations only. Definitions live in `.c` files.
- Each header includes what it uses.
- The shell core has no knowledge of threads or the UI.
- The UI has no knowledge of how the shell works. It only reads and writes bytes.
- Every allocation has exactly one owner.
- Threads synchronize with condition variables, semaphores or blocking I/O, never with plain flags or busy loops.
- There is an explicit shutdown path that both sides can trigger and observe.
## Roadmap
 
1. Build hygiene: split headers and sources, add a build file, enable `-Wall -Wextra`.
2. Memory: fix leaks and ownership, verify with `valgrind` or `-fsanitize=address`.
3. Threading: replace the flag and misused mutexes, verify with `-fsanitize=thread`.
4. System calls: replace the in-memory stream with `pipe()`/`socketpair()`, then move to `openpty()`/`forkpty()` so interactive programs like `vim` can work.
5. Design patterns: event loop, producer/consumer queue, command pattern for builtins, callbacks or observer for UI updates.
6. UI internals: study and build small versions of what UI frameworks are generally built around, such as an event loop, an input event pipeline, a widget/render tree, a layout pass and a draw pass.
## Building
 
Requirements: a C23-capable compiler (for `nullptr`, use `-std=gnu23`) and pthreads.
 
```
gcc -std=gnu23 -Wall -Wextra -g main.c -o shell -lpthread
```
 
- `gcc`: the compiler.
- `-std=gnu23`: C23 mode.
- `-Wall -Wextra`: enable most warnings.
- `-g`: include debug info.
- `-lpthread`: link the pthread library.
- Replace `main.c` with all of your source files.
TODO: list the libraries the UI needs (whatever `linux_ui.h` depends on) and the exact build command.
 
## Debugging tools
 
- Data races: add `-fsanitize=thread`
- Memory errors and leaks: add `-fsanitize=address`, or run under `valgrind`
- See `man gcc` (search for `fsanitize`) for details.
## Reading list
 
- Stephen Brennan, "Write a Shell in C" https://brennan.io/2015/01/16/write-a-shell-in-c/ .
- `man 7 pty`, `man 3 openpty`, `man 2 fork`, `man 2 pipe`, `man 3 pthread_cond_wait`.
- Gamma et al., *Design Patterns*.
- Butenhof, *Programming with POSIX Threads*.
- Stevens and Rago, *Advanced Programming in the UNIX Environment*.
## Credits
 
The shell core follows Stephen Brennan's "Write a Shell in C" tutorial.
