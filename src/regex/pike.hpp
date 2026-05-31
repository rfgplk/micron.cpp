//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "program.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the Pike virtual machine
// Thompson-NFA simulation that runs the compiled program against the input

namespace micron
{
namespace rgx
{

struct vm {
  prog_view pv;
  usize nslots = 0;      // == 2 * (ngroups + 1)
  usize cap = 0;         // max threads per list == pv.ncode
  const char *in = nullptr;
  usize n = 0;
  bool anchored = false;
  usize start_at = 0;

  u32 *a_pc = nullptr;
  i64 *a_sav = nullptr;
  usize a_n = 0;
  u32 *b_pc = nullptr;
  i64 *b_sav = nullptr;
  usize b_n = 0;

  u32 *seen = nullptr;
  u32 gen = 0;
  i64 *work = nullptr;
  i64 *mcaps = nullptr;
  bool matched = false;
  i64 mstart = 0;
  i64 mend = 0;

  constexpr void
  addthread(u32 *lpc, i64 *lsav, usize &ln, u32 pc, usize sp, i64 *sav) noexcept
  {
    if ( seen[pc] == gen ) return;
    seen[pc] = gen;
    const inst &I = pv.code[pc];
    switch ( I.code ) {
    case op::Jmp:
      addthread(lpc, lsav, ln, I.x, sp, sav);
      break;
    case op::Split:
      addthread(lpc, lsav, ln, I.x, sp, sav);
      addthread(lpc, lsav, ln, I.y, sp, sav);
      break;
    case op::Save:
      if ( I.x < nslots ) {
        i64 old = sav[I.x];
        sav[I.x] = (i64)sp;
        addthread(lpc, lsav, ln, pc + 1, sp, sav);
        sav[I.x] = old;
      } else {
        addthread(lpc, lsav, ln, pc + 1, sp, sav);
      }
      break;
    case op::Bol:
      if ( sp == 0 ) addthread(lpc, lsav, ln, pc + 1, sp, sav);
      break;
    case op::Eol:
      if ( sp == n ) addthread(lpc, lsav, ln, pc + 1, sp, sav);
      break;
    default:
      if ( ln < cap ) {
        lpc[ln] = pc;
        for ( usize s = 0; s < nslots; ++s ) lsav[ln * nslots + s] = sav[s];
        ++ln;
      }
      break;
    }
  }

  constexpr bool
  run() noexcept
  {
    for ( usize s = 0; s < nslots; ++s ) {
      work[s] = -1;
      mcaps[s] = -1;
    }
    matched = false;
    a_n = 0;
    b_n = 0;
    ++gen;
    addthread(a_pc, a_sav, a_n, 0, start_at, work);

    for ( usize sp = start_at;; ++sp ) {
      if ( a_n == 0 && (matched || anchored) ) break;
      int c = (sp < n) ? (int)(unsigned char)in[sp] : -1;
      ++gen;
      b_n = 0;
      for ( usize i = 0; i < a_n; ++i ) {
        u32 pc = a_pc[i];
        i64 *sav = &a_sav[i * nslots];
        const inst &I = pv.code[pc];
        switch ( I.code ) {
        case op::Char:
          if ( c == (int)I.x ) addthread(b_pc, b_sav, b_n, pc + 1, sp + 1, sav);
          break;
        case op::Class:
          if ( c >= 0 && pv.cls[I.x].test((unsigned char)c) ) addthread(b_pc, b_sav, b_n, pc + 1, sp + 1, sav);
          break;
        case op::Any:
          if ( c >= 0 ) addthread(b_pc, b_sav, b_n, pc + 1, sp + 1, sav);
          break;
        case op::Match: {
          i64 st = sav[0], en = (i64)sp;
          if ( !matched || st < mstart || (st == mstart && en > mend) ) {
            for ( usize s = 0; s < nslots; ++s ) mcaps[s] = sav[s];
            mstart = st;
            mend = en;
            matched = true;
          }
          break;
        }
        default:
          break;
        }
      }
      if ( !matched && !anchored && (sp + 1) <= n ) addthread(b_pc, b_sav, b_n, 0, sp + 1, work);

      u32 *tp = a_pc;
      a_pc = b_pc;
      b_pc = tp;
      i64 *ts = a_sav;
      a_sav = b_sav;
      b_sav = ts;
      a_n = b_n;

      if ( c < 0 ) break;
    }
    return matched;
  }
};

};      // namespace rgx
};      // namespace micron
