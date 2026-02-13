//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../memory/memory.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "bernstein.hpp"
#include "fib.hpp"
#include "fnv.hpp"
#include "murmur.hpp"
#include "xx.hpp"

namespace micron
{

enum class hash_types { bernstein32, murmur128, xxhash64 };

// TODO: add more hash variants
constexpr hash_types default_hash_128 = hash_types::murmur128;
constexpr hash_types default_hash_64 = hash_types::xxhash64;
constexpr u32 default_seed = 0x567E24FA;

typedef u32 hash32_t;
typedef u64 hash64_t;
typedef micron::pair<u64, u64> hash128_t;

template <u32 seed = default_seed, is_container_or_string T>
hash128_t
hash128(const T &data)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 )
    return hashes::murmur<seed>(reinterpret_cast<const char *>(data.cbegin()), data.size());
}

template <u32 seed = default_seed>
hash128_t
hash128(const byte *data, size_t len)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 )
    return hashes::murmur<seed>(data, len);
}

template <u32 seed = default_seed>
hash128_t
hash128(const char *data)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 )
    return hashes::murmur<seed>(data, micron::strlen(data));
}

template <u64 seed = default_seed, is_container_or_string T>
hash64_t
hash64(const T &data)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
}

template <u64 seed = default_seed>
hash64_t
hash64(const char *data)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
}

template <u64 seed = default_seed>
hash64_t
hash64(const byte *data, size_t len)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64<seed>(data, len);
}

template <typename T>
hash64_t
hash64(const T *data, size_t len, u64 seed)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64_rtseed(reinterpret_cast<const byte *>(data), seed, len);
}

hash64_t
hash64(const byte *data, size_t len, u64 seed)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64_rtseed(data, seed, len);
}

template <typename R = hash64_t, u32 seed = default_seed>
inline __attribute__((always_inline)) auto
hash(const char *data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> )
    return hash64<seed>(data);
  else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data);
}

template <typename R = hash64_t, is_container_or_string T, u32 seed = default_seed>
inline __attribute__((always_inline)) auto
hash(const T &data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> )
    return hash64<seed>(data);
  else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data);
}

template <typename R = hash64_t, u32 seed = default_seed>
inline __attribute__((always_inline)) auto
hash(const byte *data, size_t len)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> )
    return hash64<seed>(data, len);
  else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data, len);
}

template <typename R = hash64_t, typename F, u32 seed = default_seed>
  requires micron::is_arithmetic_v<F>
inline __attribute__((always_inline)) auto
hash(const F data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> )
    return hash64<seed>(reinterpret_cast<const byte *>(&data), sizeof(F));
  else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(reinterpret_cast<const byte *>(&data), sizeof(F));
}

};
