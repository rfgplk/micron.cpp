//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// every input byte must matter.
//
// this is the property zzz violated: its round masked the state to 48 bits
// (mask64 = 0xFFFFFFFFFFFF), and with a single round the top byte of each 8-byte lane had not
// propagated below bit 48 by the time the mask cut it -- so byte 7, 15, 23, ... of the input were
// ignored ENTIRELY. "key_0000" and "key_0001" hashed identically, and micron::hopscotch_map, which
// stores only the hash and treats hash-equal keys as one entry, silently dropped 18 of 20 inserts.
//
// hash_avalanche and hash_collide both passed throughout: avalanche measures output bit spread for
// inputs that already differ, and the collision suite's corpora happened not to differ solely in a
// dead byte. a positional test is what catches a dead byte, so that is what this is.

#include "../../src/hash/hash.hpp"

#include "../snowball/snowball.hpp"

using ::sb::print;
using ::sb::require_true;

typedef u64 (*hfn)(const byte *, i64, usize);

// the ISA-free hashes take a u64 seed; adapt them the way hash_avalanche.cpp does
static u64
h_rapid(const byte *p, i64 s, usize n)
{
  return micron::hashes::rapidhash(p, (u64)s, n);
}

static u64
h_xxh(const byte *p, i64 s, usize n)
{
  return micron::hashes::xxhash64_rtseed(p, (u64)s, n);
}

#if defined(__micron_hash_zzz)
static u64
h_zzzf(const byte *p, i64 s, usize n)
{
  return micron::hashes::zzzf64(p, s, n);
}
#endif

struct entry {
  const char *name;
  hfn fn;
  usize floor;      // known-weak only: the measured worst, pinned so it cannot silently drop
};

// N distinct buffers that differ ONLY at byte `pos`; they must produce N distinct hashes.
static usize
distinct_at(hfn h, usize len, usize pos, usize n, byte *buf, u64 *out)
{
  for ( usize i = 0; i < n; ++i ) {
    for ( usize k = 0; k < len; ++k ) buf[k] = static_cast<byte>(0xAA);
    buf[pos] = static_cast<byte>(i);
    out[i] = h(buf, 0x243F6A8885A308D3LL, len);
  }
  usize c = 0;
  for ( usize i = 0; i < n; ++i ) {
    bool seen = false;
    for ( usize j = 0; j < i; ++j )
      if ( out[j] == out[i] ) {
        seen = true;
        break;
      }
    if ( !seen ) ++c;
  }
  return c;
}

int
main()
{
  print("=== HASH BYTE SENSITIVITY ===");

  // default-grade: every one of these is reachable as default_hash_64 on some target, or is the
  // ISA-free fallback. they must have no weak byte at all.
  static const entry strong[] = {
#if defined(__micron_hash_zzz)
    { "zzz64", &micron::hashes::zzz64, 0 },
    { "zzzf64", h_zzzf, 0 },
#endif
    { "rapidhash", h_rapid, 0 },
    { "xxhash64", h_xxh, 0 },
  };

  // KNOWN WEAK, tracked in ISSUES.md, never selected as a default. asserted only against a recorded
  // floor so that they cannot silently get worse -- not against the ideal, which they do not meet.
  //   z64  : weak at the top byte of each lane. (S - (S<<3)) is -7*S and (S - (S<<2)) is -3*S,
  //          XORed together, which is only one shift-subtract of diffusion. it was a fully DEAD
  //          byte until the 48-bit mask came out of the round (see ISSUES.md).
  //   zz64 : the round uses sub_sat_i8, a SATURATING 8-bit subtract, which clamps and is therefore
  //          lossy by construction.
  static const entry weak[] = {
#if defined(__micron_hash_zzz)
    { "z64", &micron::hashes::z64, 24 },
    { "zz64", &micron::hashes::zz64, 11 },
#endif
  };

  static byte buf[256];
  static u64 out[64];

  // 64 probes per position exposes a dead byte (which collapses to 1) while a sound 64-bit hash
  // gives 64 distinct with probability 1 - ~1e-16.
  constexpr usize kProbes = 64;

  for ( const auto &H : strong ) {
    for ( usize len : { usize{ 8 }, usize{ 16 }, usize{ 32 }, usize{ 64 }, usize{ 96 } } ) {
      usize worst = kProbes;
      usize worst_pos = 0;
      for ( usize pos = 0; pos < len; ++pos ) {
        usize d = distinct_at(H.fn, len, pos, kProbes, buf, out);
        if ( d < worst ) {
          worst = d;
          worst_pos = pos;
        }
      }
      if ( worst < kProbes ) print("  ", H.name, " len=", len, ": weakest byte pos=", worst_pos, " -> ", worst, "/", kProbes);
      // no dead byte, and nothing even close to one
      require_true(worst > 1);
      require_true(worst * 2 > kProbes);
    }
    print("  byte sensitivity ok: ", H.name);
  }

  for ( const auto &H : weak ) {
    usize worst = kProbes;
    usize worst_pos = 0;
    usize worst_len = 0;
    for ( usize len : { usize{ 8 }, usize{ 16 }, usize{ 32 }, usize{ 64 } } ) {
      for ( usize pos = 0; pos < len; ++pos ) {
        usize d = distinct_at(H.fn, len, pos, kProbes, buf, out);
        if ( d < worst ) {
          worst = d;
          worst_pos = pos;
          worst_len = len;
        }
      }
    }
    print("  KNOWN WEAK (see ISSUES.md): ", H.name, " worst ", worst, "/", kProbes, " at len=", worst_len, " pos=", worst_pos);
    require_true(worst >= H.floor);      // cannot get worse
    if ( worst > H.floor ) print("    improved past its recorded floor of ", H.floor, " -- raise it");
  }

  print("[HASH BYTE SENSITIVITY OK]");
  return 1;
}
