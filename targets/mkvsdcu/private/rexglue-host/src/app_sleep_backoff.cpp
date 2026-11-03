#include <rex/ppc/context.h>
#include <rex/ppc/func.h>

#include "short_sleep.h"

// UE3 appSleep(seconds), guest sub_826037D8. The game thread waits for the
// render thread (sub_8236CC50 and similar fences) by polling with
// appSleep(0.0), which reaches the kernel as a zero-length delay, a plain
// yield. While the render thread waited for the GPU, the game thread spun on a
// full core. Real sleeps keep the game's path; a zero sleep still yields for
// short waits, then sleeps 100 us per poll once the wait drags on.
REX_EXTERN(__imp__sub_826037D8);
REX_EXTERN(sub_826037D8) {
  const bool zero_sleep = !(ctx.f1.f64 * 1000.0 >= 1.0);  // whole ms, as the guest truncates
  if (!zero_sleep || short_sleep::CountRetry() <= 16 || !short_sleep::SleepMicroseconds(100)) {
    __imp__sub_826037D8(ctx, base);
  }
}
