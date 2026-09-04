//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// The C-string layer: unicode validators, strcmp, and istring.
//
// Two traps this file pins:
//
//   u8_check takes `const char *` (src/string/unistring.hpp:157) while u16_check/u32_check take
//   their matching char16_t/char32_t. That asymmetry is deliberate -- u8_check has const char*
//   callers throughout tests/rigor -- so a unicode8 buffer is cast at the call site.
//
//   The validators answer the END pointer on well-formed input and nullptr on malformed input
//   (unistring.hpp:156-225). It is the inverse of the usual "null means fine" reading, and asserting
//   it the wrong way round is silent: the old form of this test printed `0` three times and passed.

#include "../../src/io/console.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/string/unistring.hpp"

#include "../../src/std.hpp"
#include "../../src/string/istring.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  test_case("well-formed unicode passes every validator");
  {
    const unicode8 *a8 = u8"fdgjfidgiodfg";
    const unicode16 *a16 = u"fdgjfidgiodfg";
    const unicode32 *a32 = U"fdgjfidgiodfg";

    const char *c8 = reinterpret_cast<const char *>(a8);
    require_true(micron::u8_check(c8, micron::strlen(a8)) == c8 + 13);
    require_true(micron::u16_check(a16, micron::u16strlen(a16)) == a16 + 13);
    require_true(micron::u32_check(a32, micron::ustrlen(a32)) == a32 + 13);

    require_true(micron::strlen(a8) == 13);
    require_true(micron::u16strlen(a16) == 13);
    require_true(micron::ustrlen(a32) == 13);
  }
  end_test_case();

  test_case("malformed input is rejected by every validator");
  {
    // 0xE2 opens a three-byte sequence and only one continuation byte follows
    const char trunc[] = { '\xE2', '\x82', '\0' };
    require_true(micron::u8_check(trunc, 2) == nullptr);

    // a continuation byte with no lead, and an over-long two-byte encoding of '/'
    const char orphan[] = { '\x82', '\0' };
    require_true(micron::u8_check(orphan, 1) == nullptr);
    const char overlong[] = { '\xC0', '\xAF', '\0' };
    require_true(micron::u8_check(overlong, 2) == nullptr);

    // an unpaired high surrogate, and a lone low surrogate
    const char16_t hi[] = { 0xD800, 0x0041, 0 };
    require_true(micron::u16_check(hi, 2) == nullptr);
    const char16_t lo[] = { 0xDC00, 0 };
    require_true(micron::u16_check(lo, 1) == nullptr);

    // a surrogate code point and one past the Unicode range
    const char32_t sur[] = { 0x0000D800u, 0 };
    require_true(micron::u32_check(sur, 1) == nullptr);
    const char32_t over[] = { 0x00110000u, 0 };
    require_true(micron::u32_check(over, 1) == nullptr);

    // a well-formed surrogate PAIR is valid utf-16 and must survive
    const char16_t pair[] = { 0xD83D, 0xDE00, 0 };
    require_true(micron::u16_check(pair, 2) == pair + 2);
  }
  end_test_case();

  test_case("strcmp orders and reports equality");
  {
    const char *a = "fdgjfidgiodfg";
    const char *b = "fdgjfidgiodfg";
    const char *c = "dfg";

    require_true(mc::strcmp(a, b) == 0);
    require_true(mc::strcmp(b, c) > 0);      // 'f' > 'd'
    require_true(mc::strcmp(c, b) < 0);
  }
  end_test_case();

  test_case("istring is immutable: append and += both answer a new string");
  {
    mc::istring ic = "Hello!";
    require_true(ic.find('H') == 0);

    auto id = ic.append("hi");
    require_true(mc::strcmp(id.c_str(), "Hello!hi") == 0);
    require_true(mc::strcmp(ic.c_str(), "Hello!") == 0);      // the receiver did NOT grow

    // += is the trap: on an istring it is not a mutation, it yields a new value and
    // leaves the left operand exactly as it was
    auto ie = id += "bye";
    require_true(mc::strcmp(ie.c_str(), "Hello!hibye") == 0);
    require_true(mc::strcmp(id.c_str(), "Hello!hi") == 0);
  }
  end_test_case();

  sb::print("=== ALL CSTRING TESTS PASSED ===");
  return 1;
}
