#pragma once

#include "../bits/__arch.hpp"

#if defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/bits_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/bits_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "arch/bits_amd64.hpp"
#else
#error                                                                                                                                     \
    "simd/bits.hpp: no SIMD backend matched. Build for x86_64/i386+SSE2, aarch64+NEON, or armv7-a+NEON (-msse2 / -mfpu=neon / -march=native)."
#endif
