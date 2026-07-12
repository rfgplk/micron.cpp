//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: simple allocation paths compile on every arch/opt --
// abcmalloc porcelain, the global new/delete shim, and an allocator-backed
// container. Not run.

#include "../../src/cmalloc.hpp"
#include "../../src/memory/new.hpp"
#include "../../src/vector.hpp"

int
main()
{
  int acc = 0;

  void *p = abc::malloc(1024);
  if ( p ) {
    *static_cast<volatile unsigned char *>(p) = 0xAB;
    abc::free(p);
    acc += 1;
  }

  int *q = new int(7);
  acc += *q;
  delete q;

  micron::vector<int> v;
  v.reserve(16);
  for ( int i = 0; i < 16; ++i ) v.push_back(i);
  acc += static_cast<int>(v.size());

  return acc & 0x7f;
}
