/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * KISS driver
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* High nibble of port is which physical port to use, low nibble is which device on the port to use. */

/* Receive one byte */
void kiss_recv_byte(uint8_t serial, uint8_t byte);

/* Transmit one packet.
 *
 * Returns ACKMODE packet id or 0 if ackmode is not enabled for this.
 */
uint16_t kiss_xmit(uint8_t port, uint8_t *buffer, size_t len);

/* Set tx delay in units of 10ms */
void kiss_set_txdelay(uint8_t port, uint8_t delay);

/* Set slottime in units of 10ms */
void kiss_set_slottime(uint8_t port, uint8_t delay);

/* Set duplex */
void kiss_set_duplex(uint8_t port, bool full_duplex);
