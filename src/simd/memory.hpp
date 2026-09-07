#pragma once

#include "../bits/__arch.hpp"

// the scalar tier wins outright: MICRON_NO_SIMD, the generic arch tier, x86 below SSE2,
// or ARM without NEON. see bits/__arch.hpp
#if defined(__micron_simd_generic)
#include "arch/memory_generic.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/memory_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/memory_arm32.hpp"
#elif (defined(__micron_arch_x86) || defined(__micron_arch_amd64)) && defined(__micron_x86_sse2)
#include "arch/memory_amd64.hpp"
#else
#error "micron SIMD memory backend requires SSE2 (x86) or NEON (arm): build with -msse2 / -mfpu=neon (or -march=native)"
#endif
