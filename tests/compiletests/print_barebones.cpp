//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE PRINTER, INSTANTIATED, ON A TARGET WITH NO FPU AND NO VECTOR UNIT.
//
// Built, never run. src/print.hpp replaced io/echo.hpp at Phase 4; io/echo.hpp could never compile
// under kernel flags because it went through the buffered stream layer and a file descriptor.
//
// INTEGER-ONLY on purpose, for the reason barebones_core.cpp is: under -mno-sse -mno-80387 a
// function that merely RETURNS a double is a hard error before its body is considered. The float
// paths of the formatter are exercised by the hosted cells, not here.

#include "../../src/print.hpp"

#include "../../src/array.hpp"
#include "../../src/strings.hpp"
#include "../../src/vector.hpp"

namespace
{

[[gnu::used]] void
scalars()
{
  micron::println("literal");
  micron::println("mixed ", 42, ' ', -7);
  micron::print("no newline");
  micron::printn("printn");
  micron::println(static_cast<u64>(18446744073709551615ull));
  micron::println(static_cast<i64>(-9223372036854775807ll));
  micron::flush();
}

[[gnu::used]] void
containers()
{
  micron::vector<u32> v;
  v.push_back(1u);
  micron::println(v);

  micron::array<u32, 4> a{};
  micron::println(a);

  micron::string s = "barebones";
  micron::println(s);
}

// the io:: spelling is a deliberate compatibility surface (see print.hpp's banner): it is what makes
// the 267 print-only test/bench/example files an include swap rather than a rename. Pinned so it
// cannot rot away silently.
[[gnu::used]] void
compat_namespace()
{
  micron::io::println("io spelling ", 1);
  micron::io::print("io print");
  micron::io::printn("io printn");
}

// printk is generic over the sink concept -- pin that a caller can supply its own.
struct counting_sink {
  usize n = 0;
  max_t put(const char *, usize len) { n += len; return static_cast<max_t>(len); }
  max_t put(char) { n += 1; return 1; }
  max_t flush(void) { return 0; }
};

[[gnu::used]] usize
custom_sink()
{
  counting_sink s;
  micron::printk(s, "counted");
  micron::printkn(s, 1234);
  return s.n;
}

}      // namespace

int
main()
{
  return 1;
}
