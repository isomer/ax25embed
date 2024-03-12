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
    /* Invalid address */
    METRIC_INVALID_ADDR,
    /* Number of packets received that are not for me. */
    METRIC_NOT_ME,
    /* Number of bytes received that are not for me. */
    METRIC_NOT_ME_BYTES,
    /* Number of packets where we refused to digipeat */
    METRIC_REFUSED_DIGIPEAT,
    /* Unknown frame type */
    METRIC_UNKNOWN_FRAME,
    /* Count of test response frames received */
    METRIC_TEST_RESPONSE_RECV,
    /* Count of test command frames received */
    METRIC_TEST_COMMAND_RECV,
    /* Count of test command frames sent */
    METRIC_TEST_COMMAND_SENT,
    /* Frames with R bits set to 00 or 11, used by former versions of the standard */
    METRIC_LEGACY_TYPE_RECV,
    /* We tried to send a packet, but there were no free packet buffers available for use */
    METRIC_NO_PACKETS,
    /* Packets allocated */
    METRIC_PACKETS_ALLOCATED,
    /* Packets freed */
    METRIC_PACKETS_FREED,
    /* Number of packets of a type that should be a command, but were instead a response */
    METRIC_NOT_COMMAND,
    /* Number of received SABM packets that were successfully set up */
    METRIC_SABM_SUCCESS,
    /* Number of received SABM packets that were unsuccessfully set up */
    METRIC_SABM_FAIL,
    /* No free connection slots */
    METRIC_NO_CONNS,
    /* Number of kiss data frames sent */
    METRIC_KISS_XMIT,
    /* Number of kiss data bytes sent */
    METRIC_KISS_XMIT_BYTES,
	/* Not a real metric, just the last metric number, insert new metrics before here */
	MAX_METRIC,
} metric_t;

/** Increment a metric, used for counters (eg packet counts) */
void metric_inc(metric_t metric);

/** Increment a metric by an ammount, used for things like byte counts */
void metric_inc_by(metric_t metric, size_t count);
