//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../__special/initializer_list"

#include "../../except.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tags.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../constants.hpp"
#include "../generic.hpp"
#include "../sqrt.hpp"
#include "../trig.hpp"

#include "../../control.hpp"

namespace micron
{

template <typename B, u32 D, u32 H, u32 W>
  requires(micron::is_arithmetic_v<B> && (D * H * W * sizeof(B) <= 4096) && (((D * H * W) % (64 / sizeof(B))) == 0))
class __tensor_base_avx
{
public:
  using category_type = type_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  using size_type = size_t;
  using value_type = B;
  using reference = B &;
  using ref = B &;
  using const_reference = const B &;
  using const_ref = const B &;
  using pointer = B *;
  using const_pointer = const B *;
  using iterator = B *;
  using const_iterator = const B *;

  static constexpr u32 depth = D;
  static constexpr u32 height = H;
  static constexpr u32 width = W;
  static constexpr u32 __size = D * H * W;
  static constexpr u32 __slice = H * W;

  B alignas(64) __data[__size];

  ~__tensor_base_avx() = default;

  __tensor_base_avx() { micron::czero<__size>(__data); }

  explicit __tensor_base_avx(B fill) { micron::ctypeset<__size, B>(__data, fill); }

  template <typename... Args>
    requires(sizeof...(Args) == __size)
  __tensor_base_avx(Args... args) : __data{ static_cast<B>(args)... }
  {
  }

  __tensor_base_avx(const std::initializer_list<B> &lst)
  {
    if ( lst.size() != __size )
      exc<except::library_error>("__tensor_base_avx: initializer_list size mismatch");
    micron::bytecpy(__data, lst.data(), __size * sizeof(B));
  }

  __tensor_base_avx(const __tensor_base_avx &o) { micron::cmemcpy<__size>(__data, o.__data); }

  __tensor_base_avx(__tensor_base_avx &&o)
  {
    micron::cmemcpy<__size>(__data, o.__data);
    micron::czero<__size>(o.__data);
  }

  __tensor_base_avx &
  operator=(const __tensor_base_avx &o)
  {
    micron::cmemcpy<__size>(__data, o.__data);
    return *this;
  }

  __tensor_base_avx &
  operator=(__tensor_base_avx &&o)
  {
    micron::cmemcpy<__size>(__data, o.__data);
    micron::czero<__size>(o.__data);
    return *this;
  }

  __tensor_base_avx &
  operator=(B fill)
  {
    micron::ctypeset<__size, B>(__data, fill);
    return *this;
  }

  constexpr B &
  operator[](u32 idx)
  {
    return __data[idx];
  }
  constexpr const B &
  operator[](u32 idx) const
  {
    return __data[idx];
  }

  constexpr B &
  operator[](u32 d, u32 h, u32 w)
  {
    return __data[d * __slice + h * W + w];
  }
  constexpr const B &
  operator[](u32 d, u32 h, u32 w) const
  {
    return __data[d * __slice + h * W + w];
  }

  constexpr B &
  at(u32 d, u32 h, u32 w)
  {
    return __data[d * __slice + h * W + w];
  }
  constexpr const B &
  at(u32 d, u32 h, u32 w) const
  {
    return __data[d * __slice + h * W + w];
  }

  constexpr B *
  slice(u32 d)
  {
    return &__data[d * __slice];
  }
  constexpr const B *
  slice(u32 d) const
  {
    return &__data[d * __slice];
  }

  constexpr B *
  row(u32 d, u32 h)
  {
    return &__data[d * __slice + h * W];
  }
  constexpr const B *
  row(u32 d, u32 h) const
  {
    return &__data[d * __slice + h * W];
  }

  constexpr B *
  data()
  {
    return __data;
  }
  constexpr const B *
  data() const
  {
    return __data;
  }

  constexpr iterator
  begin()
  {
    return __data;
  }
  constexpr iterator
  end()
  {
    return __data + __size;
  }
  constexpr const_iterator
  begin() const
  {
    return __data;
  }
  constexpr const_iterator
  end() const
  {
    return __data + __size;
  }
  constexpr const_iterator
  cbegin() const
  {
    return __data;
  }
  constexpr const_iterator
  cend() const
  {
    return __data + __size;
  }

  static constexpr u32
  size()
  {
    return __size;
  }
  static constexpr u32
  ndim()
  {
    return 3u;
  }
  static constexpr u32
  dim(u32 axis)
  {
    if ( axis == 0 )
      return D;
    if ( axis == 1 )
      return H;
    return W;
  }

