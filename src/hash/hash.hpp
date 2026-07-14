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
#include "rapidhash.hpp"
#include "xx.hpp"

// NOTE: zzz using intrinsics directly, so we need a direct port
#if defined(__micron_arch_x86_any)
#include "zzz.hpp"
#elif defined(__micron_arch_arm_any)
#include "zzz_arm.hpp"
#endif

// meow_hash is x86-only (AES-NI needed)
#if defined(__micron_arch_x86_any)
#include "meowhash.hpp"
#endif

// NOTE: ive added a default fallback in case a default hash is set to a bogus/wrong value (for correctness), the default hash returns a 0
// hash, be careful!

// NOTE: zzz is AVX2-specific on x86 and NEON on arm
#if !defined(MICRON_NO_ZZZ_HASH) && (defined(__micron_arch_arm_any) || defined(__micron_x86_avx2))
#define __micron_hash_zzz 1
#endif

namespace micron
{

enum class hash_types { bernstein32, murmur128, xxhash64, zzz128, zzz, zz, z, zzzf128, zzzf, rapidhash, meowhash128, meowhash };

// TODO: add more hash variants
// default to homegrown zzz
#if defined(__micron_hash_zzz)
constexpr hash_types default_hash_128 = hash_types::zzz128;
constexpr hash_types default_hash_64 = hash_types::zzz;
#else
// no zzz here: fall back to the ISA free hashes
constexpr hash_types default_hash_128 = hash_types::murmur128;
constexpr hash_types default_hash_64 = hash_types::rapidhash;
#endif
constexpr u64 default_seed = 0x369bd65914cf0616;      // try to keep it in signed i32 range
constexpr u32 default_seed_32 = 0xf2062906;

typedef u32 hash32_t;
typedef u64 hash64_t;
typedef micron::pair<u64, u64> hash128_t;

template<u32 seed = default_seed_32, is_container_or_string T>
hash128_t
hash128(const T &data)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 )
    return hashes::murmur<seed>(reinterpret_cast<const char *>(data.cbegin()), data.size());
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_128 == hash_types::zzz128 )
    return hashes::zzz128<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
  if constexpr ( default_hash_128 == hash_types::zzzf128 )
    return hashes::zzzf128<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#endif
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_128 == hash_types::meowhash128 )
    return hashes::meowhash128<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#endif
  return hash128_t{};
}

template<u32 seed = default_seed_32>
hash128_t
hash128(const byte *data, usize len)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 ) return hashes::murmur<seed>(data, len);
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_128 == hash_types::zzz128 ) return hashes::zzz128<seed>(data, len);
  if constexpr ( default_hash_128 == hash_types::zzzf128 ) return hashes::zzzf128<seed>(data, len);
#endif
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_128 == hash_types::meowhash128 ) return hashes::meowhash128<seed>(data, len);
#endif
  return hash128_t{};
}

template<u32 seed = default_seed_32>
hash128_t
hash128(const char *data)
{
  if constexpr ( default_hash_128 == hash_types::murmur128 ) return hashes::murmur<seed>(data, micron::strlen(data));
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_128 == hash_types::zzz128 )
    return hashes::zzz128<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
  if constexpr ( default_hash_128 == hash_types::zzzf128 )
    return hashes::zzzf128<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#endif
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_128 == hash_types::meowhash128 )
    return hashes::meowhash128<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#endif
  return hash128_t{};
}

template<u64 seed = default_seed, is_container_or_string T>
hash64_t
hash64(const T &data)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_64 == hash_types::zzz )
    return hashes::zzz64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
  if constexpr ( default_hash_64 == hash_types::zzzf )
    return hashes::zzzf64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
  if constexpr ( default_hash_64 == hash_types::zz ) return hashes::zz64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
  if constexpr ( default_hash_64 == hash_types::z ) return hashes::z64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#endif
  if constexpr ( default_hash_64 == hash_types::rapidhash )
    return hashes::rapidhash<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_64 == hash_types::meowhash )
    return hashes::meowhash64<seed>(reinterpret_cast<const byte *>(data.cbegin()), data.size());
#endif
  return hash64_t{};
}

template<u64 seed = default_seed>
hash64_t
hash64(const char *data)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 )
    return hashes::xxhash64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_64 == hash_types::zzz )
    return hashes::zzz64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
  if constexpr ( default_hash_64 == hash_types::zzzf )
    return hashes::zzzf64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
  if constexpr ( default_hash_64 == hash_types::zz ) return hashes::zz64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
  if constexpr ( default_hash_64 == hash_types::z ) return hashes::z64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#endif
  if constexpr ( default_hash_64 == hash_types::rapidhash )
    return hashes::rapidhash<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_64 == hash_types::meowhash )
    return hashes::meowhash64<seed>(reinterpret_cast<const byte *>(data), micron::strlen(data));
#endif
  return hash64_t{};
}

