/* Plaform using the generic C apis */
#include "platform.h"
#include "kiss.h"
#include "serial.h"
#include "time.h"
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

void debug_internal_x8(struct debug_t *v) {
    printf("%x", v->u8);
}

void debug_internal_d8(struct debug_t *v) {
    printf("%d", v->u8);
}

void debug_internal_str(struct debug_t *v) {
    printf("%s", (const char *)v->ptr);
}

void debug_internal_eol(void) {
    putchar('\n');
    fflush(stdout);
}