  __tensor_base_avx &
  operator+=(B sc)
  {
    for ( size_t i = 0; i < __size; i += 8 ) {
      __data[i] += sc;
      __data[i + 1] += sc;
      __data[i + 2] += sc;
      __data[i + 3] += sc;
      __data[i + 4] += sc;
      __data[i + 5] += sc;
      __data[i + 6] += sc;
      __data[i + 7] += sc;
    }
    return *this;
  }

  __tensor_base_avx &
  operator-=(B sc)
  {
    for ( size_t i = 0; i < __size; i += 8 ) {
      __data[i] -= sc;
      __data[i + 1] -= sc;
      __data[i + 2] -= sc;
      __data[i + 3] -= sc;
      __data[i + 4] -= sc;
      __data[i + 5] -= sc;
      __data[i + 6] -= sc;
      __data[i + 7] -= sc;
    }
    return *this;
  }

  __tensor_base_avx &
  operator*=(B sc)
  {
    for ( size_t i = 0; i < __size; i += 8 ) {
      __data[i] *= sc;
      __data[i + 1] *= sc;
      __data[i + 2] *= sc;
      __data[i + 3] *= sc;
      __data[i + 4] *= sc;
      __data[i + 5] *= sc;
      __data[i + 6] *= sc;
      __data[i + 7] *= sc;
    }
    return *this;
  }

  __tensor_base_avx &
  operator/=(B sc)
  {
    for ( size_t i = 0; i < __size; i += 8 ) {
      __data[i] /= sc;
      __data[i + 1] /= sc;
      __data[i + 2] /= sc;
      __data[i + 3] /= sc;
      __data[i + 4] /= sc;
      __data[i + 5] /= sc;
      __data[i + 6] /= sc;
      __data[i + 7] /= sc;
    }
    return *this;
  }

