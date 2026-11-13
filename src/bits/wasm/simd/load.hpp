// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron::simd
{
template<is_simd_type B, typename T>
inline B
load(const T *p) noexcept
{
  B out;
  __builtin_memcpy(&out, p, sizeof(B));
  return out;
}

template<is_simd_type B, typename T>
inline B
load(T &v) noexcept
  requires(!micron::is_pointer_v<T>)
{
  return load<B>(&v);
}

template<is_simd_type B, typename T>
inline B
loadu(const T *p) noexcept
{
  return load<B>(p);
}

template<is_simd_type B, typename T>
inline B
loadu(T &v) noexcept
  requires(!micron::is_pointer_v<T>)
{
  return load<B>(&v);
}

template<usize N, typename T>
inline i128
__wasm_load_low(const T *p) noexcept
{
  i128 out{};
  __builtin_memcpy(&out, p, N);
  return out;
}

#define __MC_WASM_LOAD_LOW(N)                                                                                                              \
  template<typename T> inline i128 loadu_##N(const T *p) noexcept { return __wasm_load_low<N / 8>(p); }                                    \
  template<typename T>                                                                                                                     \
  inline i128 loadu_##N(T &p) noexcept                                                                                                     \
    requires(!micron::is_pointer_v<T>)                                                                                                     \
  {                                                                                                                                        \
    return loadu_##N(&p);                                                                                                                  \
  }
__MC_WASM_LOAD_LOW(16)
__MC_WASM_LOAD_LOW(32)
__MC_WASM_LOAD_LOW(64)
#undef __MC_WASM_LOAD_LOW

template<typename T>
inline i128
loadl_64(const T *p) noexcept
{
  return loadu_64(p);
}

template<typename T>
inline i128
loadl_64(T &v) noexcept
  requires(!micron::is_pointer_v<T>)
{
  return loadu_64(&v);
}

inline f128
load_scalar(const float *p) noexcept
{
  __wasm_lanes<float, 4> v{};
  v.lane[0] = *p;
  return __wasm_pack<f128>(v);
}

inline d128
load_scalar(const double *p) noexcept
{
  __wasm_lanes<double, 2> v{};
  v.lane[0] = *p;
  return __wasm_pack<d128>(v);
}

inline f128
load_scalar(float &v) noexcept
{
  return load_scalar(&v);
}

inline d128
load_scalar(double &v) noexcept
{
  return load_scalar(&v);
}

inline d128
loadl_pd(d128 dst, const double *p) noexcept
{
  auto v = __wasm_unpack<double>(dst);
  v.lane[0] = *p;
  return __wasm_pack<d128>(v);
}

inline d128
loadh_pd(d128 dst, const double *p) noexcept
{
  auto v = __wasm_unpack<double>(dst);
  v.lane[1] = *p;
  return __wasm_pack<d128>(v);
}

inline d128
loadl_pd(d128 dst, double &v) noexcept
{
  return loadl_pd(dst, &v);
}

inline d128
loadh_pd(d128 dst, double &v) noexcept
{
  return loadh_pd(dst, &v);
}

inline f128
loadl_pi(f128 dst, const float *p) noexcept
{
  auto v = __wasm_unpack<float>(dst);
  __builtin_memcpy(v.lane, p, 8);
  return __wasm_pack<f128>(v);
}

inline f128
loadh_pi(f128 dst, const float *p) noexcept
{
  auto v = __wasm_unpack<float>(dst);
  __builtin_memcpy(v.lane + 2, p, 8);
  return __wasm_pack<f128>(v);
}

template<is_simd_type B, typename T>
inline B
stream_load(const T *p) noexcept
{
  return load<B>(p);
}

template<is_simd_type B, typename T>
inline B
stream_load(T &v) noexcept
  requires(!micron::is_pointer_v<T>)
{
  return load<B>(&v);
}

#define __MC_WASM_BROADCAST(N)                                                                                                             \
  template<is_simd_type B> inline B broadcast_##N(i128 src) noexcept { return __wasm_splat<B>(__wasm_unpack<u##N>(src).lane[0]); }
__MC_WASM_BROADCAST(8)
__MC_WASM_BROADCAST(16)
__MC_WASM_BROADCAST(32)
__MC_WASM_BROADCAST(64)
#undef __MC_WASM_BROADCAST

template<typename F, is_simd_type B, is_simd_type M>
inline B
__wasm_maskload(const F *p, M mask) noexcept
{
  static_assert(sizeof(B) == sizeof(M));
  using U = micron::conditional_t<sizeof(F) == 8, u64, u32>;
  const auto m = __wasm_unpack<U>(mask);
  __wasm_lanes<F, sizeof(B) / sizeof(F)> out{};
  for ( usize i = 0; i < sizeof(B) / sizeof(F); ++i )
    if ( m.lane[i] >> (sizeof(F) * 8 - 1) ) __builtin_memcpy(out.lane + i, p + i, sizeof(F));
  return __wasm_pack<B>(out);
}

template<is_simd_type B>
inline B
maskload_32(const int *p, B mask) noexcept
{
  return __wasm_maskload<int, B>(p, mask);
}

template<is_simd_type B>
inline B
maskload_64(const long long *p, B mask) noexcept
{
  return __wasm_maskload<long long, B>(p, mask);
}

template<is_simd_type B>
inline B
maskload_32(int &v, B mask) noexcept
{
  return maskload_32(&v, mask);
}

template<is_simd_type B>
inline B
maskload_64(long long &v, B mask) noexcept
{
  return maskload_64(&v, mask);
}

template<is_simd_type B, is_simd_type M>
inline B
maskload_ps(const float *p, M mask) noexcept
{
  return __wasm_maskload<float, B>(p, mask);
}

template<is_simd_type B, is_simd_type M>
inline B
maskload_pd(const double *p, M mask) noexcept
{
  return __wasm_maskload<double, B>(p, mask);
}

template<is_simd_type B, is_simd_type M>
inline B
maskload_ps(float &v, M mask) noexcept
{
  return maskload_ps<B>(&v, mask);
}

template<is_simd_type B, is_simd_type M>
inline B
maskload_pd(double &v, M mask) noexcept
{
  return maskload_pd<B>(&v, mask);
}

template<typename F, typename I, int Scale, is_simd_type B>
inline B
__wasm_gather(B source, const F *base, i128 indices, B mask) noexcept
{
  static_assert(Scale == 1 || Scale == 2 || Scale == 4 || Scale == 8);
  using U = micron::conditional_t<sizeof(F) == 8, u64, u32>;
  constexpr usize count = sizeof(B) / sizeof(F) < 16 / sizeof(I) ? sizeof(B) / sizeof(F) : 16 / sizeof(I);
  const auto ix = __wasm_unpack<I>(indices);
  const auto m = __wasm_unpack<U>(mask);
  auto out = __wasm_unpack<F>(source);
  for ( usize i = 0; i < count; ++i )
    if ( m.lane[i] >> (sizeof(F) * 8 - 1) )
      __builtin_memcpy(out.lane + i, reinterpret_cast<const u8 *>(base) + i64(ix.lane[i]) * Scale, sizeof(F));
  for ( usize i = count; i < sizeof(B) / sizeof(F); ++i ) out.lane[i] = F(0);
  return __wasm_pack<B>(out);
}

#define __MC_WASM_GATHER(NAME, F, I, R, ...)                                                                                               \
  template<__VA_ARGS__ int Scale> inline R gather_##NAME(const F *base, i128 indices) noexcept                                             \
  {                                                                                                                                        \
    return __wasm_gather<F, I, Scale>(R{}, base, indices, __wasm_splat<R>(~u64(0)));                                                       \
  }                                                                                                                                        \
  template<__VA_ARGS__ int Scale> inline R gather_##NAME(F &base, i128 indices) noexcept                                                   \
  {                                                                                                                                        \
    return __wasm_gather<F, I, Scale>(R{}, &base, indices, __wasm_splat<R>(~u64(0)));                                                      \
  }                                                                                                                                        \
  template<__VA_ARGS__ int Scale> inline R maskgather_##NAME(R src, const F *base, i128 indices, R mask) noexcept                          \
  {                                                                                                                                        \
    return __wasm_gather<F, I, Scale>(src, base, indices, mask);                                                                           \
  }                                                                                                                                        \
  template<__VA_ARGS__ int Scale> inline R maskgather_##NAME(R src, F &base, i128 indices, R mask) noexcept                                \
  {                                                                                                                                        \
    return __wasm_gather<F, I, Scale>(src, &base, indices, mask);                                                                          \
  }
__MC_WASM_GATHER(32_i32, int, i32, i128, is_simd_type B, )
__MC_WASM_GATHER(ps_i32, float, i32, f128, )
__MC_WASM_GATHER(64_i32, long long, i32, i128, )
__MC_WASM_GATHER(pd_i32, double, i32, d128, )
__MC_WASM_GATHER(32_i64, int, i64, i128, is_simd_type B, )
__MC_WASM_GATHER(ps_i64, float, i64, f128, is_simd_type B, )
__MC_WASM_GATHER(64_i64, long long, i64, i128, is_simd_type B, )
__MC_WASM_GATHER(pd_i64, double, i64, d128, is_simd_type B, )
#undef __MC_WASM_GATHER
};      // namespace micron::simd
