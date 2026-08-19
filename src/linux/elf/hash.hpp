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

inline constexpr bool
__streq(const char *n, const char *m) noexcept
{
  while ( *n && *n == *m ) {
    ++n;
    ++m;
  }
  return *n == 0 && *m == 0;
}

// spec sysv layout
// word nbuckets
// word nchain
// word bucket[nbuckets]
// word chain[nchain]
//
// DT_HASH entries are 32-bit in both classes
template<fmt_class C>
inline const typename elf_traits<C>::sym *
sysv_lookup(const dyn_info<C> &d, const char *name) noexcept
{
  if ( !d.hash || !d.symtab || !d.strtab ) return nullptr;
  const word nbuckets = d.hash[0];
  const word nchain = d.hash[1];      // chain[] length == dynsym count
  if ( !nbuckets ) return nullptr;
  const word *buckets = d.hash + 2;
  const word *chain = buckets + nbuckets;

  const u32 h = sysv_hash(name);
  word i = buckets[h % nbuckets];
  for ( word steps = 0; i != 0 && i < nchain && steps < nchain; i = chain[i], ++steps ) {
    const auto &s = d.symtab[i];
    if ( __streq(d.strtab + s.name, name) ) return &s;
  }
  return nullptr;
}

// gnu hash layout:
// word  nbuckets
// word  symbias
// word  bloom_size
// word  bloom_shift
// BLOOM bloom[bloom_size]        <- Elf32_Word on ELF32, Elf64_Xword on ELF64
// word  bucket[nbuckets]
// word  chain[]
//
template<fmt_class C>
inline const typename elf_traits<C>::sym *
gnu_lookup(const dyn_info<C> &d, const char *name) noexcept
{
  using tr = elf_traits<C>;
  using bloom_t = typename tr::uword;
  constexpr u32 bb = static_cast<u32>(tr::bloom_bits);

  if ( !d.gnu_hash || !d.symtab || !d.strtab ) return nullptr;
  const word *gh = d.gnu_hash;
  const word nbuckets = gh[0];
  const word symbias = gh[1];
  const word bloom_size = gh[2];
  const word bloom_shift = gh[3];
  if ( !nbuckets || !bloom_size ) return nullptr;      // bloom_size==0 would divide-by-zero below

  const bloom_t *bloom = reinterpret_cast<const bloom_t *>(gh + 4);
  const word *buckets = reinterpret_cast<const word *>(bloom + bloom_size);
  const word *chain = buckets + nbuckets;

  const u32 h = gnu_hash(name);
  const bloom_t bw = bloom[(h / bb) % bloom_size];
  const bloom_t bit_a = static_cast<bloom_t>(1) << (h % bb);
  // mask by the bloom word width
  const bloom_t bit_b = static_cast<bloom_t>(1) << ((static_cast<bloom_t>(h) >> (bloom_shift & (bb - 1))) % bb);
  if ( (bw & (bit_a | bit_b)) != (bit_a | bit_b) ) return nullptr;

  word idx = buckets[h % nbuckets];
  if ( idx < symbias ) return nullptr;

  const word maxidx = d.symcount ? static_cast<word>(d.symcount) : (idx + (static_cast<word>(1) << 24));
  for ( ; idx < maxidx; ++idx ) {
    const word chain_h = chain[idx - symbias];
    if ( ((chain_h ^ h) >> 1) == 0 ) {
      const auto &s = d.symtab[idx];
      if ( __streq(d.strtab + s.name, name) ) return &s;
    }
    if ( chain_h & 1 ) break;
  }
  return nullptr;
}

// modern gnu hash preferred
template<fmt_class C>
inline const typename elf_traits<C>::sym *
lookup_sym(const dyn_info<C> &d, const char *name) noexcept
{
  if ( const auto *s = gnu_lookup(d, name) ) return s;
  return sysv_lookup(d, name);
}

template<fmt_class C>
inline half
sym_version_index(const dyn_info<C> &d, const typename elf_traits<C>::sym *s) noexcept
{
  if ( !d.versym || !s || !d.symtab ) return ver_ndx_global;
  const usize i = static_cast<usize>(s - d.symtab);
  if ( d.symcount && i >= d.symcount ) return ver_ndx_global;
  return elf_ver_ndx(d.versym[i]);
}

