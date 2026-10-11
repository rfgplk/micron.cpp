//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: a buddy-tier cache must not grant a small allocation a block
// several orders larger. Skipped oversized entries remain available for a
// matching request, and adequate cached blocks still obey ownership rules.

#include "../../src/memory/allocation/abcmalloc/malloc.hpp"

#include "../snowball/snowball.hpp"

namespace
{

usize failures = 0;

void
verify(bool ok)
{
  if ( !ok ) ++failures;
  sb::check(ok);
}

void
probe(usize small, usize large)
{
  byte *p = abc::alloc(large);
  verify(p != nullptr);
  if ( !p ) return;
  const usize large_grant = abc::query_size(p);
  verify(large_grant >= large);
  p[0] = 0x5a;
  p[large - 1] = 0xa5;
  abc::dealloc(p);

  byte *q = abc::alloc(small);
  verify(q != nullptr);
  if ( !q ) return;
  const usize small_grant = abc::query_size(q);
  verify(small_grant >= small);
  // next_pow2(small + header) - header is strictly below this bound.
  verify(small_grant <= 2 * small + abc::__hdr_offset - 1);
  q[0] = 0x19;
  q[small - 1] = 0xe7;

  byte *r = abc::alloc(large);
  verify(r != nullptr);
  if ( r ) {
    verify(r != q);
    verify(abc::query_size(r) >= large);
    if constexpr ( abc::__default_per_class_free_cache && !abc::__default_launder && !abc::__default_redzone )
      verify(r == p);      // the oversized entry was skipped, not lost or freed
    r[0] = 0x42;
    r[large - 1] = 0x24;
    verify(q[0] == byte(0x19) && q[small - 1] == byte(0xe7));
    abc::dealloc(r);
  }
  abc::dealloc(q);

  byte *again = abc::alloc(small);
  verify(again != nullptr);
  if ( again ) {
    verify(abc::query_size(again) >= small);
    verify(abc::query_size(again) <= 2 * small + abc::__hdr_offset - 1);
    if constexpr ( abc::__default_per_class_free_cache && !abc::__default_launder && !abc::__default_redzone ) verify(again == q);
    abc::dealloc(again);
  }
}

}      // namespace

int
main()
{
  sb::test_case("medium buddy cache keeps grants within the requested order");
  probe(abc::__class_medium, abc::__class_large);
  sb::end_test_case();

  sb::test_case("large buddy cache keeps grants within the requested order");
  probe(abc::__class_large + 1, abc::__class_huge);
  sb::end_test_case();

  sb::test_case("header-offset boundaries and same-order cached reuse remain valid");
  const usize sizes[]
      = { 8192 - abc::__hdr_offset, 8192 - abc::__hdr_offset + 1, 16384, 65536 - abc::__hdr_offset, 65536 - abc::__hdr_offset + 1, 131072 };
  for ( usize n : sizes ) probe(n, n <= abc::__class_large ? abc::__class_large : abc::__class_huge);
  sb::end_test_case();
  sb::require(failures == 0);
  return 1;
}
