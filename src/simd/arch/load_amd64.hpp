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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// load/aligned loads

template <is_simd_type B>
inline B
load(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_load_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_load_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_load_pd(reinterpret_cast<double *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_load_si256(ptr);
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_load_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_load_pd(reinterpret_cast<double *>(ptr));
  }
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
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_load_si128(reinterpret_cast<i128 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_load_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_load_pd(reinterpret_cast<double *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_load_si256(reinterpret_cast<i256 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_load_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_load_pd(reinterpret_cast<double *>(ptr));
  }
}

template <is_simd_type B, typename T>
inline B
load(T &ref)
{
  return load<B, T>(&ref);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// loadu/unaligned loads

template <is_simd_type B>
inline B
loadu(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_loadu_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_loadu_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_loadu_pd(reinterpret_cast<double *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_loadu_si256(ptr);
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_loadu_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_loadu_pd(reinterpret_cast<double *>(ptr));
  }
}

template <is_simd_type B>
inline B
loadu(B &ref)
{
  return loadu<B>(&ref);
}

template <is_simd_type B, typename T>
inline B
loadu(T *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_loadu_si128(reinterpret_cast<i128 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_loadu_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_loadu_pd(reinterpret_cast<double *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_loadu_si256(reinterpret_cast<i256 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_loadu_ps(reinterpret_cast<float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_loadu_pd(reinterpret_cast<double *>(ptr));
  }
}

template <is_simd_type B, typename T>
inline B
loadu(const T *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_loadu_si128(reinterpret_cast<const i128 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_loadu_ps(reinterpret_cast<const float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_loadu_pd(reinterpret_cast<const double *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_loadu_si256(reinterpret_cast<const i256 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_loadu_ps(reinterpret_cast<const float *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_loadu_pd(reinterpret_cast<const double *>(ptr));
  }
}

template <is_simd_type B, typename T>
inline B
loadu(T &ref)
{
  return loadu<B, T>(&ref);
}

template <typename T>
inline i128
loadu_16(T *ptr)
{
  return _mm_loadu_si16(ptr);
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
  return _mm_loadu_si32(ptr);
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
  return _mm_loadu_si64(ptr);
}

template <typename T>
inline i128
loadu_64(T &ref)
{
  return loadu_64(&ref);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scalar loads

inline f128
load_scalar(float *ptr)
{
  return _mm_load_ss(ptr);
}

inline f128
load_scalar(float &ref)
{
  return load_scalar(&ref);
}

inline d128
load_scalar(double *ptr)
{
  return _mm_load_sd(ptr);
}

inline d128
load_scalar(double &ref)
{
  return load_scalar(&ref);
}

template <typename T>
inline i128
loadl_64(T *ptr)
{
  return _mm_loadl_epi64(reinterpret_cast<i128 *>(ptr));
}

template <typename T>
inline i128
loadl_64(T &ref)
{
  return loadl_64(&ref);
}

inline d128
loadl_pd(d128 dst, double *ptr)
{
  return _mm_loadl_pd(dst, ptr);
}

inline d128
loadl_pd(d128 dst, double &ref)
{
  return loadl_pd(dst, &ref);
}

inline f128
loadl_pi(f128 dst, __m64 *ptr)
{
  return _mm_loadl_pi(dst, ptr);
}

inline f128
loadl_pi(f128 dst, __m64 &ref)
{
  return loadl_pi(dst, &ref);
}

inline d128
loadh_pd(d128 dst, double *ptr)
{
  return _mm_loadh_pd(dst, ptr);
}

inline d128
loadh_pd(d128 dst, double &ref)
{
  return loadh_pd(dst, &ref);
}

inline f128
loadh_pi(f128 dst, __m64 *ptr)
{
  return _mm_loadh_pi(dst, ptr);
}

inline f128
loadh_pi(f128 dst, __m64 &ref)
{
  return loadh_pi(dst, &ref);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// streaming loads

template <is_simd_type B>
inline B
stream_load(void *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_stream_load_si128(reinterpret_cast<i128 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_stream_load_si256(reinterpret_cast<i256 *>(ptr));
  }
}

template <is_simd_type B>
inline B
stream_load(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_stream_load_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_stream_load_si256(ptr);
  }
}

template <is_simd_type B>
inline B
stream_load(B &ref)
{
  return stream_load<B>(&ref);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// broadcasts

inline f256
broadcast_ss(float *ptr)
{
  return _mm256_broadcast_ss(ptr);
}

inline f256
broadcast_ss(float &ref)
{
  return broadcast_ss(&ref);
}

inline d256
broadcast_sd(double *ptr)
{
  return _mm256_broadcast_sd(ptr);
}

inline d256
broadcast_sd(double &ref)
{
  return broadcast_sd(&ref);
}

inline f256
broadcast_ps(f128 *ptr)
{
  return _mm256_broadcast_ps(ptr);
}

inline f256
broadcast_ps(f128 &ref)
{
  return broadcast_ps(&ref);
}

inline d256
broadcast_pd(d128 *ptr)
{
  return _mm256_broadcast_pd(ptr);
}

inline d256
broadcast_pd(d128 &ref)
{
  return broadcast_pd(&ref);
}

inline i256
broadcast_si128(i128 *ptr)
{
  return _mm256_broadcastsi128_si256(*ptr);
}

inline i256
broadcast_si128(i128 &ref)
{
  return broadcast_si128(&ref);
}

template <is_simd_type B>
inline B
broadcast_8(i128 src)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_broadcastb_epi8(src);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_broadcastb_epi8(src);
  }
}

template <is_simd_type B>
inline B
broadcast_16(i128 src)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_broadcastw_epi16(src);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_broadcastw_epi16(src);
  }
}

template <is_simd_type B>
inline B
broadcast_32(i128 src)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_broadcastd_epi32(src);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_broadcastd_epi32(src);
  }
}

template <is_simd_type B>
inline B
broadcast_64(i128 src)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_broadcastq_epi64(src);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_broadcastq_epi64(src);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// maskes loads

template <is_simd_type B>
inline B
maskload_32(int *ptr, B mask)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_maskload_epi32(ptr, mask);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_maskload_epi32(ptr, mask);
  }
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
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_maskload_epi64(ptr, mask);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_maskload_epi64(ptr, mask);
  }
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
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_maskload_ps(ptr, mask);
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_maskload_ps(ptr, mask);
  }
}

template <is_simd_type B, is_simd_type M>
inline B
maskload_ps(float &ref, M mask)
{
  return maskload_ps<B>(&ref, mask);
}

template <is_simd_type B, is_simd_type M>
inline B
maskload_pd(double *ptr, M mask)
{
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_maskload_pd(ptr, mask);
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_maskload_pd(ptr, mask);
  }
}

template <is_simd_type B, is_simd_type M>
inline B
maskload_pd(double &ref, M mask)
{
  return maskload_pd<B>(&ref, mask);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gathers

template <is_simd_type B, int scale>
inline B
gather_32_i32(int *base, B vindex)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_i32gather_epi32(base, vindex, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_i32gather_epi32(base, vindex, scale);
  }
}

template <is_simd_type B, int scale>
inline B
gather_32_i32(int &ref, B vindex)
{
  return gather_32_i32<B, scale>(&ref, vindex);
}

template <int scale>
inline f128
gather_ps_i32(float *base, i128 vindex)
{
  return _mm_i32gather_ps(base, vindex, scale);
}

template <int scale>
inline f128
gather_ps_i32(float &ref, i128 vindex)
{
  return gather_ps_i32<scale>(&ref, vindex);
}

template <int scale>
inline f256
gather_ps_i32(float *base, i256 vindex)
{
  return _mm256_i32gather_ps(base, vindex, scale);
}

template <int scale>
inline f256
gather_ps_i32(float &ref, i256 vindex)
{
  return gather_ps_i32<scale>(&ref, vindex);
}

template <int scale>
inline i128
gather_64_i32(long long *base, i128 vindex)
{
  return _mm_i32gather_epi64(base, vindex, scale);
}

template <int scale>
inline i128
gather_64_i32(long long &ref, i128 vindex)
{
  return gather_64_i32<scale>(&ref, vindex);
}

template <int scale>
inline i256
gather_64_i32(long long *base, i128 vindex)
{
  return _mm256_i32gather_epi64(base, vindex, scale);
}

template <int scale>
inline i256
gather_64_i32(long long &ref, i256 vindex)
{
  return gather_64_i32<scale>(&ref, vindex);
}

template <int scale>
inline d128
gather_pd_i32(double *base, i128 vindex)
{
  return _mm_i32gather_pd(base, vindex, scale);
}

template <int scale>
inline d128
gather_pd_i32(double &ref, i128 vindex)
{
  return gather_pd_i32<scale>(&ref, vindex);
}

template <int scale>
inline d256
gather_pd_i32(double *base, i128 vindex)
{
  return _mm256_i32gather_pd(base, vindex, scale);
}

template <int scale>
inline d256
gather_pd_i32(double &ref, i128 vindex)
{
  return gather_pd_i32<scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline i128
gather_32_i64(int *base, B vindex)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_i64gather_epi32(base, vindex, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_i64gather_epi32(base, vindex, scale);
  }
}

template <is_simd_type B, int scale>
inline i128
gather_32_i64(int &ref, B vindex)
{
  return gather_32_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline f128
gather_ps_i64(float *base, B vindex)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_i64gather_ps(base, vindex, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_i64gather_ps(base, vindex, scale);
  }
}

template <is_simd_type B, int scale>
inline f128
gather_ps_i64(float &ref, B vindex)
{
  return gather_ps_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline B
gather_64_i64(long long *base, B vindex)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_i64gather_epi64(base, vindex, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_i64gather_epi64(base, vindex, scale);
  }
}

template <is_simd_type B, int scale>
inline B
gather_64_i64(long long &ref, B vindex)
{
  return gather_64_i64<B, scale>(&ref, vindex);
}

template <is_simd_type B, int scale>
inline B
gather_pd_i64(double *base, B vindex)
{
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_i64gather_pd(base, vindex, scale);
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_i64gather_pd(base, vindex, scale);
  }
}

