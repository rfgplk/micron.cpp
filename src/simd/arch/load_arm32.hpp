//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../namespace.hpp"

namespace micron
{
namespace simd
{

template <int scale>
static inline const u8 *
__neon_scale_ptr(const void *base, i64 index) noexcept
{
  return reinterpret_cast<const u8 *>(base) + index * scale;
}

template <typename T>
concept is_simd_type = micron::same_as<T, f128> || micron::same_as<T, i128>;

template <is_simd_type B>
inline B
load(B *ptr)
{
  if constexpr ( micron::is_same_v<B, f128> )
    return vld1q_f32(reinterpret_cast<const float *>(ptr));
  if constexpr ( micron::is_same_v<B, i128> )
    return vreinterpretq_s32_u8(vld1q_u8(reinterpret_cast<const u8 *>(ptr)));
}

template <is_simd_type B>
inline B
load(B &ref)
{
  return load<B>(&ref);
}

template <is_simd_type B, typename T>
inline B
load(T *ptr)
{
  if constexpr ( micron::is_same_v<B, f128> )
    return vld1q_f32(reinterpret_cast<const float *>(ptr));
  if constexpr ( micron::is_same_v<B, i128> )
    return vreinterpretq_s32_u8(vld1q_u8(reinterpret_cast<const u8 *>(ptr)));
}

template <is_simd_type B, typename T>
inline B
load(T &ref)
{
  return load<B, T>(&ref);
}

template <is_simd_type B>
inline B
loadu(B *ptr)
{
  return load<B>(ptr);
}

template <is_simd_type B>
inline B
loadu(B &ref)
{
  return load<B>(ref);
}

template <is_simd_type B, typename T>
inline B
loadu(T *ptr)
{
  return load<B, T>(ptr);
}

template <is_simd_type B, typename T>
inline B
loadu(T &ref)
{
  return load<B, T>(ref);
}

template <typename T>
inline i128
loadu_16(T *ptr)
{
  uint16_t v = 0;
  __builtin_memcpy(&v, reinterpret_cast<const void *>(ptr), 2);
  return vreinterpretq_s32_u16(vsetq_lane_u16(v, vdupq_n_u16(0), 0));
}

template <typename T>
inline i128
loadu_16(T &ref)
{
  return loadu_16(&ref);
}

template <typename T>
inline i128
loadu_32(T *ptr)
{
  u32 v = 0;
  __builtin_memcpy(&v, reinterpret_cast<const void *>(ptr), 4);
  return vreinterpretq_s32_u32(vsetq_lane_u32(v, vdupq_n_u32(0), 0));
}

template <typename T>
inline i128
loadu_32(T &ref)
{
  return loadu_32(&ref);
}

template <typename T>
inline i128
loadu_64(T *ptr)
{
  i64 v = 0;
  __builtin_memcpy(&v, reinterpret_cast<const void *>(ptr), 8);
  return vreinterpretq_s32_s64(vcombine_s64(vdup_n_s64(v), vdup_n_s64(0)));
}

template <typename T>
inline i128
loadu_64(T &ref)
{
  return loadu_64(&ref);
}

inline f128
load_scalar(float *ptr)
{
  return vsetq_lane_f32(*ptr, vdupq_n_f32(0.0f), 0);
}

inline f128
load_scalar(float &ref)
{
  return load_scalar(&ref);
}

template <typename T>
inline i128
loadl_64(T *ptr)
{
  i64 v = 0;
  __builtin_memcpy(&v, reinterpret_cast<const void *>(ptr), 8);
  return vreinterpretq_s32_s64(vcombine_s64(vdup_n_s64(v), vdup_n_s64(0)));
}

template <typename T>
inline i128
loadl_64(T &ref)
{
  return loadl_64(&ref);
}

inline f128
loadl_pi(f128 dst, float32x2_t lo)
{
  return vcombine_f32(lo, vget_high_f32(dst));
}

inline f128
loadl_pi(f128 dst, float *ptr)
{
  return loadl_pi(dst, vld1_f32(ptr));
}

inline f128
loadl_pi(f128 dst, float32x2_t *ptr)
{
  return loadl_pi(dst, *ptr);
}

inline f128
loadh_pi(f128 dst, float32x2_t hi)
{
  return vcombine_f32(vget_low_f32(dst), hi);
}

inline f128
loadh_pi(f128 dst, float *ptr)
{
  return loadh_pi(dst, vld1_f32(ptr));
}

inline f128
loadh_pi(f128 dst, float32x2_t *ptr)
{
  return loadh_pi(dst, *ptr);
}

template <is_simd_type B>
inline B
stream_load(void *ptr)
{
  __builtin_prefetch(ptr, 0, 0);
  return load<B>(reinterpret_cast<B *>(ptr));
}

template <is_simd_type B>
inline B
stream_load(B *ptr)
{
  __builtin_prefetch(ptr, 0, 0);
  return load<B>(ptr);
}

template <is_simd_type B>
inline B
stream_load(B &ref)
{
  return stream_load<B>(&ref);
}

template <is_simd_type B>
inline B
broadcast_8(i128 src)
{
  static_assert(micron::is_same_v<B, i128>, "broadcast_8 target must be i128");
  return vreinterpretq_s32_s8(vdupq_n_s8(vgetq_lane_s8(vreinterpretq_s8_s32(src), 0)));
}

template <is_simd_type B>
inline B
broadcast_16(i128 src)
{
  static_assert(micron::is_same_v<B, i128>, "broadcast_16 target must be i128");
  return vreinterpretq_s32_s16(vdupq_n_s16(vgetq_lane_s16(vreinterpretq_s16_s32(src), 0)));
}

template <is_simd_type B>
inline B
broadcast_32(i128 src)
{
  static_assert(micron::is_same_v<B, i128>, "broadcast_32 target must be i128");
  return vdupq_n_s32(vgetq_lane_s32(src, 0));
}

template <is_simd_type B>
inline B
broadcast_64(i128 src)
{
  static_assert(micron::is_same_v<B, i128>, "broadcast_64 target must be i128");
  return vreinterpretq_s32_s64(vdupq_n_s64(vgetq_lane_s64(vreinterpretq_s64_s32(src), 0)));
}

template <is_simd_type B>
inline B
maskload_32(int *ptr, B mask)
{
  static_assert(micron::is_same_v<B, i128>, "maskload_32 mask/result must be i128");
  const i128 loaded = vld1q_s32(reinterpret_cast<const int32_t *>(ptr));
  const i128 smask = vshrq_n_s32(mask, 31);
  return vandq_s32(loaded, smask);
}

template <is_simd_type B>
inline B
maskload_32(int &ref, B mask)
{
  return maskload_32(&ref, mask);
}

template <is_simd_type B>
inline B
maskload_64(long long *ptr, B mask)
{
  static_assert(micron::is_same_v<B, i128>, "maskload_64 mask/result must be i128");
  const i128 loaded = vreinterpretq_s32_s64(vld1q_s64(reinterpret_cast<const i64 *>(ptr)));
  const i128 smask = vreinterpretq_s32_s64(vshrq_n_s64(vreinterpretq_s64_s32(mask), 63));
  return vandq_s32(loaded, smask);
}

template <is_simd_type B>
inline B
maskload_64(long long &ref, B mask)
{
  return maskload_64(&ref, mask);
}

template <is_simd_type B, is_simd_type M>
inline B
maskload_ps(float *ptr, M mask)
{
  static_assert(micron::is_same_v<B, f128> && micron::is_same_v<M, f128>, "maskload_ps: B and M must be f128");
  const f128 loaded = vld1q_f32(ptr);
  const int32x4_t smask = vshrq_n_s32(vreinterpretq_s32_f32(mask), 31);
  return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(loaded), smask));
}

