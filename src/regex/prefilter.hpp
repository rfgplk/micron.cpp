//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "charreach.hpp"
#include "program.hpp"

namespace micron
{
namespace rgx
{
struct first_info {
  charreach first;
  bool nullable = false;
};

inline void
first_walk(prog_view pv, u32 pc, char *seen, charreach &first, bool &nullable) noexcept
{
  if ( pc >= pv.ncode || seen[pc] ) return;
  seen[pc] = 1;
  const inst &I = pv.code[pc];
  switch ( I.code ) {
  case op::Char:
    first.set((unsigned char)I.x);
    break;
  case op::Class:
    if ( I.x < pv.ncls ) first |= pv.cls[I.x];
    break;
  case op::Any:
    first.set_all();
    break;
  case op::Match:
    nullable = true;
    break;
  case op::Jmp:
    first_walk(pv, I.x, seen, first, nullable);
    break;
  case op::Split:
    first_walk(pv, I.x, seen, first, nullable);
    first_walk(pv, I.y, seen, first, nullable);
    break;
  case op::Save:
  case op::Bol:
  case op::Eol:
    first_walk(pv, pc + 1, seen, first, nullable);
    break;
  }
}

inline first_info
compute_first(prog_view pv, char *seen) noexcept
{
  first_info fi;
  for ( usize i = 0; i < pv.ncode; ++i ) seen[i] = 0;
  if ( pv.ncode ) first_walk(pv, 0, seen, fi.first, fi.nullable);
  return fi;
}

};      // namespace rgx
};      // namespace micron
