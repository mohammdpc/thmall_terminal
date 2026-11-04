#pragma once

#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "keysMapping.h"
#include "terminal_shell_communication.h"

#ifndef SHELL_LINUX_UI_H
#define SHELL_LINUX_UI_H

#define lines_buffer_length 10
typedef struct {
    int display_num;
    int display_width;
    int display_height;
    int window_width;
    int window_height;
    int window_padding;
    int line_count;
    int line_char_count;
    int x_offset;
    int y_offset;
    int line_height;
    int shell_input_read_length;
    Display *display;
    XColor background;
    Window root;
    Window window;
    GC gc;
    Font font;
    XFontStruct *font_struct;
    terminal_shell_IO *terminal_shell_communication;
    FILE *content_buffer;

    char *lines_buffer[lines_buffer_length];
    char *line_buffer;
    bool shell_writing;
} LinuxTerminal;

void special_Key_action(LinuxTerminal *terminal,const char *keyBuffer, const KeySym *keySym);
//===============================
// utills
//===============================

static bool is_Special(const KeySym *keySym) {
    const char *key = XKeysymToString(*keySym);
    for (int i = 0; i < 59; i++) {
        if (strcmp(key, special_Keys_Names[i]) == 0)
            return true;
    }
    return false;
}

static int get_Special_Key_Index(const KeySym *keySym) {
    const char *key = XKeysymToString(*keySym);
    for (int i = 0; i < 60; i++) {
        if (strcmp(key, special_Keys_Names[i]) == 0)
            return i;
    }
    return -1;
}
//===============================
//
//===============================
int init(LinuxTerminal *terminal, terminal_shell_IO *terminal_shell_communication) {
    terminal->terminal_shell_communication = terminal_shell_communication;
    terminal->content_buffer = fmemopen(nullptr,sizeof(char) * 1024, "a+");
    terminal->line_buffer = malloc(sizeof(char) * 512);
    terminal->line_buffer[0] = '\0';
    for (int i = 0; i < lines_buffer_length; i++) {
        terminal->lines_buffer[i] = malloc(sizeof(char) * 512);
        terminal->lines_buffer[i][0] = '\0';
    }
    terminal->display = XOpenDisplay(nullptr);
    if (terminal->display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int m;
    char **fonts = XListFonts(terminal->display, "*mono*medium-r-*", 1000, &m);
    terminal->font_struct = XLoadQueryFont(terminal->display, fonts[0]);
    fprintf(stderr, "fonts found: %d\n", m);
    for (int i = 0; i < m; i++) {
        fprintf(stderr, "%s\n", fonts[i]);
    }

    terminal->display_num    = DefaultScreen(terminal->display);
    terminal->display_width  = XDisplayWidth(terminal->display, terminal->display_num);
    terminal->display_height = XDisplayHeight(terminal->display, terminal->display_num);
    terminal->window_height  = terminal->display_height / 2;
    terminal->window_width   = terminal->display_width / 2;
    terminal->window_padding = 30;
    terminal->root           = XDefaultRootWindow(terminal->display);
    terminal->window         = XCreateSimpleWindow(terminal->display, terminal->root, 10, 10, terminal->window_width,
                                           terminal->window_height, 2,
                                           BlackPixel(terminal->display, terminal->display_num),
                                           BlackPixel(terminal->display, terminal->display_num));
    terminal->gc = DefaultGC(terminal->display, terminal->display_num);
    terminal->line_count = 0;
    terminal->line_char_count = 0;
    terminal->x_offset = terminal->window_padding;
    terminal->y_offset = terminal->window_padding;
    terminal->line_height = 20;
    terminal->shell_input_read_length = 0;
    terminal->shell_writing = true;

    XSelectInput(terminal->display, terminal->window, ExposureMask | KeyPressMask | KeyReleaseMask);
    return 0;
}

int open_Window(const LinuxTerminal *terminal) {
    XSetWindowBackground(terminal->display, terminal->window,BlackPixel(terminal->display, terminal->display_num));
    XSetForeground(terminal->display, terminal->gc,WhitePixel(terminal->display, terminal->display_num));
    XMapWindow(terminal->display, terminal->window);
    return 0;
}
//=================================
//events
//=================================
int keyPressedLoop(LinuxTerminal *terminal,XEvent *event ,char *keyBuffer,KeySym *keysym) {
    *keysym = XLookupKeysym(&event->xkey, 0);
    const int n = XLookupString(&event->xkey,keyBuffer,sizeof(keyBuffer),keysym,nullptr);
    keyBuffer[n] = '\0';
    return n;
}

int drawKeyBuffer(LinuxTerminal *terminal, const char *keyBuffer) {
    const int char_width = XTextWidth(terminal->font_struct,keyBuffer,(int)strlen(keyBuffer));
    if (keyBuffer[0] == '\r' || terminal->x_offset >= (terminal->window_width - terminal->window_padding)) {
        terminal->line_char_count = 0;
        terminal->y_offset+=terminal->line_height;
        terminal->x_offset = terminal->window_padding;
    }
    XTextItem text_item;
    text_item.font = terminal->font_struct->fid;
    text_item.chars = keyBuffer;
    text_item.nchars = (int)strlen(keyBuffer);
    text_item.delta = 1;
    XDrawText(terminal->display, terminal->window,terminal->gc,
                    terminal->x_offset, terminal->y_offset, &text_item, 1);
    terminal->x_offset += char_width;

}

int record_key(LinuxTerminal *terminal, const char *keyBuffer,const KeySym *keySym) {

    if (is_Special(keySym)) {
        special_Key_action(terminal,keyBuffer,keySym);
    } else if (keyBuffer != nullptr && keyBuffer[0] != '\0'){
        terminal->line_char_count++;
        strcat(terminal->line_buffer, keyBuffer);
    }

}
int redraw_terminal(LinuxTerminal *terminal, const XEvent *event) {
    terminal->window_width = event->xexpose.width;
    terminal->window_height = event->xexpose.height;
    terminal->x_offset = terminal->window_padding;
    terminal->y_offset = terminal->window_padding;
    rewind(terminal->content_buffer);
    char c[2];
    c[1] = '\0';
    while ((c[0] = (char)fgetc(terminal->content_buffer)) != EOF) {
        drawKeyBuffer(terminal,c);
    }
    if (terminal->line_buffer != "\0") {
        const int line_buffer_length = (int)strlen(terminal->line_buffer);
        for (int i = 0; i < line_buffer_length; i++) {
            c[0] = (char)terminal->line_buffer[i];
            drawKeyBuffer(terminal,c);
        }
    }
    return 0;
}

void draw_shell_output(LinuxTerminal *terminal) {
    const int shell_draw_length = terminal->terminal_shell_communication->terminal_input_length - terminal->shell_input_read_length;
    if ( shell_draw_length > 0) {
        fseek(terminal->terminal_shell_communication->terminal_shell_input, -shell_draw_length,SEEK_END);
        for (int i = 0; i < shell_draw_length; i++) {
            const char temp [2]= {(char)fgetc(terminal->terminal_shell_communication->terminal_shell_input),'\0'};
            fputc(temp[0],terminal->content_buffer);
            drawKeyBuffer(terminal,temp);
            terminal->shell_input_read_length ++;
        }
        fflush(terminal->terminal_shell_communication->terminal_shell_input);
    }
}

// main loop
int terminal_loop(LinuxTerminal *terminal) {
    XEvent event;
    KeySym keysym;
    char buffer[20] = {0};
    int n;
    while (1) {
        draw_shell_output(terminal);
        XNextEvent(terminal->display, &event);
        if (terminal->terminal_shell_communication->is_shell_reading) {
            switch (event.type) {
                case KeyPress: {
                    n = keyPressedLoop(terminal,&event,buffer,&keysym);
                    if (n > 0) {
                        record_key(terminal,buffer,&keysym);
                        drawKeyBuffer(terminal,buffer);
                    }
                    break;
                }
                case Expose: {
                    if (terminal->content_buffer != nullptr)
                        redraw_terminal(terminal,&event);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    return 0;
}

// special keys actions

int delete_char(LinuxTerminal *terminal) {
    const int line_buffer_str_len = (int)strlen(terminal->line_buffer);
    if (line_buffer_str_len == 0) {
        return 0;
    }
    const char deleted_char[2] = {terminal->line_buffer[line_buffer_str_len - 1],'\0'};
    const int char_width = XTextWidth(terminal->font_struct,deleted_char,1);
    if (terminal->x_offset - char_width < terminal->window_padding) {
        terminal->y_offset -= terminal->line_height;
        terminal->x_offset = terminal->window_width - terminal->window_padding;
    }
    terminal->line_buffer[line_buffer_str_len - 1] = '\0';
    terminal->x_offset -= char_width;
    XClearArea(terminal->display,terminal->window,terminal->x_offset,terminal->y_offset - terminal->line_height,char_width,terminal->line_height,0);
    return 0;
}

int newLine(LinuxTerminal *terminal,const char *keyBuffer) {
    strcat(terminal->line_buffer, keyBuffer);
    terminal->line_count++;
    for (int i = 0; i < lines_buffer_length - 1; i++) {
        strcpy(terminal->lines_buffer[i] , terminal->lines_buffer[i+1]);
    }
    strcpy(terminal->lines_buffer[lines_buffer_length - 1] , terminal->line_buffer);
    fputs(terminal->line_buffer,terminal->content_buffer);
    terminal->line_buffer[0] = '\0';
    return 0;
}

void special_Key_action(LinuxTerminal *terminal,const char *keyBuffer,const KeySym *keySym) {
    const int keyIndex = get_Special_Key_Index(keySym);
    switch (keyIndex) {
        case backspace:{
            delete_char(terminal);
            break;
        }
        case enter: {
            terminal->terminal_shell_communication->terminal_shell_output = strdup(terminal->line_buffer);
            terminal->terminal_shell_communication->is_shell_reading = false;
            newLine(terminal,keyBuffer);
            break;
        }
        default:
            break;
    }
}

#endif //SHELL_LINUX_UI_H