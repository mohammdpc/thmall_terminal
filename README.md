# Thmall Shell: Naive Approach

> **This branch is a frozen snapshot.** It is kept as a reference for what the first, naive version looked like and what is wrong with it. Do not build on it. Active work happens on `main`.

A terminal emulator window plus a small shell (based on Stephen Brennan's "Write a Shell in C"), written in C on Linux. Built in about 2 days with no up-front design, as a learning exercise.

## Goals of the project

- Learn software engineering (structure, boundaries, coupling).
- Learn how UIs work, and how frameworks like Qt, GTK and Flutter work under the hood.
- Learn memory management, multi-threading, system calls and design patterns.
## Structure in this snapshot

| File | Role |
|------|------|
| `main.c` | Creates the shared IO struct and starts two threads (UI and shell) |
| `thmall_shell.h` | Line splitting, builtins (`cd`, `help`, `exit`), `fork`/`exec`, and the shell thread loop |
| `terminal_shell_communication.h` | `terminal_shell_IO` struct shared by both threads, plus init and lock helpers |
| `keysmapping.h` | Enum of special keys and a parallel array of key names |
| `linux_ui.h` | UI code (**not reviewed**, so nothing below judges it) |

The UI thread and shell thread communicate through one shared struct holding an in-memory `FILE*` (from `fmemopen`), a `char*` buffer, a length counter, two mutexes and a `bool` flag.

## Architectural problems

### 1. Implementation code lives in headers

- Function bodies and global data are defined in `.h` files.
- `special_Keys_Names` is a non-static, non-const global in a header. Including it from two `.c` files causes a "multiple definition" linker error.
- `terminal_shell_IO_init`, `shell_writing`, `shell_reading` and `tprint` are non-static, non-inline definitions in headers, with the same problem.
- `static char current_dir[PATH_MAX]` in a header gives every `.c` file a private copy. It only works because everything ends up in one translation unit.
- **Fix:** declarations in `.h`, definitions in `.c`. Use `extern` or `static const char *const` for tables.
### 2. Include guards and hidden dependencies

- In `thmall_shell.h` the guard's `#define` is at the bottom instead of right after `#ifndef`.
- Headers do not include what they use (`PATH_MAX`, `malloc`, `printf`, `pid_t`, `pthread_mutex_t`, `bool`, `terminal_shell_IO`).
- The code compiles only because of include order in `main.c`. That is invisible coupling.
- **Fix:** each header includes everything it needs.
### 3. Tight coupling between shell and UI

- Both threads read and write the fields of one struct directly.
- Shell core logic (tokenizer, builtins, launcher) shares a file with the thread loop and the UI adapter (`tprint`, `lsh_loop`). The parser cannot be tested or reused without the UI.
- `main.c` initializes shell internals (`getcwd(current_dir, ...)`), and `current_dir` is a process-wide global.
- Field names are ambiguous: `terminal_shell_input` is the stream the shell writes to.
- **Fix:** layers.
    - Shell core: a `shell_ctx` (cwd, input/output fds) and `shell_execute(ctx, line)`. No threads, no UI.
    - Transport: pipe, socketpair or pty.
    - UI: only reads and writes bytes through the transport.
### 4. Output goes through two different paths

- The prompt goes into the in-memory stream through `tprint`.
- `help` uses `printf`, and child programs (`ls`, etc.) write to the real process `stdout`.
- So command output does not reach the terminal window. It goes to whatever terminal launched the program.
- Interactive programs (`vim`, `top`) need a real tty and will not get one.
- **Fix:** use a pty (`openpty` / `forkpty` from `<pty.h>`) with the UI owning the master side. Or, keeping a custom shell, `dup2` the child's stdin/stdout/stderr onto your channel before `execvp`.
### 5. The communication channel

- `fmemopen(nullptr, 1024, "a+")` is a fixed 1024-byte stream. When it fills, writes fail and output is lost, while `terminal_input_length` keeps counting.
- A `FILE` has one position shared by reads and writes. If the UI reads from the same stream, the two sides can interfere (not verified, since the UI code was not reviewed).
- `terminal_input_length` is modified without a lock.
- **Fix:** a pipe, or a ring buffer/queue with its own lock and no tiny fixed size.
### 6. Synchronization is incorrect

- `shell_writing` / `shell_reading` lock one mutex and unlock a different one, treating mutexes like semaphores. A mutex must be unlocked by the thread that locked it, otherwise behavior is undefined for default mutexes.
- These functions are not even called by the shell loop.
- The real synchronization is `is_shell_reading`, a plain `bool`. It is not atomic, so this is a data race, and the compiler may read it once and loop forever.
- `lsh_loop` busy-waits on that flag and uses a full CPU core while idle.
- **Fix:** mutex plus `pthread_cond_t`, a semaphore, or a blocking `read()`/`poll()` on a pipe. Use `atomic_bool` for simple flags.
### 7. Memory ownership is unclear and buggy

- `shell_IO->terminal_shell_output = "";` points the field at a string literal and leaks the original `calloc` buffer. Writing to or freeing it later is undefined behavior.
- `strtok` modifies the buffer in place, but that buffer belongs to the UI thread and is read without a lock.
- `terminal_input` is copied once before the loop, so the `!= nullptr` check uses a stale copy and is always true.
- `thmall_shell_title` is allocated and never freed.
- Tokens from `lsh_split_line` point into a buffer the shell does not own.
- `status` is uninitialized if the `if` body is never entered.
- The commented-out `free(...)` shows that "who frees this?" was never decided.
- **Fix:** one owner per allocation, and copy data across the thread boundary instead of sharing pointers.
### 8. No lifecycle or shutdown design

- Closing the window ends the UI thread, but the shell thread keeps spinning. `main` joins the UI thread first and then the shell thread, so the program hangs.
- Running `exit` ends the shell thread, but the UI is never told.
- Results of `pthread_create`, `fmemopen` and `calloc` are not checked.
- `if (&ts_IO != nullptr)` is always true.
- **Fix:** an explicit shutdown signal both sides can set and check, or EOF on a pipe.
### 9. `fork()` in a multithreaded process, and `chdir`

- After `fork` in a multithreaded program, the child has only the calling thread. Only async-signal-safe functions are safe before `exec`. `perror` is not on that list, although it usually works.
- `chdir` changes the working directory of the whole process, including the UI thread. This is inherent to running the shell as a thread. A separate shell process on a pty avoids it.
### 10. Code quality issues that hide real bugs

- `fprintf(,"Stephen Brennan's LSH\n")` is a syntax error.
- Thread functions have no `return`. `thmall_shell` is unused because `main` calls `lsh_loop` directly.
- `lsh_read_line` is dead code now.
- `wpid` is unused, and `lsh_exit` is declared `static` but defined without it.
- `NULL` and `nullptr` are mixed. `nullptr` needs C23 (`-std=gnu23`).
- `lsh_cd` calls `getcwd` even when `chdir` failed.
### 11. `keysmapping.h`

- The enum and name array are parallel lists kept in sync by hand. One missing entry shifts every name after it. Use designated initializers, e.g. `[KEY_ALT_L] = "Alt_L"`.
- Names like `end`, `next`, `home`, `insert`, `escape`, `enter`, `tab` are global identifiers in C and can collide. Prefix them (e.g. `KEY_END`).
- The names are X11 keysym names. Fedora KDE Plasma defaults to Wayland, so check how the UI actually receives keys (not verified here).
## What this snapshot taught

- Getting something working fast is useful, but without boundaries every change touches everything.
- Shared mutable state between threads needs a clear owner and real synchronization, not flags.
- Headers are interfaces, not places to put code.
- A terminal emulator really wants a pty, not an in-memory stream.
## Finding the bugs yourself

Build with ThreadSanitizer to see the data races:

```
gcc -std=gnu23 -Wall -Wextra -g -fsanitize=thread main.c -o shell -lpthread
```

- `gcc`: the compiler.
- `-std=gnu23`: C23 mode (for `nullptr`).
- `-Wall -Wextra`: enable most warnings.
- `-g`: debug info for readable reports.
- `-fsanitize=thread`: runtime data race detector.
- `-lpthread`: link the pthread library.
- Replace `main.c` with all your source files and any libraries `linux_ui.h` needs.
- Documented in `man gcc` (search for `fsanitize`).
## Where to go next

See the `main` branch for the redesigned version and the roadmap.