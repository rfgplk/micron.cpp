// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "vector.hpp"

namespace micron::simd
{
template<typename T>
concept __wasm_simd_class = requires {
  typename T::bit_width;
  typename T::lane_width;
};

template<typename U, unsigned Op>
inline constexpr U
__wasm_shift(U a, U b, u64 count) noexcept
{
  constexpr unsigned width = sizeof(U) * 8;
  if constexpr ( Op >= 3 ) count &= width - 1;
  if ( count == 0 ) return a;
  if constexpr ( Op == 0 ) return count >= width ? U(0) : U(u64(a) << count);
  if constexpr ( Op == 1 ) return count >= width ? U(0) : U(a >> count);
  if constexpr ( Op == 2 ) {
    const U fill = U(0) - U(a >> (width - 1));
    return count >= width ? fill : U((a >> count) | (u64(fill) << (width - count)));
  }
  if constexpr ( Op == 3 ) return U((u64(a) << count) | (a >> (width - count)));
  if constexpr ( Op == 4 ) return U((a >> count) | (u64(a) << (width - count)));
  if constexpr ( Op == 5 ) return U((u64(a) << count) | (b >> (width - count)));
  if constexpr ( Op == 6 ) return U((a >> count) | (u64(b) << (width - count)));
}

template<typename U, unsigned Op, bool Variable, __wasm_simd_class T>
inline T
__wasm_shift_apply(T a, T b, T counts, u64 count) noexcept
{
  using B = typename T::bit_width;
  auto av = __wasm_unpack<U>(B(a));
  const auto bv = __wasm_unpack<U>(B(b)), cv = __wasm_unpack<U>(B(counts));
  for ( usize i = 0; i < sizeof(B) / sizeof(U); ++i )
    av.lane[i] = __wasm_shift<U, Op>(av.lane[i], bv.lane[i], Variable ? cv.lane[i] : count);
  return __wasm_pack<B>(av);
}

template<typename U, __wasm_simd_class T, typename M>
inline T
__wasm_mask_merge(T src, M mask, T value) noexcept
{
  using B = typename T::bit_width;
  auto out = __wasm_unpack<U>(B(src));
  const auto v = __wasm_unpack<U>(B(value));
  for ( usize i = 0; i < sizeof(B) / sizeof(U); ++i )
    if ( (u64(mask) >> i) & 1 ) out.lane[i] = v.lane[i];
  return __wasm_pack<B>(out);
}

template<__wasm_simd_class T>
inline T
byte_shift_left(T &o, int count) noexcept
{
  using B = typename T::bit_width;
  auto v = __wasm_unpack<u8>(B(o));
  __wasm_lanes<u8, sizeof(B)> out{};
  if ( unsigned(count) < 16 )
    for ( usize i = 0; i < sizeof(B); ++i )
      if ( i % 16 >= unsigned(count) ) out.lane[i] = v.lane[i - count];
  return __wasm_pack<B>(out);
}

template<__wasm_simd_class T>
inline T
byte_shift_right(T &o, int count) noexcept
{
  using B = typename T::bit_width;
  auto v = __wasm_unpack<u8>(B(o));
  __wasm_lanes<u8, sizeof(B)> out{};
  if ( unsigned(count) < 16 )
    for ( usize i = 0; i < sizeof(B); ++i )
      if ( i % 16 + unsigned(count) < 16 ) out.lane[i] = v.lane[i + count];
  return __wasm_pack<B>(out);
}

template<__wasm_simd_class T>
inline T
slli_si(T &o, int n) noexcept
{
  return byte_shift_left(o, n);
}

template<__wasm_simd_class T>
inline T
srli_si(T &o, int n) noexcept
{
  return byte_shift_right(o, n);
}

#define __MC_WASM_SHIFT(NAME, OP, N)                                                                                                       \
  template<__wasm_simd_class T> inline T NAME##_##N(T &o, i128 n) noexcept                                                                 \
  {                                                                                                                                        \
    return __wasm_shift_apply<u##N, OP, false>(o, T{}, T{}, __wasm_unpack<u64>(n).lane[0]);                                                \
  }                                                                                                                                        \
  template<__wasm_simd_class T> inline T NAME##i_##N(T &o, int n) noexcept                                                                 \
  {                                                                                                                                        \
    return __wasm_shift_apply<u##N, OP, false>(o, T{}, T{}, u64(n));                                                                       \
  }                                                                                                                                        \
  template<__wasm_simd_class T> inline T NAME##v_##N(T &o, T &n) noexcept { return __wasm_shift_apply<u##N, OP, true>(o, T{}, n, 0); }     \
  template<__wasm_simd_class T, typename M> inline T mask_##NAME##i_##N(T &src, M k, T &o, int n) noexcept                                 \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(src, k, NAME##i_##N(o, n));                                                                             \
  }                                                                                                                                        \
  template<__wasm_simd_class T, typename M> inline T maskz_##NAME##i_##N(M k, T &o, int n) noexcept                                        \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(T{}, k, NAME##i_##N(o, n));                                                                             \
  }
