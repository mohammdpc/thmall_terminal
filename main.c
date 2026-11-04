#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "linux_ui.h"
#include "thmall_shell.h"


// function declaration

terminal_shell_IO ts_IO;

void *thmall_shell(void *arg) {
    lsh_loop((terminal_shell_IO *)arg);
}

void *thmall_terminal(void *arg) {
    LinuxTerminal terminal;
    init(&terminal,(terminal_shell_IO *)arg);
    open_Window(&terminal);
    terminal_loop(&terminal);
}
int main(int argc, char* argv[]) {

    terminal_shell_IO_init(&ts_IO);
    // Load config files if any
    getcwd(current_dir,sizeof(current_dir));

    pthread_t terminal_thread, shell_thread;

    if (&ts_IO != nullptr) {
        pthread_create(&terminal_thread, nullptr, &thmall_terminal,&ts_IO);
        pthread_create(&shell_thread, nullptr, &lsh_loop, &ts_IO);

        pthread_join(terminal_thread,nullptr);
        pthread_join(shell_thread,nullptr);
    }

    // Run command loop.

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}

