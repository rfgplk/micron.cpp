//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/algorithm/find.hpp"
#include "../../src/linux/sys/sysinfo.hpp"
#include "../../src/memory/mman.hpp"

#include "../snowball/snowball.hpp"

namespace
{
struct scalar_value {
  u8 value;

  constexpr bool operator==(const scalar_value &) const = default;
};

constexpr bool
constant_checks()
{
  constexpr char a[] = "number";
  constexpr char b[] = "numbers";
  return micron::equal(a, a, b, b) && micron::equal(a, a + 6, b, b + 6) && !micron::equal(a, a + 6, b, b + 7)
         && !micron::equal(a, a + 7, b, b + 7);
}

static_assert(constant_checks());

[[gnu::noinline]] bool
short_option(const char *p, usize n)
{
  constexpr char number[] = "number", nu[] = "nu";
  return micron::equal(p, p + n, number, number + 6) || micron::equal(p, p + n, nu, nu + 2);
}

template<typename T>
void
check_alignments()
{
  alignas(32) T a[128]{}, b[128]{};
  for ( usize alignment = 0; alignment < 32 / sizeof(T); ++alignment ) {
    T *left = a + alignment;
    T *right = b + (alignment * 7) % (32 / sizeof(T));
    for ( usize n = 0; n <= 65; ++n ) {
      for ( usize i = 0; i <= n; ++i ) left[i] = right[i] = T{ static_cast<u8>((i * 29 + n) & 255) };
      right[n] = T{ static_cast<u8>((n * 29 + n + 1) & 255) };
      sb::require_true(micron::equal(left, left + n, right, right + n));
      sb::require_false(micron::equal(left, left + n, right, right + n + 1));
      for ( usize at = 0; at < n; ++at ) {
        const T saved = right[at];
        right[at] = T{ static_cast<u8>((at * 29 + n + 1) & 255) };
        sb::require_false(micron::equal(left, left + n, right));
        right[at] = saved;
      }
    }
  }
}

struct guarded_page {
  usize page = micron::getpagesizelive();
  char *mapping
      = reinterpret_cast<char *>(micron::mmap(nullptr, page * 3, micron::prot_none, micron::map_private | micron::map_anonymous, -1, 0));

  guarded_page()
  {
    sb::require_true(!micron::mmap_failed(mapping));
    sb::require_true(micron::mprotect(mapping + page, page, micron::prot_read | micron::prot_write) == 0);
  }

  ~guarded_page() { micron::munmap(reinterpret_cast<addr_t *>(mapping), page * 3); }
};
}      // namespace

int
main()
{
  sb::test_case("equal preserves short literal and constexpr range semantics");
  sb::require_true(constant_checks());
  sb::require_true(short_option("number", 6) && short_option("nu", 2));
  sb::require_false(short_option("numbers", 7) || short_option("", 0) || short_option("syntax", 6));
  sb::require_true(micron::__impl::scan_equal<char>(nullptr, nullptr, 0));
  sb::end_test_case();

  sb::test_case("equal covers every eight-element tail, mismatch position and lane alignment");
  check_alignments<u8>();
  check_alignments<u16>();
  check_alignments<u32>();
  check_alignments<u64>();
  check_alignments<scalar_value>();
  sb::end_test_case();

  sb::test_case("equal never reads before or after its accessible ranges");
  {
    guarded_page a, b;
    for ( usize n = 0; n <= 129; ++n ) {
      for ( const bool at_end : { false, true } ) {
        char *left = a.mapping + a.page + (at_end ? a.page - n : 0);
        char *right = b.mapping + b.page + (at_end ? b.page - n : 0);
        for ( usize i = 0; i < n; ++i ) left[i] = right[i] = static_cast<char>(i * 17 + n);
        sb::require_true(micron::equal(left, left + n, right, right + n));
        if ( n ) {
          right[n - 1] ^= 1;
          sb::require_false(micron::equal(left, left + n, right));
          right[n - 1] ^= 1;
          right[0] ^= 1;
          sb::require_false(micron::equal(left, left + n, right));
        }
      }
    }
  }
  sb::end_test_case();
  return 1;
}
