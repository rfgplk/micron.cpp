//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/elf/hash.hpp"
#include "../linux/elf/header.hpp"
#include "../memory/cstring.hpp"

namespace micron
{
namespace elf
{
namespace dl
{

inline constexpr usize ver_chain_max = 4096;

inline const char *
verneed_name(const dyn_info_t &d, half vidx) noexcept
{
  if ( vidx == ver_ndx_local || vidx == ver_ndx_global ) return nullptr;
  if ( !d.verneed || !d.strtab ) return nullptr;

  const u8 *vn = reinterpret_cast<const u8 *>(d.verneed);
  for ( usize guard = 0; guard < ver_chain_max; ++guard ) {
    const verneed_t *n = reinterpret_cast<const verneed_t *>(vn);
    const u8 *va = vn + n->aux;
    for ( half k = 0; k < n->cnt && k < ver_chain_max; ++k ) {
      const vernaux_t *a = reinterpret_cast<const vernaux_t *>(va);
      if ( elf_ver_ndx(a->other) == vidx ) return d.strtab + a->name;
      if ( a->next == 0 ) break;
      va += a->next;
    }
    if ( n->next == 0 ) break;
    vn += n->next;
  }
  return nullptr;
}

inline const char *
verdef_name(const dyn_info_t &d, half vidx) noexcept
{
  if ( !d.verdef || !d.strtab ) return nullptr;

  const u8 *vd = reinterpret_cast<const u8 *>(d.verdef);
  for ( usize guard = 0; guard < ver_chain_max; ++guard ) {
    const verdef_t *v = reinterpret_cast<const verdef_t *>(vd);
    if ( elf_ver_ndx(v->ndx) == vidx && v->cnt != 0 && v->aux != 0 ) {
      const verdaux_t *a = reinterpret_cast<const verdaux_t *>(vd + v->aux);
      return d.strtab + a->name;
    }
    if ( v->next == 0 ) break;
    vd += v->next;
  }
  return nullptr;
}

inline bool
version_matches(const dyn_info_t &def, usize di, const char *want) noexcept
{
  if ( !def.versym ) return true;
  if ( def.symcount && di >= def.symcount ) return false;

  const half raw = def.versym[di];
  const half vidx = elf_ver_ndx(raw);
  const bool hidden = (raw & ver_ndx_hidden) != 0;

  if ( vidx == ver_ndx_local ) return false;

  if ( !want ) return !hidden;

  if ( vidx == ver_ndx_global ) {

    return true;
  }

  const char *have = verdef_name(def, vidx);
  if ( !have ) return false;
  return micron::strcmp(have, want) == 0;
}

inline const char *
wanted_version(const dyn_info_t &ref, u32 si) noexcept
{
  if ( !ref.versym || !ref.symtab ) return nullptr;
  if ( ref.symcount && si >= ref.symcount ) return nullptr;
  return verneed_name(ref, elf_ver_ndx(ref.versym[si]));
}

inline const nsym_t *
gnu_lookup_versioned(const dyn_info_t &d, const char *name, const char *want) noexcept
{
  using tr = native_traits;
  using bloom_t = tr::uword;
  constexpr u32 bb = static_cast<u32>(tr::bloom_bits);

  if ( !d.gnu_hash || !d.symtab || !d.strtab ) return nullptr;
  const word *gh = d.gnu_hash;
  const word nbuckets = gh[0];
  const word symbias = gh[1];
  const word bloom_size = gh[2];
  const word bloom_shift = gh[3];
  if ( !nbuckets || !bloom_size ) return nullptr;

  const bloom_t *bloom = reinterpret_cast<const bloom_t *>(gh + 4);
  const word *buckets = reinterpret_cast<const word *>(bloom + bloom_size);
  const word *chain = buckets + nbuckets;

  const u32 h = gnu_hash(name);
  const bloom_t bw = bloom[(h / bb) % bloom_size];
  const bloom_t bit_a = static_cast<bloom_t>(1) << (h % bb);
  // mask by the bloom word width, word size varies on arch spec
  const bloom_t bit_b = static_cast<bloom_t>(1) << ((static_cast<bloom_t>(h) >> (bloom_shift & (bb - 1))) % bb);
  if ( (bw & (bit_a | bit_b)) != (bit_a | bit_b) ) return nullptr;

  word idx = buckets[h % nbuckets];
  if ( idx < symbias ) return nullptr;

  const word maxidx = d.symcount ? static_cast<word>(d.symcount) : (idx + (static_cast<word>(1) << 24));
  for ( ; idx < maxidx; ++idx ) {
    const word chain_h = chain[idx - symbias];
    if ( ((chain_h ^ h) >> 1) == 0 ) {
      const nsym_t &s = d.symtab[idx];
      if ( __streq(d.strtab + s.name, name) && version_matches(d, idx, want) ) return &s;
    }
    if ( chain_h & 1 ) break;
  }
  return nullptr;
}

inline const nsym_t *
sysv_lookup_versioned(const dyn_info_t &d, const char *name, const char *want) noexcept
{
  if ( !d.hash || !d.symtab || !d.strtab ) return nullptr;
  const word nbuckets = d.hash[0];
  const word nchain = d.hash[1];
  if ( !nbuckets ) return nullptr;
  const word *buckets = d.hash + 2;
  const word *chain = buckets + nbuckets;

  const u32 h = sysv_hash(name);
  word i = buckets[h % nbuckets];
  for ( word steps = 0; i != 0 && i < nchain && steps < nchain; i = chain[i], ++steps ) {
    const nsym_t &s = d.symtab[i];
    if ( __streq(d.strtab + s.name, name) && version_matches(d, i, want) ) return &s;
  }
  return nullptr;
}

inline const nsym_t *
lookup_versioned(const dyn_info_t &d, const char *name, const char *want) noexcept
{
  if ( const nsym_t *s = gnu_lookup_versioned(d, name, want) ) return s;
  return sysv_lookup_versioned(d, name, want);
}

};      // namespace dl
};      // namespace elf
};      // namespace micron
