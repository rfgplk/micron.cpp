//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE arm64 FREESTANDING LINK GATE. THIS FILE EXISTS TO BE LINKED, NOT RUN.
//
// aarch64 is the only target micron builds whose `long double` is binary128: x86 lowers it to x87
// and armv7-a aliases it to `double`, so neither ever emits a TF-mode soft-float libcall. A
// freestanding link has no libgcc to supply those, which is what math/__gcc_fp128_syms.hpp is for --
// and for a long time that header's definitions were plain `weak`, so under -flto they looked
// unreferenced (a long double libcall is created at RTL expansion, AFTER LTO symbol resolution),
// were dropped, and the link failed on exactly the symbols the header exists to provide.
//
// No cell could see it. Every arm64 freestanding cell in both manifests passes --raw-obj: they
// compile, they never link. This file is the missing half, and it must be named EXPLICITLY by a
// `build` cell -- never reached by a directory sweep -- which is why it lives in tests/build/.
//
// WHY THE volatiles AND WHY NOT tests/compiletests/math.cpp: that file links clean on an unpatched
// tree, because LTO proves its long double work dead and deletes it before any libcall is emitted.
// Measured -- the resulting arm64 binary contains zero TF symbols. A gate for this has to keep the
// arithmetic reachable, which is what routing it through volatile globals does.
//
// Exercised here, in the order a link failure named them: multiply (__multf3), add (__addtf3),
// divide (__divtf3), widen from double (__extenddftf2), narrow back (__trunctfdf2), compare
// (__lttf2), and negate (__negtf2). The u64 round-trip pulls the integer conversions
// (__floatunditf / __fixunstfdi) and, through the shim's own normalisation, __lshrti3.
//
//   duck build tests/build/longdouble_freestanding_link.cpp --arm64 -k  --start ./start -i . -i ./src -o bin/chk
//   duck build tests/build/longdouble_freestanding_link.cpp --arm64 -ke --start ./start -i . -i ./src -o bin/chk

#include "../../src/math/generic.hpp"
#include "../../src/types.hpp"

volatile double g_in = 3.25;
volatile u64 g_uin = 0x0123456789ABCDEFull;
volatile double g_out = 0.0;
volatile u64 g_uout = 0;

int
main()
{
  long double x = static_cast<long double>(g_in);
  x = x * static_cast<long double>(1.0000000001);
  x = x + static_cast<long double>(2.5);
  x = x / static_cast<long double>(7.0);
  if ( x < static_cast<long double>(0.0) ) x = -x;
  g_out = static_cast<double>(x);

  long double y = static_cast<long double>(g_uin);
  y = y / static_cast<long double>(3.0);
  g_uout = static_cast<u64>(y);

  return 1;
}