  __tensor_base_avx
  add_scalar(B sc) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] + sc;
    return result;
  }

  __tensor_base_avx
  sub_scalar(B sc) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] - sc;
    return result;
  }

  __tensor_base_avx
  scale(B sc) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] * sc;
    return result;
  }

  __tensor_base_avx
  div_scalar(B sc) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] / sc;
    return result;
  }

  friend __tensor_base_avx
  operator+(B sc, const __tensor_base_avx &t)
  {
    return t.add_scalar(sc);
  }
  friend __tensor_base_avx
  operator-(B sc, const __tensor_base_avx &t)
  {
    return t.sub_scalar(sc);
  }
  friend __tensor_base_avx
  operator*(B sc, const __tensor_base_avx &t)
  {
    return t.scale(sc);
  }
  friend __tensor_base_avx
  operator/(B sc, const __tensor_base_avx &t)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = sc / t.__data[i];
    return result;
  }

  __tensor_base_avx
  operator+(B sc) const
  {
    return add_scalar(sc);
  }
  __tensor_base_avx
  operator-(B sc) const
  {
    return sub_scalar(sc);
  }
  __tensor_base_avx
  operator*(B sc) const
  {
    return scale(sc);
  }
  __tensor_base_avx
  operator/(B sc) const
  {
    return div_scalar(sc);
  }

  __tensor_base_avx &
  operator+=(const __tensor_base_avx &o)
  {
    for ( size_t i = 0; i < __size; ++i )
      __data[i] += o.__data[i];
    return *this;
  }

  __tensor_base_avx &
  operator-=(const __tensor_base_avx &o)
  {
    for ( size_t i = 0; i < __size; ++i )
      __data[i] -= o.__data[i];
    return *this;
  }

  __tensor_base_avx &
  operator*=(const __tensor_base_avx &o)
  {
    for ( size_t i = 0; i < __size; ++i )
      __data[i] *= o.__data[i];
    return *this;
  }

  __tensor_base_avx &
  operator/=(const __tensor_base_avx &o)
  {
    for ( size_t i = 0; i < __size; ++i )
      __data[i] /= o.__data[i];
    return *this;
  }

  __tensor_base_avx
  operator+(const __tensor_base_avx &o) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] + o.__data[i];
    return result;
  }

  __tensor_base_avx
  operator-(const __tensor_base_avx &o) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] - o.__data[i];
    return result;
  }

  __tensor_base_avx
  operator*(const __tensor_base_avx &o) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] * o.__data[i];
    return result;
  }

  __tensor_base_avx
  operator/(const __tensor_base_avx &o) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] / o.__data[i];
    return result;
  }

  __tensor_base_avx
  operator-() const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = -__data[i];
    return result;
  }

  bool
  operator==(const __tensor_base_avx &o) const
  {
    for ( size_t i = 0; i < __size; ++i )
      if ( __data[i] != o.__data[i] )
        return false;
    return true;
  }

  bool
  operator!=(const __tensor_base_avx &o) const
  {
    return !(*this == o);
  }

  bool
  almost_equal(const __tensor_base_avx &o, B eps = math::default_eps<B>()) const
    requires(micron::is_floating_point_v<B>)
  {
    for ( size_t i = 0; i < __size; ++i )
      if ( math::fabs(__data[i] - o.__data[i]) > eps )
        return false;
    return true;
  }

  __tensor_base_avx
  hadamard(const __tensor_base_avx &o) const
  {
    return *this * o;
  }

  template <u32 D2, u32 H2, u32 W2>
    requires(W == D2)
  __tensor_base_avx<B, D, H, W2>
  contract(const __tensor_base_avx<B, D2, H2, W2> &o) const
  {
    __tensor_base_avx<B, D, H, W2> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w2 = 0; w2 < W2; ++w2 ) {
          B acc = B{};
          for ( u32 k = 0; k < W; ++k )
            acc += at(d, h, k) * o.at(k, 0, w2);
          result.at(d, h, w2) = acc;
        }
    return result;
  }

  template <u32 W2>
    requires(W == H)
  __tensor_base_avx<B, D, H, W2>
  bmm(const __tensor_base_avx<B, D, W, W2> &o) const
  {
    __tensor_base_avx<B, D, H, W2> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w2 = 0; w2 < W2; ++w2 ) {
          B acc = B{};
          for ( u32 k = 0; k < W; ++k )
            acc += at(d, h, k) * o.at(d, k, w2);
          result.at(d, h, w2) = acc;
        }
    return result;
  }

  __tensor_base_avx &
  transpose_slices()
    requires(H == W)
  {
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = h + 1; w < W; ++w ) {
          B tmp = at(d, h, w);
          at(d, h, w) = at(d, w, h);
          at(d, w, h) = tmp;
        }
    return *this;
  }

  __tensor_base_avx
  transposed() const
    requires(H == W)
  {
    __tensor_base_avx result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(d, w, h) = at(d, h, w);
    return result;
  }

  __tensor_base_avx<B, D, W, H>
  transpose_hw() const
  {
    __tensor_base_avx<B, D, W, H> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(d, w, h) = at(d, h, w);
    return result;
  }

  __tensor_base_avx<B, H, D, W>
  transpose_dh() const
  {
    __tensor_base_avx<B, H, D, W> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(h, d, w) = at(d, h, w);
    return result;
  }

  __tensor_base_avx<B, W, H, D>
  transpose_dw() const
  {
    __tensor_base_avx<B, W, H, D> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(w, h, d) = at(d, h, w);
    return result;
  }

  B
  sum() const
  {
    B acc = B{};
    for ( size_t i = 0; i < __size; ++i )
      acc += __data[i];
    return acc;
  }

  B
  prod() const
  {
    B acc = B{ 1 };
    for ( size_t i = 0; i < __size; ++i )
      acc *= __data[i];
    return acc;
  }

  B
  mean() const
  {
    return sum() / static_cast<B>(__size);
  }

  B
  min_element() const
  {
    B m = __data[0];
    for ( size_t i = 1; i < __size; ++i )
      if ( __data[i] < m )
        m = __data[i];
    return m;
  }

  B
  max_element() const
  {
    B m = __data[0];
    for ( size_t i = 1; i < __size; ++i )
      if ( __data[i] > m )
        m = __data[i];
    return m;
  }

  u32
  argmin() const
  {
    u32 idx = 0;
    for ( size_t i = 1; i < __size; ++i )
      if ( __data[i] < __data[idx] )
        idx = static_cast<u32>(i);
    return idx;
  }

  u32
  argmax() const
  {
    u32 idx = 0;
    for ( size_t i = 1; i < __size; ++i )
      if ( __data[i] > __data[idx] )
        idx = static_cast<u32>(i);
    return idx;
  }

  __tensor_base_avx<B, 1, H, W>
  sum_depth() const
  {
    __tensor_base_avx<B, 1, H, W> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(0, h, w) += at(d, h, w);
    return result;
  }

  __tensor_base_avx<B, D, 1, W>
  sum_height() const
  {
    __tensor_base_avx<B, D, 1, W> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h )
        for ( u32 w = 0; w < W; ++w )
          result.at(d, 0, w) += at(d, h, w);
    return result;
  }

  __tensor_base_avx<B, D, H, 1>
  sum_width() const
  {
    __tensor_base_avx<B, D, H, 1> result;
    for ( u32 d = 0; d < D; ++d )
      for ( u32 h = 0; h < H; ++h ) {
        B acc = B{};
        for ( u32 w = 0; w < W; ++w )
          acc += at(d, h, w);
        result.at(d, h, 0) = acc;
      }
    return result;
  }

  B
  l1_norm() const
    requires(micron::is_floating_point_v<B>)
  {
    B acc = B{};
    for ( size_t i = 0; i < __size; ++i )
      acc += math::fabs(__data[i]);
    return acc;
  }

  B
  squared_norm() const
  {
    B acc = B{};
    for ( size_t i = 0; i < __size; ++i )
      acc += __data[i] * __data[i];
    return acc;
  }

  B
  frobenius_norm() const
    requires(micron::is_floating_point_v<B>)
  {
    return math::fsqrt(squared_norm());
  }

  B
  linf_norm() const
    requires(micron::is_floating_point_v<B>)
  {
    B m = math::fabs(__data[0]);
    for ( size_t i = 1; i < __size; ++i ) {
      B v = math::fabs(__data[i]);
      if ( v > m )
        m = v;
    }
    return m;
  }