__MC_WASM_SHIFT(sll, 0, 16)
__MC_WASM_SHIFT(sll, 0, 32)
__MC_WASM_SHIFT(sll, 0, 64)
__MC_WASM_SHIFT(srl, 1, 16)
__MC_WASM_SHIFT(srl, 1, 32)
__MC_WASM_SHIFT(srl, 1, 64)
__MC_WASM_SHIFT(sra, 2, 16)
__MC_WASM_SHIFT(sra, 2, 32)
__MC_WASM_SHIFT(sra, 2, 64)
#undef __MC_WASM_SHIFT
#define __MC_WASM_ROTATE(NAME, OP, N)                                                                                                      \
  template<__wasm_simd_class T> inline T NAME##_##N(T &o, int n) noexcept                                                                  \
  {                                                                                                                                        \
    return __wasm_shift_apply<u##N, OP, false>(o, T{}, T{}, u64(n));                                                                       \
  }                                                                                                                                        \
  template<__wasm_simd_class T> inline T NAME##v_##N(T &o, T &n) noexcept { return __wasm_shift_apply<u##N, OP, true>(o, T{}, n, 0); }     \
  template<__wasm_simd_class T, typename M> inline T mask_##NAME##_##N(T &src, M k, T &o, int n) noexcept                                  \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(src, k, NAME##_##N(o, n));                                                                              \
  }                                                                                                                                        \
  template<__wasm_simd_class T, typename M> inline T maskz_##NAME##_##N(M k, T &o, int n) noexcept                                         \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(T{}, k, NAME##_##N(o, n));                                                                              \
  }
__MC_WASM_ROTATE(rol, 3, 32)
__MC_WASM_ROTATE(rol, 3, 64)
__MC_WASM_ROTATE(ror, 4, 32)
__MC_WASM_ROTATE(ror, 4, 64)
#undef __MC_WASM_ROTATE
#define __MC_WASM_DOUBLE_SHIFT(NAME, OP, N)                                                                                                \
  template<__wasm_simd_class T> inline T NAME##i_##N(T &a, T &b, int n) noexcept                                                           \
  {                                                                                                                                        \
    return __wasm_shift_apply<u##N, OP, false>(a, b, T{}, u64(n));                                                                         \
  }                                                                                                                                        \
  template<__wasm_simd_class T> inline T NAME##v_##N(T &a, T &b, T &n) noexcept { return __wasm_shift_apply<u##N, OP, true>(a, b, n, 0); } \
  template<__wasm_simd_class T, typename M> inline T mask_##NAME##i_##N(T &src, M k, T &a, T &b, int n) noexcept                           \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(src, k, NAME##i_##N(a, b, n));                                                                          \
  }                                                                                                                                        \
  template<__wasm_simd_class T, typename M> inline T maskz_##NAME##i_##N(M k, T &a, T &b, int n) noexcept                                  \
  {                                                                                                                                        \
    return __wasm_mask_merge<u##N>(T{}, k, NAME##i_##N(a, b, n));                                                                          \
  }
