#pragma once

#include "algorithm/mem.hpp"
#include "concepts.hpp"
#include "memory/memory.hpp"
#include "string/format.h"
#include "tags.hpp"
#include "types.hpp"

namespace micron
{
// N is number of bits in field, R is the desired return type (bool by default)
template <size_t N, typename R = bool>
  requires(N % 8 == 0) and std::is_fundamental_v<R>
class alignas(64) bitfield
{
  u8 bits[N / 8];

public:
  using category_type = bitfield_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;

  ~bitfield() = default;
  constexpr bitfield(const bool b = false)
  {
    if ( !b )
      micron::czero<N / 8>(&bits[0]);     // do this explicitly
    else
      micron::cfull<N / 8>(&bits[0]);     // do this explicitly
  }

  template <typename T> constexpr bitfield(const T *ch, const T zero = '0', const T one = '1')     // standard modified
  {
    size_t sz = micron::strlen(ch);
    if ( sz > (N) )
      throw except::library_error("micron::bitfield bitfield initialization string is too long");
    for ( size_t i = 0; i < (sz / 8); i++ ) {
      if ( ch[i] == zero ) [[likely]] {
      } else if ( ch[i] == one )     // zero by default so no need for that
        bits[i] |= (1 << (i % 8));
    }
  }
  template <is_string T> bitfield(const T &str, const T zero = '0', const T one = '1')     // standard modified
  {
    if ( str.size() > (N) )
      throw except::library_error("micron::bitfield bitfield initialization string is too long");
    for ( size_t i = 0; i < (str.size() / 8); i++ ) {
      if ( str[i] == zero ) [[likely]] {
      } else if ( str[i] == one )     // zero by default so no need for that
        bits[i] |= (1 << (i % 8));
    }
  }
  constexpr bitfield(const bitfield &o) { micron::cmemcpy<N / 8>(bits, o.bits); }
  constexpr bitfield(bitfield &&o)
  {
    micron::cmemcpy<N / 8>(bits, o.bits);
    micron::czero<N / 8>(o.bits);
  }
  constexpr bitfield &
  operator=(const bitfield &o)
  {
    micron::cmemcpy<N / 8>(bits, o.bits);
    return *this;
  }
  constexpr bitfield &
  operator=(bitfield &&o)
  {
    micron::cmemcpy<N / 8>(bits, o.bits);
    micron::czero<N / 8>(o.bits);
    return *this;
  }
  constexpr R
  operator[](const size_t n)
  {
    return (bits[n / 8] & (1 << n % 8));
  }
  constexpr const R
  operator[](const size_t n) const
  {
    return (bits[n / 8] & (1 << n % 8));
  }
  constexpr bool
  operator!(void) const
  {
    return none();
  }
  constexpr bitfield &
  swap(bitfield &a)
  {
    auto i = bits;
    bits = a;
    a = i;
    return *this;
  }
  constexpr R
  at(const size_t n)
  {
    if ( n >= N / 8 )
      throw except::out_of_range_error("micron::bitfield at() out of range");
    return (bits[n / 8] & (1 << n % 8));
  }
  constexpr const R
  at(const size_t n) const
  {
    if ( n >= N / 8 )
      throw except::out_of_range_error("micron::bitfield at() out of range");
    return (bits[n / 8] & (1 << n % 8));
  }
  constexpr bool
  all(void) const
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      if ( bits[i] != 0xFF )
        return false;
    return true;
  }
  constexpr bool
  any(void) const
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      if ( bits[i] > 0x00 )
        return true;
    return false;
  }
  constexpr bool
  none(void) const
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      if ( bits[i] != 0x00 )
        return false;
    return true;
  }
  constexpr size_t
  size() const
  {
    return (N / 8);
  }
  constexpr bitfield &
  operator&=(const bitfield &o)
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      bits[i] &= o[i];
    return *this;
  }
  constexpr bitfield &
  operator|=(const bitfield &o)
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      bits[i] |= o[i];
    return *this;
  }
  constexpr bitfield &
  operator^=(const bitfield &o)
  {
    for ( size_t i = 0; i < (N / 8); i++ )
      bits[i] ^= o[i];
    return *this;
  }
  // TODO: experiment with adding explicit AVX for all functions (although the compiler does a good job of
  // autovectorizing)
  constexpr inline bitfield &
  flip(void)
  {
    for ( size_t i = 0; i < (N / 8); i += 4 ) {
      bits[i] ^= 0xFF;
      bits[i + 1] ^= 0xFF;
      bits[i + 2] ^= 0xFF;
      bits[i + 3] ^= 0xFF;
    }
    // uint64_t m32 = 0xFFFFFFFF;
    // uint32_t m = m32 >> _lzcnt_u32(bits[i]);
    // bits[i] ^= m;
    return *this;
  }
  constexpr bitfield &
  flip(const size_t n)
  {
    bits[n] ^= 0xFF;
    return *this;
  }
  constexpr bitfield &
  set(void)
  {
    for ( size_t i = 0; i < (N / 8); i += 4 ) {
      bits[i] = 0xFF;
      bits[i + 1] = 0xFF;
      bits[i + 2] = 0xFF;
      bits[i + 3] = 0xFF;
    }
    return *this;
  }
  constexpr bitfield &
  set(const size_t n)
  {
    bits[n / 8] |= (1 << n % 8);
    return *this;
  }
  constexpr bitfield &
  clear(const size_t n)
  {
    bits[n / 8] &= ~(1 << n % 8);
    return *this;
  }
};
};
