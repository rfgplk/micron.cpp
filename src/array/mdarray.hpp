//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../simd/aliases.hpp"
#include "../simd/types.hpp"
#include "../types.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/new.hpp"

namespace micron
{

// a generic-rank dense multi-dimensional array with row-major layout
// ranks 1..N are stored as a contiguous buffer plus a stride table
// on AVX2 / NEON, the elementwise operations map to SIMD kernels for fundamental Ts
template<typename T, usize __R>
  requires(__R >= 1 and __R <= 8)
class mdarray
{
  T *__data = nullptr;
  usize __shape[__R] = { 0 };
  usize __strides[__R] = { 0 };
  usize __total = 0;

  void
  __compute_strides() noexcept
  {
    if ( __R == 0 ) return;
    __strides[__R - 1] = 1;
    for ( usize i = __R - 1; i-- > 0; ) __strides[i] = __strides[i + 1] * __shape[i + 1];
  }

  void
  __alloc(usize total)
  {
    __data = static_cast<T *>(::operator new(sizeof(T) * total));
    __total = total;
    if constexpr ( !micron::is_trivially_constructible_v<T> ) {
      for ( usize i = 0; i < total; ++i ) new (__data + i) T();
    } else {
      micron::memset(reinterpret_cast<byte *>(__data), 0u, total * sizeof(T));
    }
  }

  void
  __free() noexcept
  {
    if ( !__data ) return;
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = 0; i < __total; ++i ) __data[i].~T();
    }
    ::operator delete(__data);
    __data = nullptr;
    __total = 0;
  }

  // TODO: eventually pull all of these out to a separate simd_kernels file
  void
  __fill_simd(T v) noexcept
  {
#if defined(__micron_x86_avx2)
    if constexpr ( micron::is_same_v<T, f32> ) {
      __m256 vv = _mm256_set1_ps(v);
      usize i = 0;
      for ( ; i + 8 <= __total; i += 8 ) {
        _mm256_storeu_ps(reinterpret_cast<float *>(__data + i), vv);
      }
      for ( ; i < __total; ++i ) __data[i] = v;
      return;
    }
    if constexpr ( micron::is_same_v<T, f64> ) {
      __m256d vv = _mm256_set1_pd(v);
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) {
        _mm256_storeu_pd(reinterpret_cast<double *>(__data + i), vv);
      }
      for ( ; i < __total; ++i ) __data[i] = v;
      return;
    }
#elif defined(__micron_arm_neon)
    if constexpr ( micron::is_same_v<T, f32> ) {
      float32x4_t vv = vdupq_n_f32(v);
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) vst1q_f32(reinterpret_cast<float *>(__data + i), vv);
      for ( ; i < __total; ++i ) __data[i] = v;
      return;
    }
#endif
    for ( usize i = 0; i < __total; ++i ) __data[i] = v;
  }

  void
  __add_simd(const T *o) noexcept
  {
#if defined(__micron_x86_avx2)
    if constexpr ( micron::is_same_v<T, f32> ) {
      usize i = 0;
      for ( ; i + 8 <= __total; i += 8 ) {
        __m256 a = _mm256_loadu_ps(reinterpret_cast<const float *>(__data + i));
        __m256 b = _mm256_loadu_ps(reinterpret_cast<const float *>(o + i));
        _mm256_storeu_ps(reinterpret_cast<float *>(__data + i), _mm256_add_ps(a, b));
      }
      for ( ; i < __total; ++i ) __data[i] += o[i];
      return;
    }
    if constexpr ( micron::is_same_v<T, f64> ) {
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) {
        __m256d a = _mm256_loadu_pd(reinterpret_cast<const double *>(__data + i));
        __m256d b = _mm256_loadu_pd(reinterpret_cast<const double *>(o + i));
        _mm256_storeu_pd(reinterpret_cast<double *>(__data + i), _mm256_add_pd(a, b));
      }
      for ( ; i < __total; ++i ) __data[i] += o[i];
      return;
    }
#elif defined(__micron_arm_neon)
    if constexpr ( micron::is_same_v<T, f32> ) {
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) {
        float32x4_t a = vld1q_f32(reinterpret_cast<const float *>(__data + i));
        float32x4_t b = vld1q_f32(reinterpret_cast<const float *>(o + i));
        vst1q_f32(reinterpret_cast<float *>(__data + i), vaddq_f32(a, b));
      }
      for ( ; i < __total; ++i ) __data[i] += o[i];
      return;
    }
