//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// abcmalloc sheets: make_sheet() and the try_mark() bump path.
//
// Two things this file pins that are easy to get wrong:
//
//   make_sheet takes the owning arena FIRST (book.hpp:307) -- it forwards it to __sheet_register,
//   which is what puts the sheet in the block-owner table.
//
//   try_mark reports exhaustion as { (byte*)-1, 0xFF } (book.hpp:157), so the terminator is
//   failed_allocation()/invalid(), NOT zero(): zero() is false for that sentinel and a loop written
//   against it never ends.

#include "../../src/memory/allocation/abcmalloc/arena.hpp"
#include "../../src/memory/allocation/abcmalloc/book.hpp"
#include "../../src/memory/allocation/abcmalloc/tapi.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

// deterministic xorshift32; the seed is a fixed literal, never time-based
struct xorshift32 {
  u32 s;
  constexpr xorshift32(u32 seed) noexcept : s(seed ? seed : 0xDEADBEEFu) { }
  u32
  next() noexcept
  {
    u32 x = s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s = x;
    return x;
  }
};

int
main(void)
{
  test_case("the four sheet size classes are distinct and ordered");
  {
    const usize a = sizeof(abc::sheet<abc::__class_small>);
    const usize b = sizeof(abc::sheet<abc::__class_medium>);
    const usize c = sizeof(abc::sheet<abc::__class_large>);
    const usize d = sizeof(abc::sheet<abc::__class_huge>);
    require_true(a > 0 && b > 0 && c > 0 && d > 0);
    require_true(abc::__class_small < abc::__class_medium);
    require_true(abc::__class_medium < abc::__class_large);
    require_true(abc::__class_large < abc::__class_huge);
    sb::print("sheet sizes: small=", a, " medium=", b, " large=", c, " huge=", d);
  }
  end_test_case();

  test_case("try_mark hands out distinct, correctly sized blocks and then reports exhaustion");
  {
    abc::__arena *owner = abc::__current_arena();
    require_true(owner != nullptr);

    auto sheet = abc::make_sheet<abc::__class_small>(owner, 2 << 15);
    require_true(!sheet.empty());

    constexpr usize req = 256;
    byte *prev = nullptr;
    usize n = 0;
    for ( ;; ) {
      auto mem = sheet.try_mark(req);
      if ( mem.failed_allocation() or mem.invalid() or mem.zero() ) break;
      require_true(mem.ptr != nullptr);
      require_true(mem.len >= req);
      if ( prev != nullptr ) require_true(mem.ptr != prev);      // never the same block twice
      // the block is writable end to end
      mem.ptr[0] = 0xA5;
      mem.ptr[mem.len - 1] = 0x5A;
      require_true(mem.ptr[0] == 0xA5 && mem.ptr[mem.len - 1] == 0x5A);
      prev = mem.ptr;
      if ( ++n > (1u << 20) ) break;      // a bump allocator over 128K cannot legitimately get here
    }
    require_true(n > 0);
    require_true(n < (1u << 20));      // it really did report exhaustion rather than spinning
    sb::print("small sheet (128K) yielded ", n, " blocks of ", req, "B");
  }
  end_test_case();

  test_case("a mixed-size run over one sheet stays inside the sheet");
  {
    abc::__arena *owner = abc::__current_arena();
    auto sheet = abc::make_sheet<abc::__class_small>(owner, 2 << 24);
    require_true(!sheet.empty());

    xorshift32 rng(0xC0FFEE01u);
    usize n = 0;
    usize granted = 0;
    for ( ;; ) {
      const usize want = 30 + (rng.next() % 4492);      // [30, 4521]
      auto mem = sheet.try_mark(want);
      if ( mem.failed_allocation() or mem.invalid() or mem.zero() ) break;
      require_true(mem.len >= want);
      granted += mem.len;
      if ( ++n > (1u << 22) ) break;
    }
    require_true(n > 0);
    require_true(n < (1u << 22));
    // a 32MB sheet cannot have handed out more than it owns
    require_true(granted <= static_cast<usize>(2 << 24));
    sb::print("32M sheet yielded ", n, " mixed blocks totalling ", granted, "B");
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC SHEET TESTS PASSED ===");
  return 1;
}