template<fmt_class C>
inline bool
sym_version_hidden(const dyn_info<C> &d, const typename elf_traits<C>::sym *s) noexcept
{
  if ( !d.versym || !s || !d.symtab ) return false;
  const usize i = static_cast<usize>(s - d.symtab);
  if ( d.symcount && i >= d.symcount ) return false;
  return (d.versym[i] & ver_ndx_hidden) != 0;
}

// libLLVM.so.22.1 is the largest .dynsym on a stock box at ~42k entries
inline constexpr xword max_dynsyms = xword(1) << 24;

// WARNING: do NOT simplify this to (DT_STRTAB - DT_SYMTAB); wrong for lld
// which lays out .dynsym, .gnu.version, .gnu.version_r, .gnu.hash, .dynstr
template<fmt_class C>
inline xword
__dynsym_upper_bound(const dyn_info<C> &d) noexcept
{
  using tr = elf_traits<C>;

  const u8 *const base = reinterpret_cast<const u8 *>(d.symtab);
  if ( !base ) return 0;

  const void *const cands[] = {
    reinterpret_cast<const void *>(d.strtab),     reinterpret_cast<const void *>(d.versym),
    reinterpret_cast<const void *>(d.verdef),     reinterpret_cast<const void *>(d.verneed),
    reinterpret_cast<const void *>(d.gnu_hash),   reinterpret_cast<const void *>(d.hash),
    reinterpret_cast<const void *>(d.rela),       reinterpret_cast<const void *>(d.rel),
    reinterpret_cast<const void *>(d.relr),       reinterpret_cast<const void *>(d.jmprel),
    reinterpret_cast<const void *>(d.pltgot),     reinterpret_cast<const void *>(d.init_array),
    reinterpret_cast<const void *>(d.fini_array), reinterpret_cast<const void *>(d.preinit_array),
  };

  const u8 *first = nullptr;
  for ( const void *c : cands ) {
    const u8 *p = reinterpret_cast<const u8 *>(c);
    if ( !p || p <= base ) continue;
    if ( !first || p < first ) first = p;
  }
  if ( !first ) return 0;

  const usize ent = d.syment ? static_cast<usize>(d.syment) : sizeof(typename tr::sym);
  if ( ent < sizeof(typename tr::sym) ) return 0;      // a bogus DT_SYMENT would mis-stride symtab[]

  return static_cast<xword>(static_cast<usize>(first - base) / ent);
}

// WARNING: the GNU hash chains cover only the exported definitions, [symoffset, symcount)
template<fmt_class C>
inline xword
__gnu_hash_high_water(const dyn_info<C> &d, xword limit) noexcept
{
  using tr = elf_traits<C>;
  using bloom_t = typename tr::uword;

  if ( !d.gnu_hash ) return 0;
  const word *gh = d.gnu_hash;
  const word nbuckets = gh[0];
  const word symbias = gh[1];
  const word bloom_size = gh[2];
  if ( !nbuckets ) return symbias;

  const bloom_t *bloom = reinterpret_cast<const bloom_t *>(gh + 4);
  const word *buckets = reinterpret_cast<const word *>(bloom + bloom_size);
  const word *chain = buckets + nbuckets;

  const word cap = (limit && limit < max_dynsyms) ? static_cast<word>(limit) : static_cast<word>(max_dynsyms);

  word high = symbias;
  for ( word b = 0; b < nbuckets; ++b ) {
    word idx = buckets[b];
    if ( idx < symbias ) continue;
    for ( ; idx < cap; ++idx ) {
      if ( idx >= high ) high = idx + 1;
      if ( chain[idx - symbias] & 1 ) break;
    }
  }
  return high;
}

template<fmt_class C>
inline xword
count_dynsyms(const dyn_info<C> &d) noexcept
{
  if ( !d.symtab ) return 0;      // no table to index; every consumer bails on the null pointer

  const xword bound = __dynsym_upper_bound(d);

  if ( d.hash ) {
    const xword n = static_cast<xword>(d.hash[1]);
    if ( n && n <= max_dynsyms && (bound == 0 || n <= bound) ) return n;
  }

  if ( bound ) {
    const xword floor_hi = __gnu_hash_high_water(d, bound);
    const xword n = floor_hi > bound ? floor_hi : bound;
    return n <= max_dynsyms ? n : max_dynsyms;
  }

  if ( d.gnu_hash ) {
    const xword n = __gnu_hash_high_water(d, 0);
    return n ? n : 1;
  }

  return 1;
}

};      // namespace elf
};      // namespace micron
