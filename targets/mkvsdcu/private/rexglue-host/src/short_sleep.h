#pragma once

#include <chrono>
#include <cstdint>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

// Helpers for guest polling loops that would otherwise spin on a full core.
namespace short_sleep {

inline void CpuRelax() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
  _mm_pause();
#endif
}

// Sleeps about @p us microseconds on a per-thread high-resolution timer;
// ::Sleep cannot go below a millisecond. Returns false when unavailable.
inline bool SleepMicroseconds(int64_t us) {
  thread_local struct Timer {
    HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                           TIMER_ALL_ACCESS);
    ~Timer() {
      if (handle) CloseHandle(handle);
    }
  } timer;
  if (!timer.handle) return false;
  LARGE_INTEGER due;
  due.QuadPart = -us * 10;  // Relative, 100 ns units.
  if (!SetWaitableTimerEx(timer.handle, &due, 0, nullptr, nullptr, nullptr, 0)) return false;
  return WaitForSingleObject(timer.handle, INFINITE) == WAIT_OBJECT_0;
}

// Counts the consecutive retries of one wait on the calling thread. Retries
// of one wait follow each other closely; a gap of over 1 ms starts a new one.
inline uint32_t CountRetry() {
  using Clock = std::chrono::steady_clock;
  static thread_local uint32_t retries = 0;
  static thread_local Clock::time_point last_retry;
  const auto now = Clock::now();
  if (now - last_retry > std::chrono::milliseconds(1)) retries = 0;
  last_retry = now;
  return ++retries;
}

}  // namespace short_sleep
