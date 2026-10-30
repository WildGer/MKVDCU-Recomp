#include <cstdint>
#include <thread>

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

namespace {
void CpuRelax() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
  _mm_pause();
#endif
}
}  // namespace

// ReXGlue mid-asm hook for the command-ring stall path at guest 0x827F6150.
// The first experiment intentionally avoids real sleeps: spin politely for most
// retries and hand the scheduler a chance every 64th stalled iteration.
void mkvsdcu_command_ring_backoff() {
  static thread_local uint32_t stall_count = 0;

  ++stall_count;
  if ((stall_count & 63u) != 0u) {
    CpuRelax();
  } else {
    std::this_thread::yield();
  }
}
