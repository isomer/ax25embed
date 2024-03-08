#include <stddef.h>

typedef enum metric_t {
	/* Packet was larger than receive buffer and was dropped */
	METRIC_OVERRUN,
    /* Packet was too short */
    METRIC_UNDERRUN,
	/* Packet countained an invalid escape sequence. */
	METRIC_BAD_ESCAPE,
	/* Received an unknown/unexpected kiss command byte from the TNC */
	METRIC_UNKNOWN_KISS_COMMAND,
	/* Not a real metric, just the last metric number, insert new metrics before here */
	MAX_METRIC,
} metric_t;

/** Increment a metric, used for counters (eg packet counts) */
void metric_inc(metric_t metric);

/** Increment a metric by an ammount, used for things like byte counts */
void metric_inc_by(metric_t metric, size_t count);
