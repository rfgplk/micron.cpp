//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// abcmalloc's user-facing entries: alloc / query_size / fetch<T> / dealloc, and placement new on
// top of an abc block.
//
// NOTE: abc::freeze() is NOT a free -- it forwards to freeze_sheet(), which mprotects the whole
// SHEET the pointer lives in read-only (malloc.hpp:287-311). Calling it on a block and then
// allocating again lands in the frozen sheet and SIGSEGVs on the first write. The deallocator is
// abc::dealloc().
//
// NOTE: the optimization barrier is `byte *volatile p` -- a VOLATILE POINTER -- not `volatile byte
// *p`, which is a pointer to volatile. The latter is what every sibling here avoids, and it does
// not merely read oddly: printk's pointer overload (io/echo.hpp:369) static_casts to const void*,
// which cannot strip the pointee's volatile, so `console(p)` is a hard compile error.

#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

struct s {
  int x;
  int y;
};

int
main(void)
{
  test_case("alloc grants at least what was asked for, and query_size agrees");
  {
    byte *buf = abc::alloc(65536);
    require_true(buf != nullptr);

    const usize granted = abc::query_size(buf);
    require_true(granted >= 65536);

    // the whole granted extent is writable
    micron::byteset(buf, 0xC3, granted);
    require_true(buf[0] == 0xC3 && buf[granted - 1] == 0xC3);

    sb::print("requested 65536B, query_size answers ", granted, "B");
    abc::dealloc(buf);
  }
  end_test_case();

  test_case("placement new on an abc block survives an allocation storm around it");
  {
    byte *buf = abc::alloc(65536);
    require_true(buf != nullptr);

    mc::sstr<32> *tst = new (buf) mc::sstr<32>("Hello World!");
    require_true(mc::strcmp(tst->c_str(), "Hello World!") == 0);

    byte *volatile p = nullptr;      // volatile POINTER: the barrier, and the printable spelling
    for ( int i = 0; i < 1000; i++ ) {
      p = abc::alloc(65536);
      require_true(p != nullptr);
    }
    require_true(p != nullptr);

    // 1000 further 64K allocations must not have touched a live block
    require_true(mc::strcmp(tst->c_str(), "Hello World!") == 0);
    abc::dealloc(buf);
  }
  end_test_case();

  test_case("fetch<T> hands back a typed, writable object");
  {
    auto st = abc::fetch<s>();
    require_true(st != nullptr);
    st->x = 5;
    st->y = 10;
    require_true(st->x == 5 && st->y == 10);
    require_true(abc::query_size(st) >= sizeof(s));
    abc::dealloc(st);
  }
  end_test_case();

  test_case("a randomized malloc run never hands back the same live block twice");
  {
    // fixed literal seed, never time-based
    u32 rs = 0x9E3779B9u;
    auto next = [&rs]() noexcept -> u32 {
      rs ^= rs << 13;
      rs ^= rs >> 17;
      rs ^= rs << 5;
      return rs;
    };

    constexpr usize N = 512;
    byte *live[N] = {};
    for ( usize n = 0; n < N; ++n ) {
      live[n] = static_cast<byte *>(abc::malloc(1 + (next() % 1000000u)));
      require_true(live[n] != nullptr);
      *live[n] = static_cast<byte>(n & 0xFF);
    }
    // every live pointer is distinct and still holds what was written into it
    u32 dupes = 0;
    for ( usize i = 0; i < N; ++i ) {
      require_true(*live[i] == static_cast<byte>(i & 0xFF));
      for ( usize j = i + 1; j < N; ++j )
        if ( live[i] == live[j] ) ++dupes;
    }
    require_true(dupes == 0);
    for ( usize n = 0; n < N; ++n ) abc::free(live[n]);
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC MAIN TESTS PASSED ===");
  return 1;
}
