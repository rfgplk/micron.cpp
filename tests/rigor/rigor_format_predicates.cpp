// rigor_format_predicates.cpp — exhaustive snowball suite for
// character classification + transformation predicates in src/string/format.hpp.
//
// Coverage (in micron::format namespace):
//   isupper / islower
//   isalpha / isalnum
//   isdigit / isxdigit
//   isspace / isblank
//   iscntrl / isprint / isgraph / ispunct / isascii
//   to_upper / to_lower
//
// Each predicate is exercised across the full byte range [0, 256) by
// comparing to a naive ASCII-spec reference, and run for ≥10k random
// invocations as a property test.
//
// micron's predicates take `requires is_fundamental_v<T>` and return
// `false` for any T that isn't `char` (they're char-only despite the
// templated signature). The tests reflect that — assertions only hold for
// char.

#include "../../src/string/format.hpp"

#include "../support/format_rigor.hpp"

using namespace mtest::format_rigor;
using mtest::prng;
namespace fmt = micron::format;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== FORMAT/PREDICATES RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // Full-byte-range agreement vs naive reference
  // (256 chars × 13 predicates = 3328 deterministic comparisons)
  // ════════════════════════════════════════════════════════════════════

  test_case("isupper agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isupper(c), ref::isupper(c));
    }
  }
  end_test_case();

  test_case("islower agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::islower(c), ref::islower(c));
    }
  }
  end_test_case();

  test_case("isalpha agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isalpha(c), ref::isalpha(c));
    }
  }
  end_test_case();

  test_case("isdigit agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isdigit(c), ref::isdigit(c));
    }
  }
  end_test_case();

  test_case("isalnum agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isalnum(c), ref::isalnum(c));
    }
  }
  end_test_case();

  test_case("isxdigit agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isxdigit(c), ref::isxdigit(c));
    }
  }
  end_test_case();

  test_case("isspace agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isspace(c), ref::isspace(c));
    }
  }
  end_test_case();

  test_case("isblank agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isblank(c), ref::isblank(c));
    }
  }
  end_test_case();

  test_case("iscntrl agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::iscntrl(c), ref::iscntrl(c));
    }
  }
  end_test_case();

  test_case("isprint agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isprint(c), ref::isprint(c));
    }
  }
  end_test_case();

  test_case("isgraph agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isgraph(c), ref::isgraph(c));
    }
  }
  end_test_case();

  test_case("ispunct agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::ispunct(c), ref::ispunct(c));
    }
  }
  end_test_case();

  test_case("isascii agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isascii(c), ref::isascii(c));
    }
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Transformations
  // ════════════════════════════════════════════════════════════════════

  test_case("to_upper agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::to_upper(c), ref::to_upper(c));
    }
  }
  end_test_case();

  test_case("to_lower agrees with naive over full byte range");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::to_lower(c), ref::to_lower(c));
    }
  }
  end_test_case();

  test_case("to_upper o to_lower is identity on ASCII alpha");
  {
    for ( char c = 'a'; c <= 'z'; ++c ) require(fmt::to_lower(fmt::to_upper(c)), c);
    for ( char c = 'A'; c <= 'Z'; ++c ) require(fmt::to_upper(fmt::to_lower(c)), c);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Partition properties — each char satisfies disjoint categories
  // ════════════════════════════════════════════════════════════════════

  test_case("isalpha == isupper or islower");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isalpha(c), (fmt::isupper(c) || fmt::islower(c)));
    }
  }
  end_test_case();

  test_case("isalnum == isalpha or isdigit");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      require(fmt::isalnum(c), (fmt::isalpha(c) || fmt::isdigit(c)));
    }
  }
  end_test_case();

  test_case("isxdigit superset of isdigit");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      if ( fmt::isdigit(c) ) require_true(fmt::isxdigit(c));
    }
  }
  end_test_case();

  test_case("isgraph == isprint and !isspace");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      // The micron definition: isgraph = [0x21..0x7E]; isprint = [0x20..0x7E].
      // So isgraph == isprint && c != ' '.
      bool ig = fmt::isgraph(c);
      bool ip_no_space = fmt::isprint(c) && c != ' ';
      require(ig, ip_no_space);
    }
  }
  end_test_case();

  test_case("iscntrl is disjoint from isprint");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      // For ASCII (0..0x7F) every char is either cntrl or print, exclusively.
      if ( ref::isascii(c) ) require_false(fmt::iscntrl(c) && fmt::isprint(c));
    }
  }
  end_test_case();

  test_case("ispunct excludes alnum and space");
  {
    for ( int i = 0; i < 256; ++i ) {
      char c = static_cast<char>(i);
      if ( fmt::ispunct(c) ) {
        require_false(fmt::isalnum(c));
        require_false(fmt::isspace(c));
      }
    }
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // Property tests — 10k random bytes
  // ════════════════════════════════════════════════════════════════════

  property_test(
      "isupper vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isupper(c), ref::isupper(c));
      },
      10000);

  property_test(
      "islower vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::islower(c), ref::islower(c));
      },
      10000);

  property_test(
      "isalpha vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isalpha(c), ref::isalpha(c));
      },
      10000);

  property_test(
      "isdigit vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isdigit(c), ref::isdigit(c));
      },
      10000);

  property_test(
      "isalnum vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isalnum(c), ref::isalnum(c));
      },
      10000);

  property_test(
      "isxdigit vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isxdigit(c), ref::isxdigit(c));
      },
      10000);

  property_test(
      "isspace vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isspace(c), ref::isspace(c));
      },
      10000);

  property_test(
      "isblank vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isblank(c), ref::isblank(c));
      },
      10000);

  property_test(
      "iscntrl vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::iscntrl(c), ref::iscntrl(c));
      },
      10000);

  property_test(
      "isprint vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isprint(c), ref::isprint(c));
      },
      10000);

  property_test(
      "isgraph vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::isgraph(c), ref::isgraph(c));
      },
      10000);

  property_test(
      "ispunct vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::ispunct(c), ref::ispunct(c));
      },
      10000);

  property_test(
      "to_upper vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::to_upper(c), ref::to_upper(c));
      },
      10000);

  property_test(
      "to_lower vs naive (10k random bytes)",
      [](u32 raw) {
        char c = static_cast<char>(raw & 0xff);
        require(fmt::to_lower(c), ref::to_lower(c));
      },
      10000);

  property_test(
      "to_upper -> to_lower roundtrip is identity for letters (10k)",
      [](u32 raw) {
        // produce only letters
        char c = (raw & 1u) ? static_cast<char>('a' + (raw >> 1) % 26) : static_cast<char>('A' + (raw >> 1) % 26);
        require(fmt::to_lower(fmt::to_upper(c)), fmt::islower(c) ? c : static_cast<char>(c + 32));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // char* to_upper / to_lower (in-place string transformation)
  // ════════════════════════════════════════════════════════════════════

  test_case("to_upper(char*) uppercases letters, leaves others");
  {
    char s[16] = "Hello, World!";
    fmt::to_upper(s);
    require_true(bytes_equal(s, 13, "HELLO, WORLD!", 13));
  }
  end_test_case();

  test_case("to_lower(char*) lowercases letters");
  {
    char s[16] = "Hello, World!";
    fmt::to_lower(s);
    require_true(bytes_equal(s, 13, "hello, world!", 13));
  }
  end_test_case();

  test_case("to_upper(char*) null pointer is no-op");
  {
    char *p = nullptr;
    fmt::to_upper(p);      // must not crash
    fmt::to_lower(p);      // must not crash
    require_true(true);
  }
  end_test_case();

  test_case("to_upper(char*) empty string is no-op");
  {
    char s[1] = "";
    fmt::to_upper(s);
    require(s[0], '\0');
  }
  end_test_case();

  property_test(
      "to_upper(char*) idempotent (10k random alpha strings)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x1f) + 1;
        char buf[33] = {};
        prng rng(raw_seed + 7);
        fill_random_alpha(buf, n, rng);
        char a[34];
        char b[34];
        for ( usize i = 0; i < n; ++i ) {
          a[i] = buf[i];
          b[i] = buf[i];
        }
        a[n] = '\0';
        b[n] = '\0';
        fmt::to_upper(a);
        fmt::to_upper(a);      // applying twice == once
        fmt::to_upper(b);
        require_true(bytes_equal(a, n, b, n));
      },
      10000);

  sb::print("=== FORMAT/PREDICATES RIGOR SUITE PASSED ===");
  return 1;
}
