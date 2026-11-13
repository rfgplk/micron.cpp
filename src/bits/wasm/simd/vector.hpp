// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../__special/initializer_list"
#include "types.hpp"

namespace micron::simd
{
template<is_simd_type B, is_flag_type F> class __wasm_vector
{
  B __value{};
  static constexpr usize __count = sizeof(B) / sizeof(F);
  using __lanes = __wasm_lanes<F, __count>;

  template<unsigned Op>
  static constexpr F
  __scalar(F a, F b) noexcept
  {
    if constexpr ( micron::is_integral_v<F> && Op < 3 ) {
      using U = micron::make_unsigned_t<F>;
      if constexpr ( Op == 0 ) return F(U(a) + U(b));
      if constexpr ( Op == 1 ) return F(U(a) - U(b));
      if constexpr ( Op == 2 ) return F(u64(U(a)) * u64(U(b)));
    } else {
      if constexpr ( Op == 0 ) return a + b;
      if constexpr ( Op == 1 ) return a - b;
      if constexpr ( Op == 2 ) return a * b;
      if constexpr ( Op == 3 ) return a / b;
    }
  }

  template<unsigned Op>
  constexpr __wasm_vector
  __arith(const __wasm_vector &b) const noexcept
  {
    auto a_lanes = __wasm_unpack<F>(__value);
    const auto b_lanes = __wasm_unpack<F>(b.__value);
    for ( usize i = 0; i < __count; ++i ) a_lanes.lane[i] = __scalar<Op>(a_lanes.lane[i], b_lanes.lane[i]);
    return __wasm_pack<B>(a_lanes);
  }

  template<unsigned Op>
  constexpr __wasm_vector
  __bitwise(const __wasm_vector &b) const noexcept
  {
    auto a_lanes = __wasm_unpack<u64>(__value);
    const auto b_lanes = __wasm_unpack<u64>(b.__value);
    for ( usize i = 0; i < sizeof(B) / 8; ++i ) {
      if constexpr ( Op == 0 ) a_lanes.lane[i] &= b_lanes.lane[i];
      if constexpr ( Op == 1 ) a_lanes.lane[i] |= b_lanes.lane[i];
      if constexpr ( Op == 2 ) a_lanes.lane[i] ^= b_lanes.lane[i];
    }
    return __wasm_pack<B>(a_lanes);
  }

  template<unsigned Op>
  constexpr u64
  __compare(const __wasm_vector &b) const noexcept
  {
    const auto a_lanes = __wasm_unpack<F>(__value), b_lanes = __wasm_unpack<F>(b.__value);
    constexpr usize step = micron::is_integral_v<F> ? sizeof(F) : 1;
    constexpr u64 lane_mask = (u64(1) << step) - 1;
    u64 mask = 0;
    for ( usize i = 0; i < __count; ++i ) {
      const F a = a_lanes.lane[i], b = b_lanes.lane[i];
      bool yes;
      if constexpr ( Op == 0 ) yes = a == b;
      if constexpr ( Op == 1 ) yes = a != b;
      if constexpr ( Op == 2 ) yes = a < b;
      if constexpr ( Op == 3 ) yes = a <= b;
      if constexpr ( Op == 4 ) yes = a > b;
      if constexpr ( Op == 5 ) yes = a >= b;
      if ( yes ) mask |= lane_mask << (i * step);
    }
    return mask;
  }

public:
  using bit_width = B;
  using lane_width = F;
  constexpr __wasm_vector() noexcept = default;

  constexpr __wasm_vector(B value) noexcept : __value(value) { }

  constexpr __wasm_vector(F value) noexcept : __value(__wasm_splat<B>(value)) { }

  constexpr __wasm_vector(std::initializer_list<F> values) noexcept
  {
    if ( values.size() != __count ) return;
    __lanes lanes{};
    usize i = 0;
    for ( F value : values ) lanes.lane[i++] = value;
    __value = __wasm_pack<B>(lanes);
  }

  template<typename... A>
    requires(sizeof...(A) == __count && sizeof...(A) > 1)
  constexpr __wasm_vector(A... values) noexcept : __value(__wasm_pack<B>(__lanes{ { F(values)... } }))
  {
  }

  constexpr
  operator B() const noexcept
  {
    return __value;
  }

  constexpr __wasm_vector &
  operator=(F value) noexcept
  {
    set(value);
    return *this;
  }

  __wasm_vector &
  operator=(const F *p) noexcept
  {
    return uload(p);
  }

  constexpr __wasm_vector &
  operator=(std::initializer_list<F> values) noexcept
  {
    return *this = __wasm_vector(values);
  }

  static constexpr usize
  size() noexcept
  {
    return __count;
  }

  constexpr F
  operator[](usize i) const noexcept
  {
    return __wasm_unpack<F>(__value).lane[i];
  }

  constexpr void
  set(F value) noexcept
  {
    __value = __wasm_splat<B>(value);
  }

  void
  set(const F *p) noexcept
  {
    set(*p);
  }

  void
  broadcast(const F *p) noexcept
  {
    set(*p);
  }

  template<typename T>
  __wasm_vector &
  load(const T *p) noexcept
  {
    __builtin_memcpy(&__value, p, sizeof(B));
    return *this;
  }

  template<typename T>
  __wasm_vector &
  uload(const T *p) noexcept
  {
    return load(p);
  }

  void
  get(F *p) const noexcept
  {
    __builtin_memcpy(p, &__value, sizeof(B));
  }

  void
  get_aligned(F *p) const noexcept
  {
    get(p);
  }

  constexpr void
  to_zero() noexcept
  {
    __value = B{};
  }

  constexpr void
  to_ones() noexcept
  {
    __value = __wasm_splat<B>(~u64(0));
  }

  constexpr bool
  all_zeroes() const noexcept
  {
    for ( auto x : __wasm_unpack<u64>(__value).lane )
      if ( x ) return false;
    return true;
  }

  constexpr bool
  all_ones() const noexcept
  {
    for ( auto x : __wasm_unpack<u64>(__value).lane )
      if ( x != ~u64(0) ) return false;
    return true;
  }

  constexpr u64
  operator==(const __wasm_vector &b) const noexcept
  {
    return __compare<0>(b);
  }

  constexpr u64
  operator!=(const __wasm_vector &b) const noexcept
  {
    return __compare<1>(b);
  }

  constexpr u64
  operator<(const __wasm_vector &b) const noexcept
  {
    return __compare<2>(b);
  }

  constexpr u64
  operator<=(const __wasm_vector &b) const noexcept
  {
    return __compare<3>(b);
  }

  constexpr u64
  operator>(const __wasm_vector &b) const noexcept
  {
    return __compare<4>(b);
  }

  constexpr u64
  operator>=(const __wasm_vector &b) const noexcept
  {
    return __compare<5>(b);
  }

  constexpr __wasm_vector
  operator+(const __wasm_vector &b) const noexcept
  {
    return __arith<0>(b);
  }

  constexpr __wasm_vector
  operator-(const __wasm_vector &b) const noexcept
  {
    return __arith<1>(b);
  }

  constexpr __wasm_vector
  operator*(const __wasm_vector &b) const noexcept
  {
    return __arith<2>(b);
  }

  constexpr __wasm_vector
  operator/(const __wasm_vector &b) const noexcept
  {
    return __arith<3>(b);
  }

  constexpr __wasm_vector
  operator&(const __wasm_vector &b) const noexcept
  {
    return __bitwise<0>(b);
  }

  constexpr __wasm_vector
  operator|(const __wasm_vector &b) const noexcept
  {
    return __bitwise<1>(b);
  }

  constexpr __wasm_vector
  operator^(const __wasm_vector &b) const noexcept
  {
    return __bitwise<2>(b);
  }

  constexpr __wasm_vector
  operator~() const noexcept
  {
    auto ones = *this;
    ones.to_ones();
    return *this ^ ones;
  }

  constexpr __wasm_vector
  andnot(const __wasm_vector &b) const noexcept
  {
    return ~*this & b;
  }

  constexpr __wasm_vector &
  operator+=(const __wasm_vector &b) noexcept
  {
    return *this = *this + b;
  }

  constexpr __wasm_vector &
  operator-=(const __wasm_vector &b) noexcept
  {
    return *this = *this - b;
  }

  constexpr __wasm_vector &
  operator*=(const __wasm_vector &b) noexcept
  {
    return *this = *this * b;
  }

  constexpr __wasm_vector &
  operator/=(const __wasm_vector &b) noexcept
  {
    return *this = *this / b;
  }

  constexpr __wasm_vector &
  operator&=(const __wasm_vector &b) noexcept
  {
    return *this = *this & b;
  }

  constexpr __wasm_vector &
  operator|=(const __wasm_vector &b) noexcept
  {
    return *this = *this | b;
  }

  constexpr __wasm_vector &
  operator^=(const __wasm_vector &b) noexcept
  {
    return *this = *this ^ b;
  }

  constexpr __wasm_vector
  operator<<(int count) const noexcept
    requires micron::is_integral_v<F>
  {
    using U = micron::make_unsigned_t<F>;
    auto lanes = __wasm_unpack<F>(__value);
    for ( auto &x : lanes.lane ) x = unsigned(count) >= sizeof(F) * 8 ? F(0) : F(u64(U(x)) << count);
    return __wasm_pack<B>(lanes);
  }

  constexpr __wasm_vector
  operator>>(int count) const noexcept
    requires micron::is_integral_v<F>
  {
    using U = micron::make_unsigned_t<F>;
    constexpr unsigned width = sizeof(F) * 8;
    auto lanes = __wasm_unpack<F>(__value);
    for ( auto &x : lanes.lane ) {
      const bool negative = micron::is_signed_v<F> && x < 0;
      if ( unsigned(count) >= width )
        x = negative ? F(-1) : F(0);
      else if ( count ) {
        U u = U(x) >> count;
        if ( negative ) u |= U(~u64(0) << (width - count));
        x = F(u);
      }
    }
    return __wasm_pack<B>(lanes);
  }

  constexpr __wasm_vector &
  operator<<=(int n) noexcept
    requires micron::is_integral_v<F>
  {
    return *this = *this << n;
  }

  constexpr __wasm_vector &
  operator>>=(int n) noexcept
    requires micron::is_integral_v<F>
  {
    return *this = *this >> n;
  }

  constexpr u64
  movemask() const noexcept
  {
    const auto bytes = __wasm_unpack<u8>(__value);
    u64 mask = 0;
    constexpr usize step = micron::is_integral_v<F> ? 1 : sizeof(F);
    for ( usize i = 0; i < sizeof(B) / step; ++i ) mask |= u64(bytes.lane[(i + 1) * step - 1] >> 7) << i;
    return mask;
  }

  constexpr u8
  byte0() const noexcept
  {
    return __wasm_unpack<u8>(__value).lane[0];
  }

  constexpr __wasm_vector
  shuffle(const __wasm_vector &control) const noexcept
  {
    const auto in = __wasm_unpack<u8>(__value), c = __wasm_unpack<u8>(control.__value);
    __wasm_lanes<u8, sizeof(B)> out{};
    for ( usize i = 0; i < sizeof(B); ++i ) out.lane[i] = c.lane[i] & 128 ? 0 : in.lane[(i & ~usize(15)) + (c.lane[i] & 15)];
    return __wasm_pack<B>(out);
  }

  template<int N>
  constexpr __wasm_vector
  byte_shift_left() const noexcept
  {
    static_assert(N >= 0);
    const auto in = __wasm_unpack<u8>(__value);
    __wasm_lanes<u8, sizeof(B)> out{};
    if constexpr ( N < 16 )
      for ( usize i = 0; i < sizeof(B); ++i )
        if ( i % 16 >= N ) out.lane[i] = in.lane[i - N];
    return __wasm_pack<B>(out);
  }

  template<int N>
  constexpr __wasm_vector
  byte_shift_right() const noexcept
  {
    static_assert(N >= 0);
    const auto in = __wasm_unpack<u8>(__value);
    __wasm_lanes<u8, sizeof(B)> out{};
    if constexpr ( N < 16 )
      for ( usize i = 0; i < sizeof(B); ++i )
        if ( i % 16 + N < 16 ) out.lane[i] = in.lane[i + N];
    return __wasm_pack<B>(out);
  }

  template<int N>
  constexpr __wasm_vector
  alignr(const __wasm_vector &other) const noexcept
  {
    static_assert(N >= 0);
    const auto hi = __wasm_unpack<u8>(__value), lo = __wasm_unpack<u8>(other.__value);
    __wasm_lanes<u8, sizeof(B)> out{};
    for ( usize i = 0; i < sizeof(B); ++i ) {
      const usize j = i % 16 + N, block = i & ~usize(15);
      out.lane[i] = j < 16 ? lo.lane[block + j] : j < 32 ? hi.lane[block + j - 16] : 0;
    }
    return __wasm_pack<B>(out);
  }
};

template<is_simd_128_type B, is_flag_type F> using v128 = __wasm_vector<B, F>;
template<is_simd_256_type B, is_flag_type F> using v256 = __wasm_vector<B, F>;
template<is_simd_512_type B, is_flag_type F> using v512 = __wasm_vector<B, F>;
};      // namespace micron::simd
