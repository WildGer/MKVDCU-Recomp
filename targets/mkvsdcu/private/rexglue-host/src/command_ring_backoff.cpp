#include <thread>

#include "short_sleep.h"

// ReXGlue mid-asm hook for the command-ring stall path at guest 0x827F6150,
// called on every retry while the render thread waits for ring space. The
// guest's own wait (sub_827E1410) only spins, and the ring frees up when the
// GPU thread gets past its vblank wait, which advances in 1 ms steps, so a
// stall typically lasts most of a frame. Spin briefly for short stalls, then
// sleep in 200 us steps instead of burning a core.
void mkvsdcu_command_ring_backoff() {
  if (short_sleep::CountRetry() <= 64) {
    short_sleep::CpuRelax();
  } else if (!short_sleep::SleepMicroseconds(200)) {
    std::this_thread::yield();
  }
}
