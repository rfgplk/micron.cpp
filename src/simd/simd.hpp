//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "bits.hpp"     // arched
#include "dispatch.hpp"
#include "load.hpp"      // arched
#include "types.hpp"     // arched

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include "types/simd_dispatch.hpp"

namespace micron
{
namespace simd
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 128-bit (all archs)
// NB: d128 is arm64 + x86 only; arm32 NEON has no f64 vector type.

using v8 = v128<i128, __v8>;
using v16 = v128<i128, __v16>;
using v32 = v128<i128, __v32>;
using v64 = v128<i128, __v64>;
using vfloat = v128<f128, __vf>;
#if !defined(__micron_arch_arm32)
using vdouble = v128<d128, __vd>;
#endif

// Eigen-like names
using packet16c = v128<i128, __v8>;
using packet8s = v128<i128, __v16>;
using packet4i = v128<i128, __v32>;
using packet2l = v128<i128, __v64>;
using packet4f = v128<f128, __vf>;
#if !defined(__micron_arch_arm32)
using packet2d = v128<d128, __vd>;
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 256-bit (x86 only; no NEON v256 implementation)

#if defined(__micron_arch_x86_any)
using w8 = v256<i256, __v8>;
using w16 = v256<i256, __v16>;
using w32 = v256<i256, __v32>;
using w64 = v256<i256, __v64>;
using wfloat = v256<f256, __vf>;
using wdouble = v256<d256, __vd>;

using packet32c = v256<i256, __v8>;
using packet16s = v256<i256, __v16>;
using packet8i = v256<i256, __v32>;
using packet4l = v256<i256, __v64>;
using packet8f = v256<f256, __vf>;
using packet4d = v256<d256, __vd>;
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 512-bit (x86 only)

#if defined(__micron_arch_x86_any)
using z8 = v512<i512, __v8>;
using z16 = v512<i512, __v16>;
using z32 = v512<i512, __v32>;
using z64 = v512<i512, __v64>;
using zfloat = v512<f512, __vf>;
using zdouble = v512<d512, __vd>;

using packet64c = v512<i512, __v8>;
using packet32s = v512<i512, __v16>;
using packet16i = v512<i512, __v32>;
using packet8l = v512<i512, __v64>;
using packet16f = v512<f512, __vf>;
using packet8d = v512<d512, __vd>;
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// concept: only members that actually exist on this arch

template <typename T>
concept is_simd_class
    = micron::same_as<T, v8> or micron::same_as<T, v16> or micron::same_as<T, v32> or micron::same_as<T, v64> or micron::same_as<T, vfloat>
#if !defined(__micron_arch_arm32)
      or micron::same_as<T, vdouble>
#endif
#if defined(__micron_arch_x86_any)
      or micron::same_as<T, w8> or micron::same_as<T, w16> or micron::same_as<T, w32> or micron::same_as<T, w64>
      or micron::same_as<T, wfloat> or micron::same_as<T, wdouble>
#endif
    ;

};     // namespace simd

#pragma GCC diagnostic pop

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// forwarding aliases in micron:: scope

using v8 = simd::v8;
using v16 = simd::v16;
using v32 = simd::v32;
using v64 = simd::v64;
using vfloat = simd::vfloat;
#if !defined(__micron_arch_arm32)
using vdouble = simd::vdouble;
#endif

using packet16c = simd::packet16c;
using packet8s = simd::packet8s;
using packet4i = simd::packet4i;
using packet2l = simd::packet2l;
using packet4f = simd::packet4f;
#if !defined(__micron_arch_arm32)
using packet2d = simd::packet2d;
#endif

#if defined(__micron_arch_x86_any)
using w8 = simd::w8;
using w16 = simd::w16;
using w32 = simd::w32;
using w64 = simd::w64;
using wfloat = simd::wfloat;
using wdouble = simd::wdouble;

using packet32c = simd::packet32c;
using packet16s = simd::packet16s;
using packet8i = simd::packet8i;
using packet4l = simd::packet4l;
using packet8f = simd::packet8f;
using packet4d = simd::packet4d;

using z8 = simd::z8;
using z16 = simd::z16;
using z32 = simd::z32;
using z64 = simd::z64;
using zfloat = simd::zfloat;
using zdouble = simd::zdouble;

using packet64c = simd::packet64c;
using packet32s = simd::packet32s;
using packet16i = simd::packet16i;
using packet8l = simd::packet8l;
using packet16f = simd::packet16f;
using packet8d = simd::packet8d;
#endif

};     // namespace micron
