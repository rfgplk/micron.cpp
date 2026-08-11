//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../string/format.hpp"
#include "convert.hpp"
#include "signed.hpp"
#include "unsigned.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// making arbint printable

namespace micron
{
namespace format
{

namespace __impl
{

// base and the '#' prefix
template<typename A>
inline void
__arb_write(hstring<schar> &out, const A &v, const __impl::fmt_spec &spec, bool neg)
{
  u32 base = 10;
  bool upper = false;
  if ( spec.type == 'x' )
    base = 16;
  else if ( spec.type == 'X' ) {
    base = 16;
    upper = true;
  } else if ( spec.type == 'o' )
    base = 8;
  else if ( spec.type == 'b' )
    base = 2;

  if ( neg ) out += '-';
  if ( spec.alt ) {
    if ( base == 16 ) {
      out += '0';
      out += (upper ? 'X' : 'x');
    } else if ( base == 8 ) {
      out += '0';
    } else if ( base == 2 ) {
      out += '0';
      out += 'b';
    }
  }

  // base 2; one character per bit, plus slack
  const usize cap = v.bit_length() + 8u;
  micron::vector<char> tmp;
  tmp.resize(cap);
  const usize n = micron::math::to_chars(&tmp[0], cap, v, base, upper);
  if ( n == 0 ) {
    out += '0';
    return;
  }
  out.append(&tmp[0], n);
}

};      // namespace __impl

template<usize B, micron::math::arb_solver S, class A> struct formatter<micron::math::arbuint<B, S, A>> {
  static inline void
  write_str(hstring<schar> &out, const micron::math::arbuint<B, S, A> &val, const __impl::fmt_spec &spec)
  {
    __impl::__arb_write(out, val, spec, false);
  }
};

template<usize B, micron::math::arb_solver S, class A> struct formatter<micron::math::arbint<B, S, A>> {
  static inline void
  write_str(hstring<schar> &out, const micron::math::arbint<B, S, A> &val, const __impl::fmt_spec &spec)
  {
    __impl::__arb_write(out, val.magnitude(), spec, val.negative());
  }
};

};      // namespace format

namespace math
{

// found by ADL from io/echo.hpp's unqualified printk(s, args)
template<typename Sk, usize B, arb_solver S, class A>
  requires requires(Sk &s, const char *p, usize n) { s.put(p, n); }
max_t
printk(Sk &s, const arbuint<B, S, A> &v)
{
  const usize cap = v.bit_length() + 8u;
  micron::vector<char> tmp;
  tmp.resize(cap);
  const usize n = micron::math::to_chars(&tmp[0], cap, v, 10u, false);
  if ( n == 0 ) return s.put("0", 1);
  return s.put(&tmp[0], n);
}

template<typename Sk, usize B, arb_solver S, class A>
  requires requires(Sk &s, const char *p, usize n) { s.put(p, n); }
max_t
printk(Sk &s, const arbint<B, S, A> &v)
{
  const auto mag = v.magnitude();
  const usize cap = mag.bit_length() + 9u;
  micron::vector<char> tmp;
  tmp.resize(cap);
  usize off = 0;
  if ( v.negative() ) {
    tmp[0] = '-';
    off = 1;
  }
  const usize n = micron::math::to_chars(&tmp[0] + off, cap - off, mag, 10u, false);
  if ( n == 0 ) return s.put("0", 1);
  return s.put(&tmp[0], n + off);
}

};      // namespace math
};      // namespace micron
