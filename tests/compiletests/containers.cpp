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

  micron::tuple<int, char, long> t{ 1, 'a', 2L };
  acc += micron::get<0>(t);

  return acc & 0x7f;
}