template<u64 seed = default_seed>
hash64_t
hash64(const byte *data, usize len)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 ) return hashes::xxhash64<seed>(data, len);
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_64 == hash_types::zzz ) return hashes::zzz64<seed>(data, len);
  if constexpr ( default_hash_64 == hash_types::zzzf ) return hashes::zzzf64<seed>(data, len);
  if constexpr ( default_hash_64 == hash_types::zz ) return hashes::zz64<seed>(data, len);
  if constexpr ( default_hash_64 == hash_types::z ) return hashes::z64<seed>(data, len);
#endif
  if constexpr ( default_hash_64 == hash_types::rapidhash ) return hashes::rapidhash<seed>(data, len);
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_64 == hash_types::meowhash ) return hashes::meowhash64<seed>(data, len);
#endif
  return hash64_t{};
}

template<typename T>
hash64_t
hash64(const T *data, usize len, u64 seed)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 ) return hashes::xxhash64_rtseed(reinterpret_cast<const byte *>(data), seed, len);
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_64 == hash_types::zzz ) return hashes::zzz64(reinterpret_cast<const byte *>(data), seed, len);
  if constexpr ( default_hash_64 == hash_types::zzzf ) return hashes::zzzf64(reinterpret_cast<const byte *>(data), seed, len);
  if constexpr ( default_hash_64 == hash_types::zz ) return hashes::zz64(reinterpret_cast<const byte *>(data), seed, len);
  if constexpr ( default_hash_64 == hash_types::z ) return hashes::z64(reinterpret_cast<const byte *>(data), seed, len);
#endif
  if constexpr ( default_hash_64 == hash_types::rapidhash ) return hashes::rapidhash(reinterpret_cast<const byte *>(data), seed, len);
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_64 == hash_types::meowhash ) return hashes::meowhash64(reinterpret_cast<const byte *>(data), seed, len);
#endif
  return hash64_t{};
}

hash64_t
hash64(const byte *data, usize len, u64 seed)
{
  if constexpr ( default_hash_64 == hash_types::xxhash64 ) return hashes::xxhash64_rtseed(data, seed, len);
#if defined(__micron_hash_zzz)
  if constexpr ( default_hash_64 == hash_types::zzz ) return hashes::zzz64(data, seed, len);
  if constexpr ( default_hash_64 == hash_types::zzzf ) return hashes::zzzf64(data, seed, len);
  if constexpr ( default_hash_64 == hash_types::zz ) return hashes::zz64(data, seed, len);
  if constexpr ( default_hash_64 == hash_types::z ) return hashes::z64(data, seed, len);
#endif
  if constexpr ( default_hash_64 == hash_types::rapidhash ) return hashes::rapidhash(data, seed, len);
#if defined(__micron_arch_x86_any)
  if constexpr ( default_hash_64 == hash_types::meowhash ) return hashes::meowhash64(data, seed, len);
#endif
  return hash64_t{};
}

template<typename R = hash64_t, u32 seed = default_seed_32>
inline __attribute__((always_inline)) auto
hash(const char *data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(data);
    else
      return hash64<seed>(data);
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data);
}

template<typename R = hash64_t, is_container_or_string T, u32 seed = default_seed_32>
inline __attribute__((always_inline)) auto
hash(const T &data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(data);
    else
      return hash64<seed>(data);
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data);
}

template<typename R = hash64_t, u32 seed = default_seed_32>
inline __attribute__((always_inline)) auto
hash(const byte *data, usize len)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(data, len);
    else
      return hash64<seed>(data, len);
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(data, len);
}

template<typename R = hash64_t, typename F, u32 seed = default_seed_32>
  requires micron::is_arithmetic_v<F>
inline __attribute__((always_inline)) auto
hash(const F data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(reinterpret_cast<const byte *>(&data), sizeof(F));
    else
      return hash64<seed>(reinterpret_cast<const byte *>(&data), sizeof(F));
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(reinterpret_cast<const byte *>(&data), sizeof(F));
}

#if !defined(__micron_arch_width_64)
// 32-bit targets have uint128_t/int128_t as class backed fallbacks
template<typename R = hash64_t, u32 seed = default_seed_32>
inline __attribute__((always_inline)) auto
hash(const uint128_t &data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(reinterpret_cast<const byte *>(&data), sizeof(uint128_t));
    else
      return hash64<seed>(reinterpret_cast<const byte *>(&data), sizeof(uint128_t));
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(reinterpret_cast<const byte *>(&data), sizeof(uint128_t));
}

template<typename R = hash64_t, u32 seed = default_seed_32>
inline __attribute__((always_inline)) auto
hash(const int128_t &data)
{
  if constexpr ( sizeof(R) == 8 or micron::is_same_v<R, hash64_t> ) {
    if constexpr ( seed == default_seed_32 )
      return hash64(reinterpret_cast<const byte *>(&data), sizeof(int128_t));
    else
      return hash64<seed>(reinterpret_cast<const byte *>(&data), sizeof(int128_t));
  } else if constexpr ( micron::is_same_v<R, hash128_t> or micron::is_convertible_v<R, hash128_t> )
    return hash128<seed>(reinterpret_cast<const byte *>(&data), sizeof(int128_t));
}
#endif

};      // namespace micron
