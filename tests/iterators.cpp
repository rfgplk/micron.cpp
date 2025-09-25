//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/stdout.hpp"
#include "../src/iterator.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

int
dbl(int x)
{
  return x * 2;
}

bool
skip2(int x)
{
  if ( x == 0 )
    return true;
  else
    false;
  return false;
}

int
main(void)
{
  mc::vector<float> f(500, 1.0f);
  mc::vector<double> d(2000, 55342.2f);
  mc::iterator<float> fitr(f);
  mc::iterator<double> ditr(d);
  mc::io::stdoutln(*fitr, " ", *ditr);
  mc::io::stdoutln(fitr.next_value(), " ", ditr.next_value());
  mc::io::stdoutln(fitr.count());
  mc::vector<int> vints;
  for ( int i = 0; i < (int)80; i++ )
    vints.emplace_back(i * 3);
  mc::iterator<int> ints(vints);
  mc::io::stdoutln(*ints.nth(20));
  for ( auto n : vints )
    mc::io::stdout(n, " ");
  mc::io::stdout("\n");
  ints.for_each(dbl);
  for ( auto n : vints )
    mc::io::stdout(n, " ");
  mc::io::stdout("\n");

  for ( size_t i = 0; i < vints.size(); i++ )
    vints[i] = i;
  ints = mc::iterator<int>(vints);
  ints.skip(skip2);
  mc::io::stdoutln(*ints.peek());
  ints.skip(skip2);
  mc::io::stdoutln(*ints.peek());
  ints.skip(skip2);
  mc::io::stdoutln(*ints.peek());
}
