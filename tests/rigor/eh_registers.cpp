//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/types.hpp"
#include "../snowball/snowball.hpp"

#if defined(__cpp_exceptions)
namespace
{
[[gnu::noinline]] void
throw_value(int value)
{
  throw value;
}

[[gnu::noinline]] bool
preserved(usize value, bool rethrow)
{
#if defined(__micron_arch_arm32)
  register usize held asm("r4") = value;
#else
  usize held = value;
#endif
  asm volatile("" : "+r"(held) : : "memory");
  int caught = 0;
  try {
    if ( rethrow ) {
      try {
        throw_value(99);
      } catch ( ... ) {
        throw;
      }
    } else
      throw_value(99);
  } catch ( int error ) {
    caught = error;
  }
  asm volatile("" : "+r"(held) : : "memory");
  return held == value && caught == 99;
}
}      // namespace

int
main()
{
  sb::test_case("throw and catch preserve live callee-saved registers");
  for ( const usize value : { usize(1), usize(0x12345678), usize(0x7abcdef0), usize(0xffffffff) } )
    sb::require_true(preserved(value, false) && preserved(value, true));
  sb::end_test_case();
  return 1;
}
#else
int
main()
{
  sb::skip("callee-saved exception registers require exception support");
  return 1;
}
#endif
