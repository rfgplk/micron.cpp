#pragma once

#include "../bits/__arch.hpp"

// the scalar tier wins outright: MICRON_NO_SIMD, the generic arch tier, x86 below SSE2,
// or ARM without NEON. see bits/__arch.hpp
#if defined(__micron_simd_generic)
#include "arch/types_generic.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/types_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/types_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "arch/types_amd64.hpp"
#else
#error "simd/types.hpp: no SIMD type backend matched. Build for x86_64/aarch64/armv7-a+NEON, or extend with a scalar fallback."
#endif
