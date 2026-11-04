//
// Created by mohammd on 11/4/26.
//

#ifndef SHELL_TERMINAL_SHELL_COMMUNICATION_H
#define SHELL_TERMINAL_SHELL_COMMUNICATION_H

typedef struct {
    FILE *terminal_shell_input;
    char *terminal_shell_output;
    int terminal_input_length;
    pthread_mutex_t terminal_input_lock;     /* protects terminal_input  */
    pthread_mutex_t terminal_output_lock;    /* protects terminal_output */
    bool is_shell_reading;
} terminal_shell_IO;

void terminal_shell_IO_init(terminal_shell_IO *ts_IO) {
    ts_IO->terminal_shell_input = fmemopen(nullptr,sizeof(char) * 1024, "a+");
    ts_IO->terminal_shell_output = calloc(1024,sizeof(char));
    ts_IO->terminal_input_length = 0;
    ts_IO->is_shell_reading = 0;

    pthread_mutex_init(&ts_IO->terminal_input_lock,nullptr);
    pthread_mutex_init(&ts_IO->terminal_output_lock,nullptr);
}
 void shell_writing(terminal_shell_IO *ts_IO) {
    pthread_mutex_lock(&ts_IO->terminal_input_lock);
    pthread_mutex_unlock(&ts_IO->terminal_output_lock);
}

void shell_reading(terminal_shell_IO *ts_IO) {
    pthread_mutex_unlock(&ts_IO->terminal_input_lock);
    pthread_mutex_lock(&ts_IO->terminal_output_lock);
}

#endif //SHELL_TERMINAL_SHELL_COMMUNICATION_H
