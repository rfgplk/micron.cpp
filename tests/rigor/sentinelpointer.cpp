//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/memory/pointers/sentinel.hpp"
#include "../src/std.hpp"
#include "../src/string/string.hpp"

#include "../snowball/snowball.hpp"

#include <cstdint>

// ─── main ────────────────────────────────────────────────────────────────────

int
main(void)
{
  sb::print("=== SENTINEL POINTER TESTS ===");

  // ── construction ─────────────────────────────────────────────────────────

  sb::test_case("construction - from byte*: stores address correctly");
  {
    byte buf[4] = {};
    micron::sentinel_pointer s(buf);
    sb::require(static_cast<bool>(s));
    sb::require(s() == buf);
  }
  sb::end_test_case();

  sb::test_case("construction - from uintptr_t: stores address correctly");
  {
    byte buf[4] = {};
    uintptr_t addr = reinterpret_cast<uintptr_t>(buf);
    micron::sentinel_pointer s(addr);
    sb::require(static_cast<bool>(s));
    sb::require(s() == reinterpret_cast<byte *>(addr));
  }
  sb::end_test_case();

  sb::test_case("construction - from nullptr byte*: null sentinel");
  {
    micron::sentinel_pointer s(static_cast<byte *>(nullptr));
    sb::require(!s);
    sb::require(s() == nullptr);
  }
  sb::end_test_case();

  sb::test_case("construction - from uintptr_t 0: null sentinel");
  {
    micron::sentinel_pointer s(uintptr_t(0));
    sb::require(!s);
  }
  sb::end_test_case();

  sb::test_case("construction - two sentinels on same address: both equal");
  {
    byte buf[1] = {};
    micron::sentinel_pointer a(buf);
    micron::sentinel_pointer b(buf);
    sb::require(a() == b());
  }
  sb::end_test_case();

  sb::test_case("construction - two sentinels on different addresses: not equal");
  {
    byte buf_a[1] = {};
    byte buf_b[1] = {};
    micron::sentinel_pointer a(buf_a);
    micron::sentinel_pointer b(buf_b);
    sb::require(a() != b());
  }
  sb::end_test_case();

  // ── operator bool / operator! ─────────────────────────────────────────────

  sb::test_case("operator bool - true for non-null sentinel");
  {
    byte buf[1] = {};
    micron::sentinel_pointer s(buf);
    sb::require(static_cast<bool>(s) == true);
  }
  sb::end_test_case();

  sb::test_case("operator bool - false for null sentinel");
  {
    micron::sentinel_pointer s(static_cast<byte *>(nullptr));
    sb::require(static_cast<bool>(s) == false);
  }
  sb::end_test_case();

  sb::test_case("operator! - true for null sentinel");
  {
    micron::sentinel_pointer s(static_cast<byte *>(nullptr));
    sb::require(!s == true);
  }
  sb::end_test_case();

  sb::test_case("operator! - false for non-null sentinel");
  {
    byte buf[1] = {};
    micron::sentinel_pointer s(buf);
    sb::require(!s == false);
  }
  sb::end_test_case();

  // ── operator() ───────────────────────────────────────────────────────────

  sb::test_case("operator() - returns stored address");
  {
    byte buf[8] = {};
    micron::sentinel_pointer s(buf);
    sb::require(s() == buf);
  }
  sb::end_test_case();

  sb::test_case("operator() - address stable across multiple calls");
  {
    byte buf[4] = {};
    micron::sentinel_pointer s(buf);
    sb::require(s() == s());
  }
  sb::end_test_case();

  sb::test_case("operator() - uintptr_t round-trip preserves address");
  {
    byte buf[4] = {};
    uintptr_t addr = reinterpret_cast<uintptr_t>(buf);
    micron::sentinel_pointer s(addr);
    sb::require(reinterpret_cast<uintptr_t>(s()) == addr);
  }
  sb::end_test_case();

  // ── operator== ───────────────────────────────────────────────────────────

  sb::test_case("operator== (void*) - matches same address");
  {
    byte buf[4] = {};
    micron::sentinel_pointer s(buf);
    sb::require(s == static_cast<void *>(buf));
  }
  sb::end_test_case();

  sb::test_case("operator== (void*) - does not match different address");
  {
    byte buf_a[4] = {};
    byte buf_b[4] = {};
    micron::sentinel_pointer s(buf_a);
    sb::require(!(s == static_cast<void *>(buf_b)));
  }
  sb::end_test_case();

  sb::test_case("operator== (uintptr_t) - matches same numeric address");
  {
    byte buf[4] = {};
    uintptr_t addr = reinterpret_cast<uintptr_t>(buf);
    micron::sentinel_pointer s(buf);
    sb::require(s == addr);
  }
  sb::end_test_case();

  sb::test_case("operator== (uintptr_t) - does not match different numeric address");
  {
    byte buf[4] = {};
    micron::sentinel_pointer s(buf);
    sb::require(!(s == uintptr_t(0)));
  }
  sb::end_test_case();

  sb::test_case("operator== (uintptr_t 0) - null sentinel matches 0");
  {
    micron::sentinel_pointer s(uintptr_t(0));
    sb::require(s == uintptr_t(0));
  }
  sb::end_test_case();

  // ── operator* ────────────────────────────────────────────────────────────

  sb::test_case("operator* - reads byte at sentinel address");
  {
    byte buf[4] = { 0xAB, 0x00, 0x00, 0x00 };
    micron::sentinel_pointer s(buf);
    sb::require(*s == byte(0xAB));
  }
  sb::end_test_case();

  sb::test_case("operator* - sentinel does not affect the underlying value");
  {
    byte buf[4] = { 0x01, 0x02, 0x03, 0x04 };
    micron::sentinel_pointer s(buf);
    (void)*s;     // read only — must not modify
    sb::require(buf[0] == byte(0x01));
    sb::require(buf[1] == byte(0x02));
  }
  sb::end_test_case();

  // ── primary use case: checking against sentinel values ───────────────────

  sb::test_case("sentinel use - detect end-of-buffer address");
  {
    byte buf[16] = {};
    byte *end = buf + 16;
    micron::sentinel_pointer sentinel(end);

    byte *cursor = buf;
    while ( !(sentinel == static_cast<void *>(cursor)) )
      ++cursor;

    sb::require(cursor == end);
  }
  sb::end_test_case();

  sb::test_case("sentinel use - detect specific address inside array");
  {
    byte buf[8] = {};
    micron::sentinel_pointer sentinel(buf + 4);

    bool found = false;
    for ( byte *p = buf; p < buf + 8; ++p ) {
      if ( sentinel == static_cast<void *>(p) ) {
        found = true;
        break;
      }
    }
    sb::require(found);
  }
  sb::end_test_case();

  sb::test_case("sentinel use - null sentinel never matches valid pointer");
  {
    byte buf[4] = {};
    micron::sentinel_pointer null_sentinel(static_cast<byte *>(nullptr));

    bool matched = false;
    for ( byte *p = buf; p < buf + 4; ++p )
      if ( null_sentinel == static_cast<void *>(p) )
        matched = true;

    sb::require(matched == false);
  }
  sb::end_test_case();

  sb::test_case("sentinel use - uintptr_t sentinel compared against runtime address");
  {
    byte buf[4] = {};
    uintptr_t magic = reinterpret_cast<uintptr_t>(buf + 2);
    micron::sentinel_pointer sentinel(magic);

    sb::require(sentinel == static_cast<void *>(buf + 2));
    sb::require(!(sentinel == static_cast<void *>(buf + 0)));
    sb::require(!(sentinel == static_cast<void *>(buf + 1)));
    sb::require(!(sentinel == static_cast<void *>(buf + 3)));
  }
  sb::end_test_case();

  sb::test_case("sentinel use - two sentinels guard start and end of region");
  {
    byte buf[16] = {};
    micron::sentinel_pointer begin_sentinel(buf);
    micron::sentinel_pointer end_sentinel(buf + 16);

    sb::require(begin_sentinel == static_cast<void *>(buf));
    sb::require(end_sentinel == static_cast<void *>(buf + 16));
    sb::require(!(begin_sentinel == static_cast<void *>(buf + 16)));
    sb::require(!(end_sentinel == static_cast<void *>(buf)));
  }
  sb::end_test_case();

  sb::test_case("sentinel use - immutability: underlying buffer unaffected by sentinel");
  {
    byte buf[4] = { 10, 20, 30, 40 };
    {
      micron::sentinel_pointer s(buf);
      (void)s();
      (void)static_cast<bool>(s);
      (void)(s == static_cast<void *>(buf));
    }
    // Buffer must be exactly as initialised
    sb::require(buf[0] == byte(10));
    sb::require(buf[1] == byte(20));
    sb::require(buf[2] == byte(30));
    sb::require(buf[3] == byte(40));
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}
