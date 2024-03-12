#include "platform.h"
#include "kiss.h"
#include "serial.h"
#include <stdlib.h>
#include <stdio.h>

void debug_putch(char ch) {
    putchar(ch);
}

void debug(const char *msg) {
    puts(msg);
}

void panic(const char *msg) {
    debug(msg);
    abort();
}

