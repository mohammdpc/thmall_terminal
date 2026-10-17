#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "linux_ui.h"

// function declaration

static void lsh_loop();

static int lsh_cd(char **args);

static int lsh_help(char **args);

static int lsh_exit(char **args);

// variables
char current_dir[PATH_MAX];
// built-in commands
static char *builtin_str[] = {
    "cd",
    "help",
    "exit",
  };

static int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit,
  };

static int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int main(int argc, char* argv[]) {
    // Load config files if any
    getcwd(current_dir,sizeof(current_dir));
    // initializing UI

    //init();

    // Run command loop.

    lsh_loop();

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}

#define LSH_RL_BUFSIZE 1024
static char* lsh_read_line() {
    int bufferSize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufferSize);
    int c;
    if (!buffer) {
        printf("lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }

        position++;

        if (position >= bufferSize) {
            bufferSize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer,bufferSize);
            if (!buffer) {
                free(&buffer);
                printf("lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

static char **lsh_split_line(char *line)
{
    int buffer_size = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*)); // NOLINT(*-suspicious-realloc-usage)
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(nullptr, LSH_TOK_DELIM);
    }
    tokens[position] = nullptr;
    return tokens;
}

static int lsh_launch(char **args)
{
    pid_t wpid;
    int status;

    const pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else{
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

static int lsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
        getcwd(current_dir,sizeof(current_dir));
    }
    return 1;
}

static int lsh_help(char **args)
{
    int i;
    printf("Stephen Brennan's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;
}

static int lsh_execute(char **args)
{
    int i;

    if (args[0] == NULL || args[0][0] == '^C') {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

static  void lsh_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("Thmall_Shell:%s>",current_dir);
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}
