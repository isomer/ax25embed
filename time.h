#ifndef TIME_H
#define TIME_H
#include <stdint.h>

/* We have two representations of time.
 *
 * instant is an instant in time, with an unknown resolution and unknown epoch
 *
 * duration is a duration between two instants, with an unknown resolution.
 *
 * Both are opaque with helpers to allow for arbitrary implementations.
 */
struct duration_t {
    int64_t duration;
};

struct instant_t {
    int64_t instant;
};

typedef struct duration_t duration_t;
extern const duration_t DURATION_ZERO;

duration_t duration_minutes(int minutes);
duration_t duration_seconds(int seconds);
duration_t duration_millis(int millis);
duration_t duration_div(duration_t numerator, int denominator);
duration_t duration_mul(duration_t multiplier, int multiplicand);
duration_t duration_add(duration_t augend, duration_t addend);
duration_t duration_sub(duration_t minuend, duration_t subtrahend);
int duration_cmp(duration_t lhs, duration_t rhs);

/* instant's represent an instant in time.  Their rate and epoch is unknown,
 * and generally are only interacted with via durations and instant_now() */
typedef struct instant_t instant_t;

// used as a flag value for "unset"
extern const instant_t INSTANT_ZERO;

instant_t instant_now(void);

duration_t instant_sub(instant_t minuend, instant_t subtrahend);
instant_t instant_add(instant_t base, duration_t offset);
/* returns <0 0 or >0 depending on if lhs is < rhs, lhs == rhs, lhs > rhs */
int instant_cmp(instant_t lhs, instant_t rhs);


#endif
