#include "serial.h"

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    serial_init();
    serial_wait();
}
