#pragma once

#if defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "neon_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "neon_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "simd128.hpp"
#include "simd256.hpp"
#include "simd512.hpp"
#endif
