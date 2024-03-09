#include <stdint.h>

void serial_init(void);
void serial_putch(uint8_t serial, uint8_t data);
void serial_wait(void);
