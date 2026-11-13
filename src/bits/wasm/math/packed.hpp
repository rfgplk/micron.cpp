// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../math/bits/trig.hpp"
#include "../simd/math.hpp"
#include "packed_types.hpp"

namespace micron::math::mk
{
#define __MC_WASM_PACKED(NAME, NS)                                                                                                         \
  template<packed_real V> inline V NAME(V x) noexcept                                                                                      \
  {                                                                                                                                        \
    return simd::__wasm_math_map<elem_t<V>>(x, [](elem_t<V> a) { return mkbits::NS::NAME<elem_t<V>>(a); });                                \
  }
__MC_WASM_PACKED(sqrt, sqrt_ns)
__MC_WASM_PACKED(rsqrt, sqrt_ns)
__MC_WASM_PACKED(exp, exp_ns)
__MC_WASM_PACKED(exp2, exp_ns)
__MC_WASM_PACKED(exp10, exp_ns)
__MC_WASM_PACKED(expm1, exp_ns)
__MC_WASM_PACKED(log, log_ns)
__MC_WASM_PACKED(log2, log_ns)
__MC_WASM_PACKED(log10, log_ns)
__MC_WASM_PACKED(log1p, log_ns)
__MC_WASM_PACKED(sin, trig_ns)
__MC_WASM_PACKED(cos, trig_ns)
__MC_WASM_PACKED(tan, trig_ns)
__MC_WASM_PACKED(fabs, manip)
__MC_WASM_PACKED(fneg, manip)
__MC_WASM_PACKED(ceil, round_ns)
__MC_WASM_PACKED(floor, round_ns)
__MC_WASM_PACKED(trunc, round_ns)
__MC_WASM_PACKED(rint, round_ns)
#undef __MC_WASM_PACKED

template<packed_real V>
inline V
copysign(V a, V b) noexcept
{
  return simd::__wasm_math_zip<elem_t<V>>(a, b, [](elem_t<V> x, elem_t<V> y) { return mkbits::manip::copysign(x, y); });
}

template<packed_real V>
inline void
sincos(V x, V *s, V *c) noexcept
{
  *s = mk::sin(x);
  *c = mk::cos(x);
}

template<packed_real V>
inline V
sin_cordic(V x) noexcept
{
  return mk::sin(x);
}

template<packed_real V>
inline V
cos_cordic(V x) noexcept
{
  return mk::cos(x);
}

template<packed_real V>
inline V
tan_cordic(V x) noexcept
{
  return mk::tan(x);
}

template<packed_real V>
inline void
sincos_cordic(V x, V *s, V *c) noexcept
{
  mk::sincos(x, s, c);
}
}      // namespace micron::math::mk
