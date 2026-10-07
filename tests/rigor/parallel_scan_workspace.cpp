// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/parallel/scan_workspace.hpp"

#include "../../src/math/rng/engines.hpp"
#include "../../src/memory/allocation/abcmalloc/stats.hpp"
#include "../snowball/snowball.hpp"

extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1";
}

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

struct reversed_executor {
  usize calls{};
  max_t failure{};

  template<auto Op, class Work>
  max_t
  run(Work *items, usize count) noexcept
  {
    ++calls;
    if ( failure ) return failure;
    while ( count ) {
      const auto result = Op(items[--count]);
      if ( result < 0 ) return result;
    }
    return 0;
  }
};

int
main()
{
  auto rng = micron::math::rng::xoshiro256ss::from_seed(0x5343414E504C414Eull);
  micron::vector<affine> input(514, affine{}), output(514, affine{}), inplace(514, affine{});
  for ( usize i = 0; i < 514; ++i ) input[i] = { u32(rng.next()), u32(rng.next()) };
  sb::test_case("prepared scan preserves noncommutative order across ragged blocks and arbitrary job completion order");
  for ( usize blocks : { usize(1), usize(2), usize(3), usize(7), usize(16), usize(32) } ) {
    micron::parallel::scan_workspace<affine, composition> workspace(blocks);
    reversed_executor executor;
    bool good = true;
    const auto before = abc::stats();
    for ( usize n = 0; n <= 513; ++n ) {
      for ( usize i = 0; i < n; ++i ) inplace[i] = input[i];
      good &= micron::parallel::scan_into(input.data(), output.data(), n, composition{}, workspace, executor) == 0;
      good &= micron::parallel::scan_into(inplace.data(), inplace.data(), n, composition{}, workspace, executor) == 0;
      u32 a = 1, b = 0, point = 37;
      for ( usize i = 0; i < n; ++i ) {
        a = input[i].a * a;
        b = input[i].a * b + input[i].b;
        point = input[i].a * point + input[i].b;
        good &= output[i].a == a && output[i].b == b && output[i].a * 37 + output[i].b == point;
        good &= inplace[i].a == a && inplace[i].b == b;
      }
    }
    const auto after = abc::stats();
#if MICRON_ABC_STATS
    good &= before.enabled && before.alloc_requests == after.alloc_requests && before.dealloc_requests == after.dealloc_requests;
#else
    (void)before;
    (void)after;
#endif
    sb::require_true(good);
  }
  sb::end_test_case();
  sb::test_case("invalid spans and executor faults return before dependent scan phases");
  micron::parallel::scan_workspace<affine, composition> workspace(4), invalid(0);
  reversed_executor executor;
  sb::require_true(micron::parallel::scan_into(input.data(), input.data() + 1, 3, composition{}, workspace, executor) == -22);
  sb::require_true(workspace.scan(nullptr, nullptr, 0, composition{}, executor) == 0);
  sb::require_true(workspace.scan(nullptr, output.data(), 1, composition{}, executor) == -22);
  sb::require_true(invalid.scan(input.data(), output.data(), 1, composition{}, executor) == -22);
  sb::require_true(executor.calls == 0);
  executor.failure = -125;
  sb::require_true(workspace.scan(input.data(), output.data(), 5, composition{}, executor) == -125 && executor.calls == 1);
  sb::end_test_case();
  return 1;
}