#endif
    for ( usize i = 0; i < __total; ++i ) __data[i] += o[i];
  }

  void
  __sub_simd(const T *o) noexcept
  {
#if defined(__micron_x86_avx2)
    if constexpr ( micron::is_same_v<T, f32> ) {
      usize i = 0;
      for ( ; i + 8 <= __total; i += 8 ) {
        __m256 a = _mm256_loadu_ps(reinterpret_cast<const float *>(__data + i));
        __m256 b = _mm256_loadu_ps(reinterpret_cast<const float *>(o + i));
        _mm256_storeu_ps(reinterpret_cast<float *>(__data + i), _mm256_sub_ps(a, b));
      }
      for ( ; i < __total; ++i ) __data[i] -= o[i];
      return;
    }
#elif defined(__micron_arm_neon)
    if constexpr ( micron::is_same_v<T, f32> ) {
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) {
        float32x4_t a = vld1q_f32(reinterpret_cast<const float *>(__data + i));
        float32x4_t b = vld1q_f32(reinterpret_cast<const float *>(o + i));
        vst1q_f32(reinterpret_cast<float *>(__data + i), vsubq_f32(a, b));
      }
      for ( ; i < __total; ++i ) __data[i] -= o[i];
      return;
    }
#endif
    for ( usize i = 0; i < __total; ++i ) __data[i] -= o[i];
  }

  void
  __mul_scalar_simd(T s) noexcept
  {
#if defined(__micron_x86_avx2)
    if constexpr ( micron::is_same_v<T, f32> ) {
      __m256 vs = _mm256_set1_ps(s);
      usize i = 0;
      for ( ; i + 8 <= __total; i += 8 ) {
        __m256 a = _mm256_loadu_ps(reinterpret_cast<const float *>(__data + i));
        _mm256_storeu_ps(reinterpret_cast<float *>(__data + i), _mm256_mul_ps(a, vs));
      }
      for ( ; i < __total; ++i ) __data[i] *= s;
      return;
    }
#elif defined(__micron_arm_neon)
    if constexpr ( micron::is_same_v<T, f32> ) {
      float32x4_t vs = vdupq_n_f32(s);
      usize i = 0;
      for ( ; i + 4 <= __total; i += 4 ) {
        float32x4_t a = vld1q_f32(reinterpret_cast<const float *>(__data + i));
        vst1q_f32(reinterpret_cast<float *>(__data + i), vmulq_f32(a, vs));
      }
      for ( ; i < __total; ++i ) __data[i] *= s;
      return;
    }
#endif
    for ( usize i = 0; i < __total; ++i ) __data[i] *= s;
  }

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using value_type = T;
  using size_type = usize;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = T *;
  using const_iterator = const T *;

  static constexpr usize rank = __R;

  ~mdarray() { __free(); }

  mdarray() = default;

  template<typename... Dims>
    requires(sizeof...(Dims) == __R)
  explicit mdarray(Dims... dims)
  {
    usize tmp[__R] = { static_cast<usize>(dims)... };
    usize total = 1;
    for ( usize i = 0; i < __R; ++i ) {
      __shape[i] = tmp[i];
      total *= tmp[i];
    }
    if ( total == 0 ) [[unlikely]]
      exc<except::library_error>("mdarray: zero-sized dimension");
    __compute_strides();
    __alloc(total);
  }

  mdarray(const mdarray &o)
  {
    if ( !o.__data ) return;
    for ( usize i = 0; i < __R; ++i ) __shape[i] = o.__shape[i];
    for ( usize i = 0; i < __R; ++i ) __strides[i] = o.__strides[i];
    __alloc(o.__total);
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(reinterpret_cast<byte *>(__data), reinterpret_cast<byte *>(o.__data), o.__total * sizeof(T));
    } else {
      for ( usize i = 0; i < o.__total; ++i ) __data[i] = o.__data[i];
    }
  }

  mdarray(mdarray &&o) noexcept : __data(o.__data), __total(o.__total)
  {
    for ( usize i = 0; i < __R; ++i ) __shape[i] = o.__shape[i];
    for ( usize i = 0; i < __R; ++i ) __strides[i] = o.__strides[i];
    o.__data = nullptr;
    o.__total = 0;
    for ( usize i = 0; i < __R; ++i ) {
      o.__shape[i] = 0;
      o.__strides[i] = 0;
    }
  }

  mdarray &
  operator=(const mdarray &o)
  {
    if ( this == &o ) return *this;
    __free();
    if ( !o.__data ) return *this;
    for ( usize i = 0; i < __R; ++i ) __shape[i] = o.__shape[i];
    for ( usize i = 0; i < __R; ++i ) __strides[i] = o.__strides[i];
    __alloc(o.__total);
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(reinterpret_cast<byte *>(__data), reinterpret_cast<byte *>(o.__data), o.__total * sizeof(T));
    } else {
      for ( usize i = 0; i < o.__total; ++i ) __data[i] = o.__data[i];
    }
    return *this;
  }

  mdarray &
  operator=(mdarray &&o) noexcept
  {
    if ( this == &o ) return *this;
    __free();
    __data = o.__data;
    __total = o.__total;
    for ( usize i = 0; i < __R; ++i ) __shape[i] = o.__shape[i];
    for ( usize i = 0; i < __R; ++i ) __strides[i] = o.__strides[i];
    o.__data = nullptr;
    o.__total = 0;
    for ( usize i = 0; i < __R; ++i ) {
      o.__shape[i] = 0;
      o.__strides[i] = 0;
    }
    return *this;
  }

  usize
  size() const noexcept
  {
    return __total;
  }

  usize
  shape(usize axis) const noexcept
  {
    return __shape[axis];
  }

  usize
  stride(usize axis) const noexcept
  {
    return __strides[axis];
  }

  T *
  data() noexcept
  {
    return __data;
  }

  const T *
  data() const noexcept
  {
    return __data;
  }

  iterator
  begin() noexcept
  {
    return __data;
  }

  iterator
  end() noexcept
  {
    return __data + __total;
  }

  const_iterator
  begin() const noexcept
  {
    return __data;
  }

  const_iterator
  end() const noexcept
  {
    return __data + __total;
  }

  template<typename... Idx>
    requires(sizeof...(Idx) == __R)
  T &
  operator()(Idx... ix)
  {
    usize ind[__R] = { static_cast<usize>(ix)... };
    usize off = 0;
    for ( usize i = 0; i < __R; ++i ) off += ind[i] * __strides[i];
    return __data[off];
  }

  template<typename... Idx>
    requires(sizeof...(Idx) == __R)
  const T &
  operator()(Idx... ix) const
  {
    usize ind[__R] = { static_cast<usize>(ix)... };
    usize off = 0;
    for ( usize i = 0; i < __R; ++i ) off += ind[i] * __strides[i];
    return __data[off];
  }

  void
  fill(T v)
  {
    if constexpr ( micron::is_arithmetic_v<T> ) {
      __fill_simd(v);
    } else {
      for ( usize i = 0; i < __total; ++i ) __data[i] = v;
    }
  }

  mdarray &
  operator+=(const mdarray &o)
  {
    if constexpr ( micron::is_arithmetic_v<T> ) {
      __add_simd(o.__data);
    } else {
      for ( usize i = 0; i < __total; ++i ) __data[i] += o.__data[i];
    }
    return *this;
  }

  mdarray &
  operator-=(const mdarray &o)
  {
    if constexpr ( micron::is_arithmetic_v<T> ) {
      __sub_simd(o.__data);
    } else {
      for ( usize i = 0; i < __total; ++i ) __data[i] -= o.__data[i];
    }
    return *this;
  }

  mdarray &
  operator*=(T scalar)
  {
    if constexpr ( micron::is_arithmetic_v<T> ) {
      __mul_scalar_simd(scalar);
    } else {
      for ( usize i = 0; i < __total; ++i ) __data[i] *= scalar;
    }
    return *this;
  }

  T
  sum() const
  {
    T acc{};
    for ( usize i = 0; i < __total; ++i ) acc += __data[i];
    return acc;
  }

  const_iterator
  cbegin() const noexcept
  {
    return __data;
  }

  const_iterator
  cend() const noexcept
  {
    return __data + __total;
  }

  usize
  max_size() const noexcept
  {
    return __total;
  }

  T &
  at(usize flat_idx)
  {
    if ( flat_idx >= __total ) [[unlikely]]
      exc<except::library_error>("mdarray::at: out of range");
    return __data[flat_idx];
  }

  const T &
  at(usize flat_idx) const
  {
    if ( flat_idx >= __total ) [[unlikely]]
      exc<except::library_error>("mdarray::at: out of range");
    return __data[flat_idx];
  }

  void
  swap(mdarray &o) noexcept
  {
    micron::swap(__data, o.__data);
    micron::swap(__total, o.__total);
    for ( usize i = 0; i < __R; ++i ) {
      micron::swap(__shape[i], o.__shape[i]);
      micron::swap(__strides[i], o.__strides[i]);
    }
  }

  void
  clear() noexcept
  {
    if constexpr ( micron::is_arithmetic_v<T> ) {
      __fill_simd(T{});
    } else {
      for ( usize i = 0; i < __total; ++i ) __data[i] = T{};
    }
  }
};

};      // namespace micron
