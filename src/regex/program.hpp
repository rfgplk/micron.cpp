//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "ast.hpp"
#include "charreach.hpp"

// opcodes:
//  -> Char  x: consume one byte == x
//  -> Class x: consume one byte in class table[x]
//  -> Any: consume any one byte
//  -> Match: accept
//  -> Jmp   x: goto x
//  -> Split x,y: fork: try x (higher priority) then y
//  -> Save  x: record current input offset into capture slot x
//  -> Bol: assert start-of-text (^)
//  -> Eol: assert end-of-text   ($)

namespace micron
{
namespace rgx
{

enum class op : unsigned char { Char, Class, Any, Match, Jmp, Split, Save, Bol, Eol };

struct inst {
  op code = op::Match;
  u32 x = 0;
  u32 y = 0;
};

struct prog_view {
  const inst *code = nullptr;
  usize ncode = 0;
  const charreach *cls = nullptr;
  usize ncls = 0;
  u32 ngroups = 0;      // capture groups, excluding whole-match group 0
};

struct emitter {
  const node *nodes = nullptr;
  inst *code = nullptr;
  usize maxi = 0;
  usize ni = 0;
  bool ok = true;

  constexpr u32
  emit(op c, u32 x = 0, u32 y = 0) noexcept
  {
    if ( ni >= maxi ) {
      ok = false;
      return maxi ? (u32)(maxi - 1) : 0;
    }
    code[ni] = inst{ c, x, y };
    return (u32)ni++;
  }

  constexpr void
  emit_node(u32 idx) noexcept
  {
    if ( !ok ) return;
    const node &n = nodes[idx];
    switch ( n.kind ) {
    case nk::Empty:
      break;
    case nk::Char:
      emit(op::Char, n.a);
      break;
    case nk::Class:
      emit(op::Class, n.a);
      break;
    case nk::Any:
      emit(op::Any);
      break;
    case nk::Bol:
      emit(op::Bol);
      break;
    case nk::Eol:
      emit(op::Eol);
      break;
    case nk::Concat:
      emit_node(n.a);
      emit_node(n.b);
      break;
    case nk::Alt: {
      u32 sp = emit(op::Split);
      emit_node(n.a);
      u32 jm = emit(op::Jmp);
      u32 l2 = (u32)ni;
      emit_node(n.b);
      code[sp].x = sp + 1;
      code[sp].y = l2;
      code[jm].x = (u32)ni;
      break;
    }
    case nk::Star: {
      u32 l1 = emit(op::Split);
      u32 bs = (u32)ni;
      emit_node(n.a);
      emit(op::Jmp, l1);
      code[l1].x = bs;
      code[l1].y = (u32)ni;
      break;
    }
    case nk::Plus: {      // body ; split body,out
      u32 bs = (u32)ni;
      emit_node(n.a);
      u32 sp = emit(op::Split);
      code[sp].x = bs;
      code[sp].y = (u32)ni;
      break;
    }
    case nk::Quest: {      // split body,out ; body
      u32 sp = emit(op::Split);
      emit_node(n.a);
      code[sp].x = sp + 1;
      code[sp].y = (u32)ni;
      break;
    }
    case nk::Group: {      // save 2g ; body ; save 2g+1
      u32 g = n.b;
      emit(op::Save, 2 * g);
      emit_node(n.a);
      emit(op::Save, 2 * g + 1);
      break;
    }
    case nk::Repeat: {
      u32 lo = n.b, hi = n.c, child = n.a;
      for ( u32 i = 0; i < lo && ok; ++i ) emit_node(child);
      if ( hi == kInf ) {      // a{m,} == a{m} a*
        u32 l1 = emit(op::Split);
        u32 bs = (u32)ni;
        emit_node(child);
        emit(op::Jmp, l1);
        code[l1].x = bs;
        code[l1].y = (u32)ni;
      } else {      // a{m,n} == a{m} (a?){n-m}
        for ( u32 i = lo; i < hi && ok; ++i ) {
          u32 sp = emit(op::Split);
          u32 bs = (u32)ni;
          emit_node(child);
          code[sp].x = bs;
          code[sp].y = (u32)ni;
        }
      }
      break;
    }
    }
  }
};

constexpr bool
compile_regex(const char *pat, usize len, node *nodes, usize maxn, charreach *cls, usize maxc, inst *code, usize maxi, usize &out_ncode,
              usize &out_ncls, u32 &out_ngroups) noexcept
{
  parser p;
  p.pat = pat;
  p.len = len;
  p.nodes = nodes;
  p.maxn = maxn;
  p.cls = cls;
  p.maxc = maxc;
  u32 root = p.parse();
  if ( !p.ok ) return false;

  emitter e;
  e.nodes = nodes;
  e.code = code;
  e.maxi = maxi;
  e.emit(op::Save, 0);
  e.emit_node(root);
  e.emit(op::Save, 1);
  e.emit(op::Match);
  if ( !e.ok ) return false;

  out_ncode = e.ni;
  out_ncls = p.ncls;
  out_ngroups = p.ngroups;
  return true;
}

};      // namespace rgx
};      // namespace micron
