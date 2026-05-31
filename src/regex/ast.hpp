//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "charreach.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// constexpr recursive-descent parser for
// the POSIX Extended Regular Expressions (ERE) **ONLY**
// Supported ERE grammar:
//   alt    := concat ('|' concat)*
//   concat := piece*
//   piece  := atom ('*' | '+' | '?' | '{m}' | '{m,}' | '{m,n}')?
//   atom   := '(' alt ')' | '[' class ']' | '.' | '^' | '$' | '\' c | char
//   class  := '^'? ( char | char '-' char | '[:name:]' )+

namespace micron
{
namespace rgx
{

enum class nk : u8 {
  Empty,       // matches the empty string
  Char,        // a == byte value
  Class,       // a == index into the class table
  Any,         // '.'  any byte
  Concat,      // a,b == left,right children
  Alt,         // a,b == left,right children
  Star,        // a == child  ( * )
  Plus,        // a == child  ( + )
  Quest,       // a == child  ( ? )
  Repeat,      // a == child, b == min, c == max  ({m,n}; c==kInf for unbounded)
  Group,       // a == child, b == group number (1-based)
  Bol,         // '^'
  Eol          // '$'
};

inline constexpr u32 kInf = 0xffffffffu;

struct node {
  nk kind = nk::Empty;
  u32 a = 0;
  u32 b = 0;
  u32 c = 0;
};

constexpr bool
posix_class(const char *name, usize n, charreach &r) noexcept
{
  auto is = [&](const char *s) constexpr {
    usize i = 0;
    for ( ; i < n && s[i]; ++i )
      if ( s[i] != name[i] ) return false;
    return s[i] == 0 && i == n;
  };
  if ( is("digit") ) {
    r.set_range('0', '9');
    return true;
  }
  if ( is("alpha") ) {
    r.set_range('A', 'Z');
    r.set_range('a', 'z');
    return true;
  }
  if ( is("alnum") ) {
    r.set_range('0', '9');
    r.set_range('A', 'Z');
    r.set_range('a', 'z');
    return true;
  }
  if ( is("upper") ) {
    r.set_range('A', 'Z');
    return true;
  }
  if ( is("lower") ) {
    r.set_range('a', 'z');
    return true;
  }
  if ( is("space") ) {
    r.set(' ');
    r.set('\t');
    r.set('\n');
    r.set('\r');
    r.set('\v');
    r.set('\f');
    return true;
  }
  if ( is("blank") ) {
    r.set(' ');
    r.set('\t');
    return true;
  }
  if ( is("xdigit") ) {
    r.set_range('0', '9');
    r.set_range('A', 'F');
    r.set_range('a', 'f');
    return true;
  }
  if ( is("cntrl") ) {
    r.set_range(0, 31);
    r.set(127);
    return true;
  }
  if ( is("print") ) {
    r.set_range(32, 126);
    return true;
  }
  if ( is("graph") ) {
    r.set_range(33, 126);
    return true;
  }
  if ( is("punct") ) {
    r.set_range(33, 47);
    r.set_range(58, 64);
    r.set_range(91, 96);
    r.set_range(123, 126);
    return true;
  }
  return false;
}

struct parser {
  const char *pat = nullptr;
  usize len = 0;
  usize pos = 0;

  node *nodes = nullptr;
  usize maxn = 0;
  usize nn = 0;

  charreach *cls = nullptr;
  usize maxc = 0;
  usize ncls = 0;

  u32 ngroups = 0;
  bool ok = true;

  constexpr int
  peek(usize off = 0) const noexcept
  {
    return (pos + off) < len ? (int)(u8)pat[pos + off] : -1;
  }

  constexpr int
  advance(void) noexcept
  {
    return pos < len ? (int)(u8)pat[pos++] : -1;
  }

  constexpr u32
  mk(nk k, u32 a = 0, u32 b = 0, u32 c = 0) noexcept
  {
    if ( nn >= maxn ) {
      ok = false;
      return 0;
    }
    nodes[nn] = node{ k, a, b, c };
    return (u32)nn++;
  }

  constexpr u32
  add_class(const charreach &r) noexcept
  {
    if ( ncls >= maxc ) {
      ok = false;
      return 0;
    }
    cls[ncls] = r;
    return (u32)ncls++;
  }

  constexpr u32
  parse(void) noexcept
  {
    u32 r = parse_alt();
    if ( pos != len ) ok = false;
    return r;
  }

  constexpr u32
  parse_alt(void) noexcept
  {
    u32 left = parse_concat();
    while ( ok && peek() == '|' ) {
      pos++;
      u32 right = parse_concat();
      left = mk(nk::Alt, left, right);
    }
    return left;
  }

