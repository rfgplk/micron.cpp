#pragma once

#if defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/memory_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/memory_arm32.hpp"
#elif (defined(__micron_arch_x86) || defined(__micron_arch_amd64)) && defined(__micron_x86_sse2)
#include "arch/memory_amd64.hpp"
#else
#error "micron SIMD memory backend requires SSE2 (x86) or NEON (arm): build with -msse2 / -mfpu=neon (or -march=native)"
#endif
