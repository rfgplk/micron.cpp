//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "hash/hash.hpp"

#include "../bits.hpp"
#include "../simd/types.hpp"
#include "../types.hpp"

namespace micron
{

struct __mask {
  i32 bits;
  explicit __mask(i32 b) : bits(b) {}
  bool
  any() const
  {
    return bits != 0;
  }
  int
  lowest() const
  {
    return countr_zero(bits);
  }
};

template <typename K, typename V, size_t N, size_t NH = 16> class stack_swiss_map
{
  static constexpr u8
  __hash(const K &k)
  {
    return hash<hash64_t>(k) & 0xFF;     // lower 8
  }

  static hash64_t
  __b_index(const K &k)
  {
    return hash<hash64_t>(k) % N;
  }
  __mask
  __match(u8 hash, size_t ind) const
  {
    simd::i128 match = _mm_set1_epi8(hash);
    simd::i128 meta = _mm_load_si128(reinterpret_cast<const simd::i128 *>(__control_bytes[ind]));
    int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(match, meta));
    return __mask(mask);
  }

public:
  alignas(16) u8 __control_bytes[N]{};
  struct __swiss_entry {
    K key;
    V value;
  };

  __swiss_entry __entries[N]{};
  size_t __size = 0;

  bool
  insert(const K &key, const V &value)
  {
    if ( __size >= N )
      return false;

    u8 h2 = __hash(key);
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; ++i ) {
      size_t _i = (start + i) % N;
      if ( __control_bytes[_i] == 0 ) {
        __control_bytes[_i] = h2;
        __entries[_i] = __swiss_entry{ key, value };
        ++__size;
        return true;
      }
    }

    return false;
  }

  V *
  find(const K &key)
  {
    u8 h2 = __hash(key);
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; i += 16 ) {
      size_t _i = (start + i) % N;

      if ( _i + 16 <= N ) {
        __mask m = __match(h2, _i);
        int mask = m.bits;
        while ( mask ) {
          int offset = __builtin_ctz(mask);
          size_t probe = _i + offset;
          if ( __entries[probe].key == key )
            return &__entries[probe].value;
          mask &= mask - 1;
        }
      } else {
        size_t chunk = N - _i;
        for ( size_t j = 0; j < chunk && j < NH - i; ++j ) {
          size_t probe = (_i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key )
            return &__entries[probe].value;
        }
      }
    }
    return nullptr;
  }
};

};