  constexpr u32
  parse_concat(void) noexcept
  {
    bool have = false;
    u32 left = 0;
    while ( ok ) {
      int ch = peek();
      if ( ch == -1 || ch == '|' || ch == ')' ) break;
      u32 piece = parse_piece();
      if ( !ok ) break;
      if ( !have ) {
        left = piece;
        have = true;
      } else {
        left = mk(nk::Concat, left, piece);
      }
    }
    if ( !have ) return mk(nk::Empty);
    return left;
  }

  constexpr u32
  parse_piece(void) noexcept
  {
    u32 atom = parse_atom();
    while ( ok ) {
      int ch = peek();
      if ( ch == '*' ) {
        pos++;
        atom = mk(nk::Star, atom);
      } else if ( ch == '+' ) {
        pos++;
        atom = mk(nk::Plus, atom);
      } else if ( ch == '?' ) {
        pos++;
        atom = mk(nk::Quest, atom);
      } else if ( ch == '{' && looks_like_interval() ) {
        atom = parse_interval(atom);
      } else {
        break;
      }
    }
    return atom;
  }

  constexpr bool
  looks_like_interval(void) const noexcept
  {
    usize i = pos + 1;
    if ( i >= len || pat[i] < '0' || pat[i] > '9' ) return false;
    while ( i < len && pat[i] >= '0' && pat[i] <= '9' ) ++i;
    if ( i < len && pat[i] == ',' ) {
      ++i;
      while ( i < len && pat[i] >= '0' && pat[i] <= '9' ) ++i;
    }
    return i < len && pat[i] == '}';
  }

  constexpr u32
  parse_interval(u32 atom) noexcept
  {
    pos++;      // '{'
    u32 lo = 0;
    while ( peek() >= '0' && peek() <= '9' ) lo = lo * 10 + (u32)(advance() - '0');
    u32 hi = lo;
    if ( peek() == ',' ) {
      pos++;
      if ( peek() == '}' ) {
        hi = kInf;
      } else {
        hi = 0;
        while ( peek() >= '0' && peek() <= '9' ) hi = hi * 10 + (u32)(advance() - '0');
      }
    }
    if ( peek() == '}' )
      pos++;
    else
      ok = false;
    if ( hi != kInf && hi < lo ) ok = false;
    return mk(nk::Repeat, atom, lo, hi);
  }

  constexpr u32
  parse_atom(void) noexcept
  {
    int ch = peek();
    if ( ch == '(' ) {
      pos++;
      u32 g = ++ngroups;
      u32 inner = parse_alt();
      if ( peek() == ')' )
        pos++;
      else
        ok = false;
      return mk(nk::Group, inner, g);
    }
    if ( ch == '[' ) return parse_class();
    if ( ch == '.' ) {
      pos++;
      return mk(nk::Any);
    }
    if ( ch == '^' ) {
      pos++;
      return mk(nk::Bol);
    }
    if ( ch == '$' ) {
      pos++;
      return mk(nk::Eol);
    }
    if ( ch == '\\' ) {
      pos++;
      int e = advance();
      if ( e == -1 ) {
        ok = false;
        return 0;
      }
      return mk(nk::Char, (u32)(u8)e);
    }
    if ( ch == -1 ) return mk(nk::Empty);
    pos++;
    return mk(nk::Char, (u32)(u8)ch);
  }

  constexpr u32
  parse_class(void) noexcept
  {
    pos++;
    charreach r;
    bool negate = false;
    if ( peek() == '^' ) {
      negate = true;
      pos++;
    }
    bool first = true;
    while ( ok ) {
      int ch = peek();
      if ( ch == -1 ) {
        ok = false;
        break;
      }
      if ( ch == ']' && !first ) {
        pos++;
        break;
      }
      first = false;
      if ( ch == '[' && peek(1) == ':' ) {
        usize start = pos + 2;
        usize i = start;
        while ( i < len && pat[i] != ':' ) ++i;
        if ( i + 1 < len && pat[i] == ':' && pat[i + 1] == ']' ) {
          if ( !posix_class(pat + start, i - start, r) ) ok = false;
          pos = i + 2;
          continue;
        }
      }
      int lo = advance();
      if ( lo == '\\' ) {
        int e = advance();
        if ( e == -1 ) {
          ok = false;
          break;
        }
        lo = e;
      }
      if ( peek() == '-' && peek(1) != ']' && peek(1) != -1 ) {
        pos++;
        int hi = advance();
        if ( hi == '\\' ) hi = advance();
        if ( hi == -1 ) {
          ok = false;
          break;
        }
        if ( (u8)lo <= (u8)hi )
          r.set_range((u8)lo, (u8)hi);
        else
          ok = false;
      } else {
        r.set((u8)lo);
      }
    }
    if ( negate ) r.flip();
    return mk(nk::Class, add_class(r));
  }
};

};      // namespace rgx
};      // namespace micron
