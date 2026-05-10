#pragma once

#include "../bits/__arch.hpp"

#if defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "arch/types_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "arch/types_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "arch/types_amd64.hpp"
#else
#error "simd/types.hpp: no SIMD type backend matched. Build for x86_64/aarch64/armv7-a+NEON, or extend with a scalar fallback."
#endif
