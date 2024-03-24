#include "time.h"

const instant_t INSTANT_ZERO = { .instant = 0 };
const duration_t DURATION_ZERO = { .duration = 0 };

/* How many "durations" in a second */
#define TIME_BASE INT64_C(1000000000)

static int int64_sign(int64_t value) {
    /* int is probably much smaller than int64_t, so the obvious subtraction
     * isn't going to work, convert to -1, 0 or +1 which definitely fits in an
     * int
     */
    return (value > 0) - (value < 0);
}

duration_t instant_sub(instant_t minuend, instant_t subtrahend) {
    return (duration_t) { .duration = minuend.instant - subtrahend.instant, };
}

instant_t instant_add(instant_t base, duration_t offset) {
    return (instant_t) { .instant = base.instant + offset.duration, };
}

int instant_cmp(instant_t lhs, instant_t rhs) {
    return int64_sign(lhs.instant - rhs.instant);
}

duration_t duration_minutes(int minutes) {
    return duration_seconds(minutes * 60);
}

duration_t duration_seconds(int seconds) {
    return (duration_t) { .duration = seconds * TIME_BASE, };
}

duration_t duration_millis(int millis) {
    return (duration_t) { .duration = millis * (TIME_BASE / INT64_C(1000)), };
}

duration_t duration_div(duration_t numerator, int denominator) {
    return (duration_t) { .duration = numerator.duration / (int64_t) denominator, };
}

duration_t duration_mul(duration_t multiplier, int multiplicand) {
    return (duration_t) { .duration = multiplier.duration * (int64_t) multiplicand, };
}

duration_t duration_add(duration_t augend, duration_t addend) {
    return (duration_t) { .duration = augend.duration +  addend.duration, };
}

duration_t duration_sub(duration_t minuend, duration_t subtrahend) {
    return (duration_t) { .duration = minuend.duration +  subtrahend.duration, };
}

int duration_cmp(duration_t lhs, duration_t rhs) {
    return int64_sign(lhs.duration - rhs.duration);
}
