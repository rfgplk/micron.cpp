//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: abcmalloc's out-of-process inspection descriptor must build under a config
// where two tiers share a sheet count. Not run.
//
// forcing precise == large collides the two tiers' __tier<> geometry on EVERY profile, so this gate
// does not depend on __ABC_EMBED to reach the defect. that collision is the assertion -- "it compiles"
// is not. before the fix this file failed on all four arches at inspect.hpp's tier table check and
// nowhere else: 3112 != 3112 on amd64/arm64, 1556 != 1556 on i386/arm32
#define MICRON_ABC_MAX_SHEETS_PRECISE 128
#define MICRON_ABC_MAX_SHEETS_LARGE 128

#include "../../src/cmalloc.hpp"

static_assert(abc::abc_inspect_valid(&abc::__ins::__self), "the descriptor must satisfy its own handshake");
static_assert(abc::__ins::__self.tiers[0].off_count == abc::__ins::__self.tiers[3].off_count,
              "this gate is pointless unless the two tiers actually collide");

int
main()
{
  return abc::__abc_inspect.n_tiers;
}
