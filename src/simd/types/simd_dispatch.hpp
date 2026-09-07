#pragma once

// The class-based lane API (simd::v128 and friends) is intrinsic-typed all the way down and has
// no scalar analogue: every one of its operations takes and returns a lane container BY VALUE,
// which is exactly the ABI error the generic tier exists to avoid. It is absent here rather than
// emulated. Nothing in the container/string/math path uses it -- the free functions in
// simd/{memory,bitwise}.hpp are what those call.
#if defined(__micron_simd_generic)
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "neon_arm64.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm32)
#include "neon_arm32.hpp"
#elif defined(__micron_arch_x86) || defined(__micron_arch_amd64)
#include "simd128.hpp"
#include "simd256.hpp"
#include "simd512.hpp"
#endif
