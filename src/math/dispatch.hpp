//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../types.hpp"

namespace micron
{
namespace math
{
namespace arch
{

// pushing through from the preprocessor to constexpr symbols

#if defined(__micron_x86_sse)
inline constexpr bool has_sse = true;
#else
inline constexpr bool has_sse = false;
#endif

#if defined(__micron_x86_sse2)
inline constexpr bool has_sse2 = true;
#else
inline constexpr bool has_sse2 = false;
#endif

#if defined(__micron_x86_sse4_1)
inline constexpr bool has_sse4_1 = true;
#else
inline constexpr bool has_sse4_1 = false;
#endif

#if defined(__micron_x86_avx)
inline constexpr bool has_avx = true;
#else
inline constexpr bool has_avx = false;
#endif

#if defined(__micron_x86_avx2)
inline constexpr bool has_avx2 = true;
#else
inline constexpr bool has_avx2 = false;
#endif

#if defined(__micron_x86_avx512f)
inline constexpr bool has_avx512f = true;
#else
inline constexpr bool has_avx512f = false;
#endif

#if defined(__micron_x86_fma)
inline constexpr bool has_fma = true;
#else
inline constexpr bool has_fma = false;
#endif

#if defined(__micron_x86_bmi1)
inline constexpr bool has_bmi1 = true;
#else
inline constexpr bool has_bmi1 = false;
#endif

#if defined(__micron_x86_bmi2)
inline constexpr bool has_bmi2 = true;
#else
inline constexpr bool has_bmi2 = false;
#endif

#if defined(__micron_x86_rdrnd)
inline constexpr bool has_rdrand = true;
#else
inline constexpr bool has_rdrand = false;
#endif

#if defined(__micron_x86_rdseed)
inline constexpr bool has_rdseed = true;
#else
inline constexpr bool has_rdseed = false;
#endif

#if defined(__micron_arm_neon)
inline constexpr bool has_neon = true;
#else
inline constexpr bool has_neon = false;
#endif

#if defined(__micron_arm_sve)
inline constexpr bool has_sve = true;
#else
inline constexpr bool has_sve = false;
#endif

#if defined(__micron_arm_sve2)
inline constexpr bool has_sve2 = true;
#else
inline constexpr bool has_sve2 = false;
#endif

#if defined(__micron_arm_fma)
inline constexpr bool has_arm_fma = true;
#else
inline constexpr bool has_arm_fma = false;
#endif

#if defined(__micron_arm_directed_rounding)
inline constexpr bool has_arm_directed_rounding = true;
#else
inline constexpr bool has_arm_directed_rounding = false;
#endif

#if defined(__micron_x86_sse2) || defined(__micron_arm_neon) || defined(__micron_arch_amd64) || defined(__micron_arch_arm64)               \
    || defined(__micron_arch_arm32)
inline constexpr bool has_fsqrt = true;
#else
inline constexpr bool has_fsqrt = false;
#endif

};      // namespace arch
};      // namespace math
};      // namespace micron
