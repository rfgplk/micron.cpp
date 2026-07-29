//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: micron containers instantiate + exercise their basic
// surface on every arch/opt combo (see tests/verify_compile.duck). Not run.

#include "../../src/array.hpp"
#include "../../src/list.hpp"
#include "../../src/map.hpp"
#include "../../src/queue.hpp"
#include "../../src/stack.hpp"
#include "../../src/tuple.hpp"
#include "../../src/vector.hpp"

// static_mpmc is the one container meant to live at namespace scope with no allocator behind it;
// this is what would break first in a freestanding (-k/-ke) cell
static micron::static_mpmc<int *, 16> __mpmc_global;

int
main()
{
  micron::vector<int> v = { 1, 2, 3, 4, 5 };
  v.push_back(6);
  v.emplace_back(7);
  int acc = 0;
  for ( auto &x : v ) acc += x;
  acc += static_cast<int>(v.size());
  acc += v[0];

  micron::list<int> l;
  l.push_back(10);
  l.push_front(20);

  micron::stack<int> st;
  st.push(1);

  micron::queue<int> q;
  q.push(2);

  // allocation-free MPMC: must instantiate on every arch/opt/freestanding cell, and at namespace
  // scope (see the static below) -- a template that is never instantiated proves nothing here
  micron::static_mpmc<int, 8> mq;
  mq.push(3);
  int mv = 0;
  if ( mq.pop(mv) ) acc += mv;
  acc += static_cast<int>(mq.size() + __mpmc_global.capacity());
  __mpmc_global.push(&acc);
  if ( __mpmc_global.pop() == &acc ) ++acc;

  micron::tuple<int, char, long> t{ 1, 'a', 2L };
  acc += micron::get<0>(t);

  return acc & 0x7f;
}
