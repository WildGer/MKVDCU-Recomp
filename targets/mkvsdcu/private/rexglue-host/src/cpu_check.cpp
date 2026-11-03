// Built only for MKVSDCU_CPU_LEVEL=x86-64-v3, and compiled for the x86-64
// baseline so it can run on a CPU that lacks the instructions the rest of the
// game uses. It runs before other static initializers and stops with a clear
// message instead of an illegal-instruction crash.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <intrin.h>

namespace {

__attribute__((target("xsave"))) bool CpuSupportsX86_64V3() {
  int regs[4];
  __cpuid(regs, 0);
  if (regs[0] < 7) return false;

  __cpuid(regs, 1);
  const int ecx1 = regs[2];
  const bool fma = ecx1 & (1 << 12), movbe = ecx1 & (1 << 22), osxsave = ecx1 & (1 << 27),
             avx = ecx1 & (1 << 28), f16c = ecx1 & (1 << 29);
  if (!(fma && movbe && osxsave && avx && f16c)) return false;
  // The OS must save the AVX (YMM) state on context switches.
  if ((_xgetbv(0) & 0x6) != 0x6) return false;

  __cpuidex(regs, 7, 0);
  const int ebx7 = regs[1];
  const bool bmi1 = ebx7 & (1 << 3), avx2 = ebx7 & (1 << 5), bmi2 = ebx7 & (1 << 8);

  __cpuid(regs, int(0x80000001));
  const bool lzcnt = regs[2] & (1 << 5);
  return bmi1 && avx2 && bmi2 && lzcnt;
}

struct CpuCheck {
  CpuCheck() {
    if (CpuSupportsX86_64V3()) return;
    MessageBoxW(nullptr,
                L"This build of Mortal Kombat vs. DC Universe needs a CPU with AVX2 "
                L"(Intel Haswell / AMD Zen or newer).\n\nRebuild it with "
                L"-DMKVSDCU_CPU_LEVEL=sse4.1 for older processors.",
                L"Mortal Kombat vs. DC Universe", MB_OK | MB_ICONERROR);
    ExitProcess(1);
  }
};

__attribute__((init_priority(101))) CpuCheck cpu_check;

}  // namespace