#define __tensor_unop(name, fn)                                                                                                            \
  __tensor_base_avx name() const                                                                                                           \
    requires(micron::is_floating_point_v<B>)                                                                                               \
  {                                                                                                                                        \
    __tensor_base_avx result;                                                                                                              \
    for ( size_t i = 0; i < __size; ++i )                                                                                                  \
      result.__data[i] = fn(__data[i]);                                                                                                    \
    return result;                                                                                                                         \
  }

  __tensor_unop(abs, math::fabs) __tensor_unop(floor, math::ffloor) __tensor_unop(ceil, math::fceil) __tensor_unop(round, math::fround)
      __tensor_unop(trunc, math::ftrunc) __tensor_unop(frac, math::ffract) __tensor_unop(sqrt, math::fsqrt)
          __tensor_unop(rsqrt, math::frsqrt) __tensor_unop(exp, math::fexp) __tensor_unop(exp2, math::fexp2) __tensor_unop(log, math::flog)
              __tensor_unop(log2, math::flog2) __tensor_unop(log10, math::flog10) __tensor_unop(sin, math::fsin)
                  __tensor_unop(cos, math::fcos) __tensor_unop(tan, math::ftan) __tensor_unop(asin, math::fasin)
                      __tensor_unop(acos, math::facos) __tensor_unop(atan, math::fatan) __tensor_unop(sinh, math::fsinh)
                          __tensor_unop(cosh, math::fcosh) __tensor_unop(tanh, math::ftanh) __tensor_unop(erf, math::ferf)
                              __tensor_unop(erfc, math::ferfc) __tensor_unop(gamma, math::fgamma)
#undef __tensor_unop

                                  __tensor_base_avx saturate() const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::fclamp(__data[i], B{ 0 }, B{ 1 });
    return result;
  }

  __tensor_base_avx
  clamp(B lo, B hi) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::fclamp(__data[i], lo, hi);
    return result;
  }

  __tensor_base_avx
  sign() const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = static_cast<B>((__data[i] > B{}) - (__data[i] < B{}));
    return result;
  }

#define __tensor_binop(name, fn)                                                                                                           \
  __tensor_base_avx name(const __tensor_base_avx &o) const                                                                                 \
    requires(micron::is_floating_point_v<B>)                                                                                               \
  {                                                                                                                                        \
    __tensor_base_avx result;                                                                                                              \
    for ( size_t i = 0; i < __size; ++i )                                                                                                  \
      result.__data[i] = fn(__data[i], o.__data[i]);                                                                                       \
    return result;                                                                                                                         \
  }

  __tensor_binop(min, math::fmin) __tensor_binop(max, math::fmax) __tensor_binop(fmin, math::fmin) __tensor_binop(fmax, math::fmax)
      __tensor_binop(pow, math::fpow) __tensor_binop(atan2, math::fatan2)
