//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

template<typename BitType> struct flag_traits {
  static constexpr bool is_bitmask = false;
};

template<typename BitType> class flags
{
public:
  using bits_type = BitType;
  using mask_type = underlying_type_t<BitType>;

  constexpr flags() noexcept : __m(0) { }

  constexpr flags(BitType bit) noexcept : __m(static_cast<mask_type>(bit)) { }

  constexpr flags(const flags &) noexcept = default;
  constexpr flags(flags &&) noexcept = default;

  constexpr explicit flags(mask_type raw) noexcept : __m(raw) { }

  constexpr flags &operator=(const flags &) noexcept = default;
  constexpr flags &operator=(flags &&) noexcept = default;

  constexpr flags
  operator|(flags r) const noexcept
  {
    return flags(__m | r.__m);
  }

  constexpr flags
  operator&(flags r) const noexcept
  {
    return flags(__m & r.__m);
  }

  constexpr flags
  operator^(flags r) const noexcept
  {
    return flags(__m ^ r.__m);
  }

  constexpr flags
  operator~() const noexcept
  {
    return flags(__m ^ flag_traits<BitType>::all_flags.__m);
  }

  constexpr flags &
  operator|=(flags r) noexcept
  {
    __m |= r.__m;
    return *this;
  }

  constexpr flags &
  operator&=(flags r) noexcept
  {
    __m &= r.__m;
    return *this;
  }

  constexpr flags &
  operator^=(flags r) noexcept
  {
    __m ^= r.__m;
    return *this;
  }

  constexpr bool
  operator==(flags r) const noexcept
  {
    return __m == r.__m;
  }

  constexpr bool
  operator!=(flags r) const noexcept
  {
    return __m != r.__m;
  }

  constexpr bool
  operator!() const noexcept
  {
    return __m == 0;
  }

  constexpr explicit
  operator mask_type() const noexcept
  {
    return __m;
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return __m != 0;
  }

  constexpr mask_type
  mask() const noexcept
  {
    return __m;
  }

private:
  mask_type __m;

  template<typename> friend struct flag_traits;
};

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator|(BitType a, BitType b) noexcept
{
  return flags<BitType>(a) | flags<BitType>(b);
}

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator&(BitType a, BitType b) noexcept
{
  return flags<BitType>(a) & flags<BitType>(b);
}

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator^(BitType a, BitType b) noexcept
{
  return flags<BitType>(a) ^ flags<BitType>(b);
}

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator~(BitType a) noexcept
{
  return ~flags<BitType>(a);
}

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator|(BitType a, flags<BitType> b) noexcept
{
  return flags<BitType>(a) | b;
}

template<typename BitType>
  requires(flag_traits<BitType>::is_bitmask)
constexpr flags<BitType>
operator&(BitType a, flags<BitType> b) noexcept
{
  return flags<BitType>(a) & b;
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