__MC_WASM_DOUBLE_SHIFT(shld, 5, 16)
__MC_WASM_DOUBLE_SHIFT(shld, 5, 32)
__MC_WASM_DOUBLE_SHIFT(shld, 5, 64)
__MC_WASM_DOUBLE_SHIFT(shrd, 6, 16)
__MC_WASM_DOUBLE_SHIFT(shrd, 6, 32)
__MC_WASM_DOUBLE_SHIFT(shrd, 6, 64)
#undef __MC_WASM_DOUBLE_SHIFT
#define __MC_WASM_INFER_SHIFT(NAME, ARG)                                                                                                   \
  template<__wasm_simd_class T> inline T NAME(T &a, ARG n) noexcept                                                                        \
  {                                                                                                                                        \
    if constexpr ( sizeof(typename T::lane_width) == 2 )                                                                                   \
      return NAME##_16(a, n);                                                                                                              \
    else if constexpr ( sizeof(typename T::lane_width) == 4 )                                                                              \
      return NAME##_32(a, n);                                                                                                              \
    else                                                                                                                                   \
      return NAME##_64(a, n);                                                                                                              \
  }
__MC_WASM_INFER_SHIFT(sll, i128)
__MC_WASM_INFER_SHIFT(srl, i128)
__MC_WASM_INFER_SHIFT(sra, i128)
__MC_WASM_INFER_SHIFT(slli, int)
__MC_WASM_INFER_SHIFT(srli, int)
__MC_WASM_INFER_SHIFT(srai, int)
__MC_WASM_INFER_SHIFT(sllv, T &)
__MC_WASM_INFER_SHIFT(srlv, T &)
__MC_WASM_INFER_SHIFT(srav, T &)
#undef __MC_WASM_INFER_SHIFT
#define __MC_WASM_INFER_ROTATE(NAME, ARG)                                                                                                  \
  template<__wasm_simd_class T> inline T NAME(T &a, ARG n) noexcept                                                                        \
  {                                                                                                                                        \
    if constexpr ( sizeof(typename T::lane_width) == 4 )                                                                                   \
      return NAME##_32(a, n);                                                                                                              \
    else                                                                                                                                   \
      return NAME##_64(a, n);                                                                                                              \
  }
__MC_WASM_INFER_ROTATE(rol, int)
__MC_WASM_INFER_ROTATE(ror, int)
__MC_WASM_INFER_ROTATE(rolv, T &)
__MC_WASM_INFER_ROTATE(rorv, T &)
#undef __MC_WASM_INFER_ROTATE
#define __MC_WASM_INFER_DOUBLE(NAME, ARG)                                                                                                  \
  template<__wasm_simd_class T> inline T NAME(T &a, T &b, ARG n) noexcept                                                                  \
  {                                                                                                                                        \
    if constexpr ( sizeof(typename T::lane_width) == 2 )                                                                                   \
      return NAME##_16(a, b, n);                                                                                                           \
    else if constexpr ( sizeof(typename T::lane_width) == 4 )                                                                              \
      return NAME##_32(a, b, n);                                                                                                           \
    else                                                                                                                                   \
      return NAME##_64(a, b, n);                                                                                                           \
  }
__MC_WASM_INFER_DOUBLE(shldi, int)
__MC_WASM_INFER_DOUBLE(shrdi, int)
__MC_WASM_INFER_DOUBLE(shldv, T &)
__MC_WASM_INFER_DOUBLE(shrdv, T &)
#undef __MC_WASM_INFER_DOUBLE
};      // namespace micron::simd
