//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../alloc.hpp"
#include "../simd/simd.hpp"
#include "../types.hpp"

#include "program.hpp"

namespace micron
{
namespace rgx
{

inline constexpr i32 kDfaMaxStates = 255;      // state ids fit in a u8
inline constexpr u8 kShengAccept = 0x10;       // bit 4 -- $-less accept
inline constexpr u8 kShengEol = 0x20;          // bit 5 -- accept only at end ($)

struct dfa {
  i32 nstates = 0;
  i32 start = 0;
  u8 *table = nullptr;           // nstates*256: next state id
  u8 *accept = nullptr;          // nstates: 0/1  ($-less match here)
  u8 *eol_accept = nullptr;      // nstates: 0/1  (match here iff at end)
  bool has_sheng = false;
  u8 masks[256][16];      // valid iff has_sheng
  u8 start_byte = 0;
};

inline void
dfa_eclose(prog_view pv, u32 pc, char *seen, u64 *key, bool &accept, bool &eol, bool at_start, bool in_eol) noexcept
{
  if ( pc >= pv.ncode ) return;
  const inst &I = pv.code[pc];
  if ( I.code == op::Match ) {      // terminal -- never deduped, so both flags can set
    if ( in_eol )
      eol = true;
    else
      accept = true;
    return;
  }
  if ( seen[pc] ) return;
  seen[pc] = 1;
  switch ( I.code ) {
  case op::Char:
  case op::Class:
  case op::Any:
    if ( !in_eol ) key[pc >> 6] |= (u64(1) << (pc & 63));      // cannot consume after $
    break;
  case op::Jmp:
    dfa_eclose(pv, I.x, seen, key, accept, eol, at_start, in_eol);
    break;
  case op::Split:
    dfa_eclose(pv, I.x, seen, key, accept, eol, at_start, in_eol);
    dfa_eclose(pv, I.y, seen, key, accept, eol, at_start, in_eol);
    break;
  case op::Save:
    dfa_eclose(pv, pc + 1, seen, key, accept, eol, at_start, in_eol);
    break;
  case op::Bol:
    if ( at_start ) dfa_eclose(pv, pc + 1, seen, key, accept, eol, at_start, in_eol);      // ^: pass only at offset 0
    break;
  case op::Eol:
    dfa_eclose(pv, pc + 1, seen, key, accept, eol, at_start, true);      // $: everything past is end-only
    break;
  case op::Match:
    break;
  }
}

inline bool
dfa_consumes(const inst &I, u8 c, const charreach *cls, usize ncls) noexcept
{
  switch ( I.code ) {
  case op::Char:
    return (u8)I.x == c;
  case op::Class:
    return I.x < ncls && cls[I.x].test(c);
  case op::Any:
    return true;
  default:
    return false;
  }
}

inline dfa *
build_dfa(prog_view pv, char *seen) noexcept
{
  if ( pv.ncode == 0 || pv.ncode > 256 ) return nullptr;

  bool has_bol = false;
  for ( usize i = 0; i < pv.ncode; ++i )
    if ( pv.code[i].code == op::Bol ) {
      has_bol = true;
      break;
    }

  // start-anchored iff there is NO way to begin matching without a ^
  bool start_anchored = false;
  if ( has_bol ) {
    u64 k[4] = {};
    bool a = false, e = false;
    for ( usize i = 0; i < pv.ncode; ++i ) seen[i] = 0;
    dfa_eclose(pv, 0, seen, k, a, e, false, false);
    start_anchored = (k[0] == 0 && k[1] == 0 && k[2] == 0 && k[3] == 0 && !a && !e);
    if ( !start_anchored ) return nullptr;      // mixed ^ -> Pike
  }
  const bool restart = !start_anchored;      // unanchored DFAs re-seed the start every step

  const i32 __cap = kDfaMaxStates;
  u64 *keys = micron::alloc<u64>((usize)(__cap + 1) * 4 * sizeof(u64));
  u8 *acc = micron::alloc<u8>((usize)(__cap + 1));
  u8 *eolacc = micron::alloc<u8>((usize)(__cap + 1));
  u8 *tbl = micron::alloc<u8>((usize)(__cap + 1) * 256);
  i32 nstates = 0;
  bool overflow = false;

  auto same = [&](i32 i, const u64 *k) -> bool {
    const u64 *ki = keys + (usize)i * 4;
    return ki[0] == k[0] && ki[1] == k[1] && ki[2] == k[2] && ki[3] == k[3];
  };
  auto intern = [&](const u64 *k, bool a, bool e) -> i32 {
    for ( i32 i = 0; i < nstates; ++i )
      if ( acc[i] == (a ? 1 : 0) && eolacc[i] == (e ? 1 : 0) && same(i, k) ) return i;
    if ( nstates >= __cap ) {
      overflow = true;
      return -1;
    }
    u64 *ki = keys + (usize)nstates * 4;
    ki[0] = k[0];
    ki[1] = k[1];
    ki[2] = k[2];
    ki[3] = k[3];
    acc[nstates] = a ? 1 : 0;
    eolacc[nstates] = e ? 1 : 0;
    return nstates++;
  };

  u64 k0[4] = {};
  bool a0 = false, e0 = false;
  for ( usize i = 0; i < pv.ncode; ++i ) seen[i] = 0;
  dfa_eclose(pv, 0, seen, k0, a0, e0, /*at_start=*/true, false);
  i32 start = intern(k0, a0, e0);

  for ( i32 sid = 0; sid < nstates && !overflow; ++sid ) {
    const u64 *ks = keys + (usize)sid * 4;
    for ( i32 c = 0; c < 256; ++c ) {
      u64 nk[4] = {};
      bool na = false, ne = false;
      for ( usize i = 0; i < pv.ncode; ++i ) seen[i] = 0;
      if ( restart ) dfa_eclose(pv, 0, seen, nk, na, ne, false, false);      // unanchored restart
      for ( usize pc = 0; pc < pv.ncode; ++pc ) {
        if ( !((ks[pc >> 6] >> (pc & 63)) & 1) ) continue;
        if ( dfa_consumes(pv.code[pc], (u8)c, pv.cls, pv.ncls) ) dfa_eclose(pv, pc + 1, seen, nk, na, ne, false, false);
      }
      i32 nid = intern(nk, na, ne);
      if ( nid < 0 ) break;
      tbl[(usize)sid * 256 + (usize)c] = (u8)nid;
    }
  }

  if ( overflow || start < 0 ) {
    micron::free(keys);
    micron::free(acc);
    micron::free(eolacc);
    micron::free(tbl);
    return nullptr;
  }

  dfa *d = micron::alloc<dfa>(sizeof(dfa));
  d->nstates = nstates;
  d->start = start;
  d->table = micron::alloc<u8>((usize)nstates * 256);
  d->accept = micron::alloc<u8>((usize)nstates);
  d->eol_accept = micron::alloc<u8>((usize)nstates);
  for ( i32 s = 0; s < nstates; ++s ) {
    d->accept[s] = acc[s];
    d->eol_accept[s] = eolacc[s];
    for ( i32 c = 0; c < 256; ++c ) d->table[(usize)s * 256 + (usize)c] = tbl[(usize)s * 256 + (usize)c];
  }

  d->has_sheng = (nstates <= 16);
  if ( d->has_sheng ) {
    auto sbyte = [&](i32 s) -> u8 { return (u8)(s | (d->accept[s] ? kShengAccept : 0) | (d->eol_accept[s] ? kShengEol : 0)); };
    for ( i32 c = 0; c < 256; ++c )
      for ( i32 s = 0; s < 16; ++s ) {
        i32 ns = (s < nstates) ? d->table[(usize)s * 256 + (usize)c] : 0;
        d->masks[c][s] = sbyte(ns);
      }
    d->start_byte = sbyte(start);
  }

  micron::free(keys);
  micron::free(acc);
  micron::free(eolacc);
  micron::free(tbl);
  return d;
}

inline void
dfa_free(dfa *d) noexcept
{
  if ( !d ) return;
  if ( d->table ) micron::free(d->table);
  if ( d->accept ) micron::free(d->accept);
  if ( d->eol_accept ) micron::free(d->eol_accept);
  micron::free(d);
}

// one PSHUFB per byte (<=16 states), unrolled 4 bytes per iteration; four loads are independent, shuffles chained on cur
inline bool
dfa_sheng_has_match(const dfa *d, const char *in, usize n) noexcept
{
  if ( d->start_byte & kShengAccept ) return true;
  if ( n == 0 ) return (d->start_byte & kShengEol) != 0;
  const u8(*masks)[16] = d->masks;
  micron::simd::v8 cur((signed char)d->start_byte);
  usize i = 0;

  for ( ; i + 4 <= n; i += 4 ) {
    micron::simd::v8 m1;
    m1.uload(const_cast<u8 *>(masks[(u8)in[i + 0]]));
    cur = m1.shuffle(cur);
    u8 a1 = cur.byte0();
    micron::simd::v8 m2;
    m2.uload(const_cast<u8 *>(masks[(u8)in[i + 1]]));
    cur = m2.shuffle(cur);
    u8 a2 = cur.byte0();
    micron::simd::v8 m3;
    m3.uload(const_cast<u8 *>(masks[(u8)in[i + 2]]));
    cur = m3.shuffle(cur);
    u8 a3 = cur.byte0();
    micron::simd::v8 m4;
    m4.uload(const_cast<u8 *>(masks[(u8)in[i + 3]]));
    cur = m4.shuffle(cur);
    u8 a4 = cur.byte0();
    if ( (a1 | a2 | a3 | a4) & kShengAccept ) return true;      // any of the 4 accepted
  }

  for ( ; i < n; ++i ) {
    micron::simd::v8 m;
    m.uload(const_cast<u8 *>(masks[(u8)in[i]]));
    cur = m.shuffle(cur);
    if ( cur.byte0() & kShengAccept ) return true;
  }
  return (cur.byte0() & kShengEol) != 0;
}

inline bool
dfa_table_has_match(const dfa *d, const char *in, usize n) noexcept
{
  i32 s = d->start;
  if ( d->accept[s] ) return true;
  if ( n == 0 ) return d->eol_accept[s] != 0;
  for ( usize i = 0; i < n; ++i ) {
    s = d->table[(usize)s * 256 + (u8)in[i]];
    if ( d->accept[s] ) return true;
  }
  return d->eol_accept[s] != 0;
}

inline bool
dfa_has_match(const dfa *d, const char *in, usize n) noexcept
{
  return d->has_sheng ? dfa_sheng_has_match(d, in, n) : dfa_table_has_match(d, in, n);
}

};      // namespace rgx
};      // namespace micron
