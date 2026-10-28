//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/memory_block.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__ubsan_default_options()
{
  return "halt_on_error=1:exitcode=86";
}

static void
check_fill(const micron::memory_block<> &block, byte value)
{
  for ( usize i = 0; i < block.size(); ++i ) sb::require_true(block[i] == value);
}

static void
allocated_blocks()
{
  micron::memory_block<> block(17);
  block = byte{ 0x5a };
  check_fill(block, byte{ 0x5a });
  micron::memory_block<> copy(block);
  check_fill(copy, byte{ 0x5a });
  block = byte{ 0xa5 };
  check_fill(block, byte{ 0xa5 });
  check_fill(copy, byte{ 0x5a });
  copy = block;
  check_fill(copy, byte{ 0xa5 });
  block.resize(4096);
  block = byte{ 0x42 };
  check_fill(block, byte{ 0x42 });
}

static void
empty_blocks()
{
  micron::memory_block<> zero(0);
  zero = byte{ 0x5a };
  sb::require_true(zero.size() == 0 && zero.begin() == zero.end());
  micron::memory_block<> null(micron::chunk<byte>{ nullptr, 0 });
  null = byte{ 0xa5 };
  sb::require_true(null.data() == nullptr && null.size() == 0);
  micron::memory_block<> null_copy(null);
  null_copy = byte{ 0x42 };
  sb::require_true(null_copy.size() == 0);

  micron::memory_block<> source(23);
  source = byte{ 0x17 };
  micron::memory_block<> destination(micron::move(source));
  source = byte{ 0x00 };
  sb::require_true(source.data() == nullptr && source.size() == 0);
  check_fill(destination, byte{ 0x17 });
  source.resize(8);
  source = byte{ 0x25 };
  check_fill(source, byte{ 0x25 });
  source = null;
  source = byte{ 0x99 };
  sb::require_true(source.size() == 0);
  destination.recreate(0);
  destination = byte{ 0x81 };
  sb::require_true(destination.size() == 0);
}

int
main(int argc, char **argv)
{
  if ( argc > 1 ) {
    if ( argv[1][0] == 'a' )
      allocated_blocks();
    else
      empty_blocks();
    return 1;
  }
  sb::test_case("memory blocks retain ordinary fill and copy semantics");
  allocated_blocks();
  sb::end_test_case();
  sb::test_case("zero-size and moved-from memory blocks accept byte fills");
  empty_blocks();
  sb::end_test_case();
  return 1;
}
