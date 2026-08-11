//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: the 128-bit print and format entry points, on EVERY arch.
//
// this is an arch gate, not a behaviour gate, and it can only be a compiletest -- the defect it
// pins is invisible on amd64 by construction.
//
// io::printk is what io::echo/echof funnel arithmetic through, and it was constrained
// `requires micron::is_arithmetic_v<T>`. type_traits.hpp registers the __int128 traits under
// __micron_arch_amd64 ONLY, while bits/__int128.hpp gives the TYPE on every arch of 64-bit width
// -- so on aarch64 (and i386/armv7-a, where u128 is micron's own struct) a u128 satisfies no
// integral or arithmetic concept at all, the constraint rejected the call, and the u128/i128 arms
// added to printk's ladder were unreachable. `io::echo("x", v)` with a u128 was a hard compile
// error off amd64 and printed correctly on it.
//
// conversions/chars.hpp:146 documents the same trap and keys its own 128-bit entry points on the
// TYPE; printk now does too, via io::__printk_arith. int_to_string got the same treatment for the
// same reason -- tests/compiletests/strings.cpp used int_to_string<u128> as though it were
// portable, which broke every --arm/--arm64/--i386 cell of verify_compile.duck.
//
// Nothing here is run. Failing to compile IS the failing test, on whichever cell fails.

#include "../../src/io/echo.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"

namespace mc = micron;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the concept itself must accept both, whatever is_arithmetic_v says on this arch

static_assert(mc::io::__printk_arith<u128>);
static_assert(mc::io::__printk_arith<i128>);
static_assert(mc::io::__printk_arith<u64>);
static_assert(mc::io::__printk_arith<f64>);
static_assert(mc::io::__printk_arith<bool>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// every 128-bit entry point, instantiated

[[maybe_unused]] static void
__echo_128(void)
{
  const u128 u = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
  const i128 s = -(static_cast<i128>(1) << 100);

  mc::io::echo("u128:", u, " i128:", s);
  mc::io::echon(u, s);
  mc::io::echof("{} {}\n", u, s);
  mc::io::echofn("{:x} {:#b}", u, s);

  // the sink path, which carries its own scratch
  mc::io::stdout_sink snk;
  (void)mc::io::printk(snk, u);
  (void)mc::io::printk(snk, s);

  // the format layer
  (void)mc::format::format("{}", u);
  (void)mc::format::format("{:x}", s);
  (void)mc::format::format("{:#b}", u);

  // the scratch converters the ladder dispatches to
  char buf[160];
  (void)mc::io::__impl::arith_to_buf(buf, sizeof(buf), u);
  (void)mc::io::__impl::arith_to_buf(buf, sizeof(buf), s);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string / to_string, both spellings, with the VALUE type first

[[maybe_unused]] static void
__int128_to_string(void)
{
  const u128 u = (static_cast<u128>(1) << 100) + static_cast<u128>(7);
  const i128 s = -(static_cast<i128>(1) << 64);

  (void)mc::int_to_string<u128>(u).size();
  (void)mc::int_to_string<i128>(s).size();
  (void)mc::to_string<u128>(u).size();

  char buf[160];
  (void)mc::to_chars(buf, sizeof(buf), u, 10u);
  (void)mc::to_chars(buf, sizeof(buf), s, 16u, true);
  u128 ub{};
  i128 ib{};
  (void)mc::from_chars(ub, "1", 1);
  (void)mc::from_chars(ib, "-1", 2);
}

int
main()
{
  return 1;
}
