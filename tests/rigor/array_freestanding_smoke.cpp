//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/__std.hpp"
#include "../../src/types.hpp"

#include "../../src/array/array.hpp"
#include "../../src/array/bisect_array.hpp"
#include "../../src/array/carray.hpp"
#include "../../src/array/constexprarray.hpp"
#include "../../src/array/farray.hpp"
#include "../../src/array/iarray.hpp"

namespace
{

struct Ctr {
  static inline int live = 0;
  int v = 0;

  Ctr() { ++live; }

  explicit Ctr(int x) : v(x) { ++live; }

  Ctr(const Ctr &o) : v(o.v) { ++live; }

  Ctr(Ctr &&o) noexcept : v(o.v) { ++live; }

  Ctr &operator=(const Ctr &o) = default;
  Ctr &operator=(Ctr &&o) noexcept = default;

  ~Ctr() { --live; }

  bool
  operator==(const Ctr &o) const
  {
    return v == o.v;
  }

  bool
  operator<(const Ctr &o) const
  {
    return v < o.v;
  }
};

template<class T>
constexpr T &&
mv(T &x) noexcept
{
  return static_cast<T &&>(x);
}
}      // namespace

constexpr int
ce_sum()
{
  micron::constexpr_array<int, 4> x{ 1, 2, 3, 4 };
  int s = 0;
  for ( auto it = x.cbegin(); it != x.cend(); ++it ) s += *it;
  return s;
}

static_assert(ce_sum() == 10, "constexpr_array iteration must work in constant evaluation");

extern "C" int
main(int, char **, char **)
{

  {
    micron::array<int, 4> a{ 2, 3, 4, 5 };
    if ( a.add() != 14 || a.sub() != -10 || a.mul() != 120 || a.div() != 0 ) return 11;
  }
  {
    micron::farray<int, 4> a{ 2, 3, 4, 5 };
    if ( a.add() != 14 || a.front() != 2 || a.back() != 5 ) return 12;
  }

  {
    micron::array<int, 4> a{ 10, 20, 30, 40 };
    if ( a.front() != 10 || a.back() != 40 ) return 13;
  }

  Ctr::live = 0;
  {
    micron::array<Ctr, 8> a;
    {
      micron::array<Ctr, 8> b(mv(a));
      (void)b;
    }
  }
  if ( Ctr::live != 0 ) return 21;

  Ctr::live = 0;
  {
    micron::carray<Ctr, 8> a;
    {
      micron::carray<Ctr, 8> b(mv(a));
      (void)b;
    }
  }
  if ( Ctr::live != 0 ) return 22;

  Ctr::live = 0;
  {
    micron::iarray<Ctr, 8> a;
    if ( Ctr::live != 8 ) return 23;
    {
      micron::iarray<Ctr, 8> b(mv(a));
      (void)b;
    }
  }
  if ( Ctr::live != 0 ) return 24;

  {
    micron::bisect_array<int, 8> b;
    b.insert(5);
    b.insert(2);
    b.insert(8);
    b.insert(1);
    if ( b[0] != 1 || b[3] != 8 || b.front() != 1 || b.back() != 8 ) return 31;
  }

  {
    micron::constexpr_array<int, 4> a;
    if ( a.front() != 0 ) return 41;
    micron::constexpr_array<int, 4> c{ 1, 2, 3, 4 };
    if ( c.at(2) != 3 || c.back() != 4 ) return 42;
  }

  return 0;
}