#undef __tensor_binop

          __tensor_base_avx pow(B s) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::fpow(__data[i], s);
    return result;
  }

  __tensor_base_avx
  fma(const __tensor_base_avx &v, const __tensor_base_avx &w) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::ffma(__data[i], v.__data[i], w.__data[i]);
    return result;
  }

  __tensor_base_avx
  fma(B s, const __tensor_base_avx &w) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::ffma(__data[i], s, w.__data[i]);
    return result;
  }

  __tensor_base_avx
  mul_add(const __tensor_base_avx &v, const __tensor_base_avx &w) const
  {
    return fma(v, w);
  }
  __tensor_base_avx
  mul_add(B s, const __tensor_base_avx &w) const
  {
    return fma(s, w);
  }

  __tensor_base_avx
  lerp(const __tensor_base_avx &other, B t) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = math::flerp(__data[i], other.__data[i], t);
    return result;
  }

  bool
  all() const
  {
    for ( size_t i = 0; i < __size; ++i )
      if ( __data[i] == B{} )
        return false;
    return true;
  }

  bool
  any() const
  {
    for ( size_t i = 0; i < __size; ++i )
      if ( __data[i] != B{} )
        return true;
    return false;
  }

  bool
  none() const
  {
    return !any();
  }

  __tensor_base_avx &
  fill(B val)
  {
    micron::ctypeset<__size, B>(__data, val);
    return *this;
  }

  __tensor_base_avx &
  zero()
  {
    micron::czero<__size>(__data);
    return *this;
  }

  __tensor_base_avx &
  iota(B start = B{}, B step = B{ 1 })
  {
    B val = start;
    for ( size_t i = 0; i < __size; ++i, val += step )
      __data[i] = val;
    return *this;
  }

  __tensor_base_avx
  softmax() const
    requires(micron::is_floating_point_v<B>)
  {

    B m = max_element();
    __tensor_base_avx shifted;
    for ( size_t i = 0; i < __size; ++i )
      shifted.__data[i] = math::fexp(__data[i] - m);
    B s = shifted.sum();
    return shifted / s;
  }

  __tensor_base_avx
  relu() const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] > B{} ? __data[i] : B{};
    return result;
  }

  __tensor_base_avx
  leaky_relu(B alpha) const
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = __data[i] > B{} ? __data[i] : alpha * __data[i];
    return result;
  }

  __tensor_base_avx
  sigmoid() const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( size_t i = 0; i < __size; ++i )
      result.__data[i] = B{ 1 } / (B{ 1 } + math::fexp(-__data[i]));
    return result;
  }

  __tensor_base_avx
  layer_norm(B eps = math::default_eps<B>()) const
    requires(micron::is_floating_point_v<B>)
  {
    __tensor_base_avx result;
    for ( u32 d = 0; d < D; ++d ) {
      for ( u32 h = 0; h < H; ++h ) {

        B mu = B{};
        for ( u32 w = 0; w < W; ++w )
          mu += at(d, h, w);
        mu /= static_cast<B>(W);

        B var = B{};
        for ( u32 w = 0; w < W; ++w ) {
          B diff = at(d, h, w) - mu;
          var += diff * diff;
        }
        var /= static_cast<B>(W);
        B inv_std = math::frsqrt(var + eps);
        for ( u32 w = 0; w < W; ++w )
          result.at(d, h, w) = (at(d, h, w) - mu) * inv_std;
      }
    }
    return result;
  }
};

template <u32 D, u32 H, u32 W> using tensor3f = __tensor_base_avx<f32, D, H, W>;
template <u32 D, u32 H, u32 W> using tensor3d = __tensor_base_avx<f64, D, H, W>;
template <u32 D, u32 H, u32 W> using tensor3ld = __tensor_base_avx<f128, D, H, W>;

template <u32 D, u32 H, u32 W> using tensor3i = __tensor_base_avx<i32, D, H, W>;
template <u32 D, u32 H, u32 W> using tensor3u = __tensor_base_avx<u32, D, H, W>;
template <u32 D, u32 H, u32 W> using tensor3i8 = __tensor_base_avx<i8, D, H, W>;
template <u32 D, u32 H, u32 W> using tensor3u8 = __tensor_base_avx<u8, D, H, W>;

}
