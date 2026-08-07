//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../allocator.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// limb buffer
//
//  a) dynamic, heap, unbounded -> store<0, Alloc>     (micron::vector<limb_t, Alloc, false>)
//  b) bounded, stack/inline    -> store<Bits, Alloc>  (limb_t d[limbs_for(Bits)])

namespace micron
{
namespace math
{
namespace __arb
{

template<usize Bytes> inline constexpr usize store_align_v = Bytes >= 256u ? 64u : (Bytes >= 64u ? 32u : alignof(mpn::limb_t));

// %%%%%%%%%%%%%%%
// bounded

template<usize Bits, class Alloc> struct store {
  static constexpr bool bounded = true;
  static constexpr usize width_bits = Bits;
  static constexpr usize cap_limbs = mpn::limbs_of<Bits>;

  static_assert(cap_limbs >= 1, "arbint: a bounded width must hold at least one limb");

  static constexpr usize excess_bits = cap_limbs * mpn::limb_bits - Bits;
  static constexpr mpn::limb_t top_mask = excess_bits == 0 ? mpn::limb_max : static_cast<mpn::limb_t>(mpn::limb_max >> excess_bits);

  alignas(store_align_v<cap_limbs * sizeof(mpn::limb_t)>) mpn::limb_t d[cap_limbs];
  u32 n;

  constexpr store() noexcept : d{}, n(0) { }

  constexpr store(const store &) noexcept = default;
  constexpr store(store &&) noexcept = default;
  constexpr store &operator=(const store &) noexcept = default;
  constexpr store &operator=(store &&) noexcept = default;
  ~store() noexcept = default;

  [[nodiscard, gnu::always_inline]] constexpr mpn::limb_t *
  limbs() noexcept
  {
    return d;
  }

  [[nodiscard, gnu::always_inline]] constexpr const mpn::limb_t *
  limbs() const noexcept
  {
    return d;
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  size() const noexcept
  {
    return static_cast<usize>(n);
  }

  [[gnu::always_inline]] constexpr void
  set_size(usize k) noexcept
  {
    n = static_cast<u32>(k <= cap_limbs ? k : cap_limbs);
  }

  [[nodiscard, gnu::always_inline]] constexpr usize
  capacity() const noexcept
  {
    return cap_limbs;
  }

  [[nodiscard, gnu::always_inline]] constexpr bool
  ensure(usize k) noexcept
  {
    return k <= cap_limbs;
  }

  [[gnu::always_inline]] constexpr void
  finish(usize k) noexcept
  {
    if ( k > cap_limbs ) k = cap_limbs;
    if constexpr ( excess_bits != 0 ) {
      if ( k == cap_limbs ) d[cap_limbs - 1] &= top_mask;
    }
    n = static_cast<u32>(mpn::normalize(d, k));
  }

  [[gnu::always_inline]] constexpr void
  swap_with(store &o) noexcept
  {
    for ( usize i = 0; i < cap_limbs; ++i ) {
      const mpn::limb_t t = d[i];
      d[i] = o.d[i];
      o.d[i] = t;
    }
    const u32 tn = n;
    n = o.n;
    o.n = tn;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// heap, unbounded
template<class Alloc> struct store<0, Alloc> {
  static constexpr bool bounded = false;
  static constexpr usize width_bits = 0;
  static constexpr usize cap_limbs = 0;
  static constexpr usize excess_bits = 0;
  static constexpr mpn::limb_t top_mask = mpn::limb_max;

  using storage_type = micron::vector<mpn::limb_t, Alloc, false>;

  storage_type buf;

  store() noexcept : buf() { }

  store(const store &) = default;
  store(store &&) noexcept = default;
  store &operator=(const store &) = default;
  store &operator=(store &&) noexcept = default;
  ~store() = default;

  [[nodiscard, gnu::always_inline]] mpn::limb_t *
  limbs() noexcept
  {
    return buf.data();
  }

  [[nodiscard, gnu::always_inline]] const mpn::limb_t *
  limbs() const noexcept
  {
    return buf.data();
  }

  [[nodiscard, gnu::always_inline]] usize
  size() const noexcept
  {
    return buf.size();
  }

  [[gnu::always_inline]] void
  set_size(usize k) noexcept
  {
    buf.set_size(k);
  }

  [[nodiscard, gnu::always_inline]] usize
  capacity() const noexcept
  {
    return buf.max_size();
  }

  [[nodiscard, gnu::always_inline]] bool
  ensure(usize k)
  {
    if ( k <= buf.max_size() ) return true;
    buf.try_reserve(k);
    return buf.max_size() >= k;
  }

  [[gnu::always_inline]] void
  finish(usize k) noexcept
  {
    if ( k > buf.max_size() ) k = buf.max_size();
    buf.set_size(mpn::normalize(buf.data(), k));
  }

  [[gnu::always_inline]] void
  swap_with(store &o) noexcept
  {
    buf.swap(o.buf);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// operation scratch

template<usize N, class Alloc, bool Bounded> struct scratch;

template<usize N, class Alloc> struct scratch<N, Alloc, true> {
  mpn::limb_t d[N];

  constexpr explicit scratch(usize) noexcept
  {
    if consteval {
      for ( usize i = 0; i < N; ++i ) d[i] = 0;
    }
  }

  [[nodiscard, gnu::always_inline]] constexpr mpn::limb_t *
  get() noexcept
  {
    return d;
  }
};

template<usize N, class Alloc> struct scratch<N, Alloc, false> {
  micron::vector<mpn::limb_t, Alloc, false> v;

  explicit scratch(usize n) : v(n) { }

  [[nodiscard, gnu::always_inline]] mpn::limb_t *
  get() noexcept
  {
    return v.data();
  }
};

};      // namespace __arb
};      // namespace math
};      // namespace micron