template <is_simd_type B, int scale>
inline B
gather_pd_i64(double &ref, B vindex)
{
  return gather_pd_i64<B, scale>(&ref, vindex);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// masked gathers

template <is_simd_type B, int scale>
inline B
maskgather_32_i32(B src, int *base, B vindex, B mask)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_mask_i32gather_epi32(src, base, vindex, mask, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_mask_i32gather_epi32(src, base, vindex, mask, scale);
  }
}

template <is_simd_type B, int scale>
inline B
maskgather_32_i32(B src, int &ref, B vindex, B mask)
{
  return maskgather_32_i32<B, scale>(src, &ref, vindex, mask);
}

template <int scale>
inline f128
maskgather_ps_i32(f128 src, float *base, i128 vindex, f128 mask)
{
  return _mm_mask_i32gather_ps(src, base, vindex, mask, scale);
}

template <int scale>
inline f128
maskgather_ps_i32(f128 src, float &ref, i128 vindex, f128 mask)
{
  return maskgather_ps_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline f256
maskgather_ps_i32(f256 src, float *base, i256 vindex, f256 mask)
{
  return _mm256_mask_i32gather_ps(src, base, vindex, mask, scale);
}

template <int scale>
inline f256
maskgather_ps_i32(f256 src, float &ref, i256 vindex, f256 mask)
{
  return maskgather_ps_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline i128
maskgather_64_i32(i128 src, long long *base, i128 vindex, i128 mask)
{
  return _mm_mask_i32gather_epi64(src, base, vindex, mask, scale);
}

template <int scale>
inline i128
maskgather_64_i32(i128 src, long long &ref, i128 vindex, i128 mask)
{
  return maskgather_64_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline i256
maskgather_64_i32(i256 src, long long *base, i128 vindex, i256 mask)
{
  return _mm256_mask_i32gather_epi64(src, base, vindex, mask, scale);
}

template <int scale>
inline i256
maskgather_64_i32(i256 src, long long &ref, i128 vindex, i256 mask)
{
  return maskgather_64_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline d128
maskgather_pd_i32(d128 src, double *base, i128 vindex, d128 mask)
{
  return _mm_mask_i32gather_pd(src, base, vindex, mask, scale);
}

template <int scale>
inline d128
maskgather_pd_i32(d128 src, double &ref, i128 vindex, d128 mask)
{
  return maskgather_pd_i32<scale>(src, &ref, vindex, mask);
}

template <int scale>
inline d256
maskgather_pd_i32(d256 src, double *base, i128 vindex, d256 mask)
{
  return _mm256_mask_i32gather_pd(src, base, vindex, mask, scale);
}

template <int scale>
inline d256
maskgather_pd_i32(d256 src, double &ref, i128 vindex, d256 mask)
{
  return maskgather_pd_i32<scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i64(i128 src, int *base, B vindex, i128 mask)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_mask_i64gather_epi32(src, base, vindex, mask, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_mask_i64gather_epi32(src, base, vindex, mask, scale);
  }
}

template <is_simd_type B, int scale>
inline i128
maskgather_32_i64(i128 src, int &ref, B vindex, i128 mask)
{
  return maskgather_32_i64<B, scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline f128
maskgather_ps_i64(f128 src, float *base, B vindex, f128 mask)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_mask_i64gather_ps(src, base, vindex, mask, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_mask_i64gather_ps(src, base, vindex, mask, scale);
  }
}

template <is_simd_type B, int scale>
inline f128
maskgather_ps_i64(f128 src, float &ref, B vindex, f128 mask)
{
  return maskgather_ps_i64<B, scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline B
maskgather_64_i64(B src, long long *base, B vindex, B mask)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_mask_i64gather_epi64(src, base, vindex, mask, scale);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_mask_i64gather_epi64(src, base, vindex, mask, scale);
  }
}

template <is_simd_type B, int scale>
inline B
maskgather_64_i64(B src, long long &ref, B vindex, B mask)
{
  return maskgather_64_i64<B, scale>(src, &ref, vindex, mask);
}

template <is_simd_type B, int scale>
inline B
maskgather_pd_i64(B src, double *base, B vindex, B mask)
{
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_mask_i64gather_pd(src, base, vindex, mask, scale);
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_mask_i64gather_pd(src, base, vindex, mask, scale);
  }
}

template <is_simd_type B, int scale>
inline B
maskgather_pd_i64(B src, double &ref, B vindex, B mask)
{
  return maskgather_pd_i64<B, scale>(src, &ref, vindex, mask);
}

#pragma GCC diagnostic pop

};     // namespace simd
};     // namespace micron
