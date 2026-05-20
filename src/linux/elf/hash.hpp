//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "consts.hpp"
#include "header.hpp"

namespace micron
{
namespace elf
{

inline constexpr u32
sysv_hash(const char *name) noexcept
{
  u32 h = 0;
  u32 g = 0;
  while ( *name ) {
    h = (h << 4) + static_cast<u8>(*name++);
    g = h & 0xf0000000;
    if ( g ) h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

// modern preferred
inline constexpr u32
gnu_hash(const char *name) noexcept
{
  u32 h = 5381;
  while ( *name ) h = h * 33 + static_cast<u8>(*name++);
  return h;
}

// spec sysv layout
// word nbuckets
// word nchain
// word bucket[nbuckets]
// word chain[nchain]
//
inline const sym_t *
sysv_lookup(const dyn_info_t &d, const char *name) noexcept
{
  if ( !d.hash || !d.symtab || !d.strtab ) return nullptr;
  const word nbuckets = d.hash[0];
  if ( !nbuckets ) return nullptr;
  const word *buckets = d.hash + 2;
  const word *chain = buckets + nbuckets;

  const u32 h = sysv_hash(name);
  for ( word i = buckets[h % nbuckets]; i != 0; i = chain[i] ) {
    const sym_t &s = d.symtab[i];
    const char *n = d.strtab + s.name;
    const char *m = name;
    while ( *n && *n == *m ) {
      ++n;
      ++m;
    }
    if ( *n == 0 && *m == 0 ) return &s;
  }
  return nullptr;
}

// gnu hash layout:
// word nbuckets
// word symbias
// word bloom_size
// word bloom_shift
// xword bloom[bloom_size]
// word bucket[nbuckets]
// word chain[]
inline const sym_t *
gnu_lookup(const dyn_info_t &d, const char *name) noexcept
{
  if ( !d.gnu_hash || !d.symtab || !d.strtab ) return nullptr;
  const word *gh = d.gnu_hash;
  const word nbuckets = gh[0];
  const word symbias = gh[1];
  const word bloom_size = gh[2];
  const word bloom_shift = gh[3];
  if ( !nbuckets ) return nullptr;

  const xword *bloom = reinterpret_cast<const xword *>(gh + 4);
  const word *buckets = reinterpret_cast<const word *>(bloom + bloom_size);
  const word *chain = buckets + nbuckets;

  const u32 h = gnu_hash(name);
  const xword bw = bloom[(h / 64) % bloom_size];
  const xword bit_a = static_cast<xword>(1) << (h % 64);
  const xword bit_b = static_cast<xword>(1) << ((h >> bloom_shift) % 64);
  if ( (bw & (bit_a | bit_b)) != (bit_a | bit_b) ) return nullptr;

  word idx = buckets[h % nbuckets];
  if ( idx < symbias ) return nullptr;

  for ( ;; ) {
    const word chain_h = chain[idx - symbias];
    if ( ((chain_h ^ h) >> 1) == 0 ) {
      const sym_t &s = d.symtab[idx];
      const char *n = d.strtab + s.name;
      const char *m = name;
      while ( *n && *n == *m ) {
        ++n;
        ++m;
      }
      if ( *n == 0 && *m == 0 ) return &s;
    }
    if ( chain_h & 1 ) break;
    ++idx;
  }
  return nullptr;
}

// modern gnu hash preferred
inline const sym_t *
lookup_sym(const dyn_info_t &d, const char *name) noexcept
{
  if ( const sym_t *s = gnu_lookup(d, name) ) return s;
  return sysv_lookup(d, name);
}

inline xword
count_dynsyms(const dyn_info_t &d) noexcept
{
  if ( d.hash ) {
    return static_cast<xword>(d.hash[1]);
  }
  if ( d.gnu_hash ) {
    const word *gh = d.gnu_hash;
    const word nbuckets = gh[0];
    const word symbias = gh[1];
    const word bloom_size = gh[2];
    if ( !nbuckets ) return symbias;
    const xword *bloom = reinterpret_cast<const xword *>(gh + 4);
    const word *buckets = reinterpret_cast<const word *>(bloom + bloom_size);
    const word *chain = buckets + nbuckets;

    word max_idx = symbias;
    for ( word b = 0; b < nbuckets; ++b ) {
      word idx = buckets[b];
      if ( idx < symbias ) continue;
      for ( ;; ) {
        if ( idx >= max_idx ) max_idx = idx + 1;
        if ( chain[idx - symbias] & 1 ) break;
        ++idx;
      }
    }
    return max_idx;
  }
  return 0;
}

};      // namespace elf
};      // namespace micron
