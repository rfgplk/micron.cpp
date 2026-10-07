// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../src/parallel/scan_workspace.hpp"
#include "../src/chrono/clock.hpp"
#include "../src/io/echo.hpp"
#include "../src/memory/allocation/abcmalloc/stats.hpp"

struct affine {
  u32 a{}, b{};
};

struct composition {
  affine
  operator()(const affine &left, const affine &right) const noexcept
  {
    return { right.a * left.a, right.a * left.b + right.b };
  }
};

usize
number(const char *s)
{
  usize n{};
  while ( *s >= '0' && *s <= '9' ) n = n * 10 + usize(*s++ - '0');
  return n;
}

int
main(int argc, char **argv)
{
  const usize n = argc > 1 ? number(argv[1]) : 4096, blocks = argc > 2 ? number(argv[2]) : 4;
  if ( !n || !blocks ) return 2;
  micron::vector<affine> input(n, affine{ 3, 7 }), output(n, affine{});
  micron::parallel::scan_workspace<affine, composition> workspace(blocks);
  micron::parallel::serial_scan_executor executor;
  if ( workspace.scan(input.data(), output.data(), n, composition{}, executor) < 0 ) return 2;
  const auto before = abc::stats();
  const auto begin = micron::chrono::mono_ns();
  usize iterations{};
  u64 elapsed{};
  do {
    if ( workspace.scan(input.data(), output.data(), n, composition{}, executor) < 0 ) return 2;
    __asm__ volatile("" ::: "memory");
    ++iterations;
    elapsed = micron::chrono::mono_ns() - begin;
  } while ( elapsed < 100000000ull );
  const auto after = abc::stats();
  micron::io::print("n=", n, " blocks=", blocks, " executor=serial iterations=", iterations, " ns=", elapsed,
                    " ns/op=", f64(elapsed) / f64(iterations), " allocs=", after.alloc_requests - before.alloc_requests,
                    " frees=", after.dealloc_requests - before.dealloc_requests, " checksum=", output[n - 1].a + output[n - 1].b, "\n");
  return before.enabled && before.alloc_requests == after.alloc_requests && before.dealloc_requests == after.dealloc_requests ? 0 : 2;
}