template <is_simd_type B, is_simd_type M>
inline B
maskload_ps(float &ref, M mask)
{
  return maskload_ps<B, M>(&ref, mask);
}

template <is_simd_type B, int scale>
inline i128
gather_32_i32(int *base, i128 vindex)
{
  int32_t idx[4], out[4];
  vst1q_s32(idx, vindex);
  for ( int i = 0; i < 4; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
  return vld1q_s32(out);
}

template <is_simd_type B, int scale>
inline i128
gather_32_i32(int &ref, i128 vindex)
{
  return gather_32_i32<B, scale>(&ref, vindex);
}

template <int scale>
inline f128
gather_ps_i32(float *base, i128 vindex)
{
  int32_t idx[4];
  float out[4];
  vst1q_s32(idx, vindex);
  for ( int i = 0; i < 4; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
  return vld1q_f32(out);
}

template <int scale>
inline f128
gather_ps_i32(float &ref, i128 vindex)
{
  return gather_ps_i32<scale>(&ref, vindex);
}

template <int scale>
inline i128
gather_64_i32(long long *base, i128 vindex)
{
  int32_t idx[4];
  i64 out[2];
  vst1q_s32(idx, vindex);
  for ( int i = 0; i < 2; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 8);
  return vreinterpretq_s32_s64(vld1q_s64(out));
}

template <int scale>
inline i128
gather_64_i32(long long &ref, i128 vindex)
{
  return gather_64_i32<scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline i128
gather_32_i64(int *base, i128 vindex)
{
  i64 idx[2];
  int32_t out[4] = {};
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  for ( int i = 0; i < 2; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
  return vld1q_s32(out);
}

template <is_simd_type B, int scale>
inline i128
gather_32_i64(int &ref, i128 vindex)
{
  return gather_32_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline f128
gather_ps_i64(float *base, i128 vindex)
{
  i64 idx[2];
  float out[4] = {};
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  for ( int i = 0; i < 2; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
  return vld1q_f32(out);
}

template <is_simd_type B, int scale>
inline f128
gather_ps_i64(float &ref, i128 vindex)
{
  return gather_ps_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline i128
gather_64_i64(long long *base, i128 vindex)
{
  i64 idx[2], out[2];
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  for ( int i = 0; i < 2; ++i )
    __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 8);
  return vreinterpretq_s32_s64(vld1q_s64(out));
}

template <is_simd_type B, int scale>
inline i128
gather_64_i64(long long &ref, i128 vindex)
{
  return gather_64_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i32(i128 src, int *base, i128 vindex, i128 mask)
{
  int32_t idx[4], msrc[4], mk[4], out[4];
  vst1q_s32(idx, vindex);
  vst1q_s32(msrc, src);
  vst1q_s32(mk, mask);
  for ( int i = 0; i < 4; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
    else
      out[i] = msrc[i];
  }
  return vld1q_s32(out);
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i32(i128 src, int &ref, i128 vindex, i128 mask)
{
  return maskgather_32_i32<B, scale>(src, &ref, vindex, mask);
}

template <int scale>
inline f128
maskgather_ps_i32(f128 src, float *base, i128 vindex, f128 mask)
{
  int32_t idx[4], mk[4];
  float msrc[4], out[4];
  vst1q_s32(idx, vindex);
  vst1q_f32(msrc, src);
  vst1q_s32(mk, vreinterpretq_s32_f32(mask));
  for ( int i = 0; i < 4; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
    else
      out[i] = msrc[i];
  }
  return vld1q_f32(out);
}

template <int scale>
inline f128
maskgather_ps_i32(f128 src, float &ref, i128 vindex, f128 mask)
{
  return maskgather_ps_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline i128
maskgather_64_i32(i128 src, long long *base, i128 vindex, i128 mask)
{
  int32_t idx[4];
  i64 msrc[2], out[2], mk[2];
  vst1q_s32(idx, vindex);
  vst1q_s64(msrc, vreinterpretq_s64_s32(src));
  vst1q_s64(mk, vreinterpretq_s64_s32(mask));
  for ( int i = 0; i < 2; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 8);
    else
      out[i] = msrc[i];
  }
  return vreinterpretq_s32_s64(vld1q_s64(out));
}

template <int scale>
inline i128
maskgather_64_i32(i128 src, long long &ref, i128 vindex, i128 mask)
{
  return maskgather_64_i32<scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i64(i128 src, int *base, i128 vindex, i128 mask)
{
  i64 idx[2];
  int32_t msrc[4] = {}, out[4] = {}, mk[4] = {};
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  vst1q_s32(msrc, src);
  vst1q_s32(mk, mask);
  for ( int i = 0; i < 2; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
    else
      out[i] = msrc[i];
  }
  return vld1q_s32(out);
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i64(i128 src, int &ref, i128 vindex, i128 mask)
{
  return maskgather_32_i64<B, scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline f128
maskgather_ps_i64(f128 src, float *base, i128 vindex, f128 mask)
{
  i64 idx[2];
  float msrc[4] = {}, out[4] = {};
  int32_t mk[4] = {};
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  vst1q_f32(msrc, src);
  vst1q_s32(mk, vreinterpretq_s32_f32(mask));
  for ( int i = 0; i < 2; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 4);
    else
      out[i] = msrc[i];
  }
  return vld1q_f32(out);
}

template <is_simd_type B, int scale>
inline f128
maskgather_ps_i64(f128 src, float &ref, i128 vindex, f128 mask)
{
  return maskgather_ps_i64<B, scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline i128
maskgather_64_i64(i128 src, long long *base, i128 vindex, i128 mask)
{
  i64 idx[2], msrc[2], out[2], mk[2];
  vst1q_s64(idx, vreinterpretq_s64_s32(vindex));
  vst1q_s64(msrc, vreinterpretq_s64_s32(src));
  vst1q_s64(mk, vreinterpretq_s64_s32(mask));
  for ( int i = 0; i < 2; ++i ) {
    if ( mk[i] < 0 )
      __builtin_memcpy(&out[i], __neon_scale_ptr<scale>(base, idx[i]), 8);
    else
      out[i] = msrc[i];
  }
  return vreinterpretq_s32_s64(vld1q_s64(out));
}

template <is_simd_type B, int scale>
inline i128
maskgather_64_i64(i128 src, long long &ref, i128 vindex, i128 mask)
{
  return maskgather_64_i64<B, scale>(src, &ref, vindex, mask);
}

};     // namespace simd
};     // namespace micron
