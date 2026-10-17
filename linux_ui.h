#include "stdio.h"
#include <X11/Xlib.h>

#ifndef SHELL_LINUX_UI_H
#define SHELL_LINUX_UI_H

Display* display;
int display_num;
int display_width;
int display_height;
int window_width;
int window_height;
XColor background;
Window root;
Window window;
GC gc;

int init() {
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    display_num = DefaultScreen(display);
    display_width = XDisplayWidth(display,display_num);
    display_height = XDisplayHeight(display,display_num);
    window_height = display_height / 2;
    window_width = display_width / 2;
    root = XDefaultRootWindow(display);
    window = XCreateSimpleWindow(display,root,10,10,window_width, window_height,2,BlackPixel(display,display_num),BlackPixel(display,display_num));
    gc = DefaultGC(display,display_num);

    XEvent event;
    XSetWindowBackground(display,window,BlackPixel(display,display_num));
    XSetForeground(display,gc,WhitePixel(display,display_num));
    XMapWindow(display,window);
    XSelectInput(display,window, ExposureMask | KeyPressMask |  KeyReleaseMask);
    XTextItem text;
    int fc =50;
    const Font font = XLoadFont(display,XListFonts(display,"*",1,&fc)[2]);
    text.chars = "a";
    text.nchars = 1;
    text.font = font;
    XDrawText(display, window,DefaultGC(display,display_num),0,0,&text,2);
    XDrawString(display, window,DefaultGC(display,display_num),0,0,"hi",2);

    while (1) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            XDrawString(display, window, gc, 20, 40, "me", 2);
        } else if (event.type == KeyPress) {
            XDrawString(display, window,DefaultGC(display,display_num),30,40,"a",1);
        }
    }
    return  0;
}

int openWindow(int width, int height ) {

}
#endif //SHELL_LINUX_UI_H
