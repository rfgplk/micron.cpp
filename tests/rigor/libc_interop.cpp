//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#if !defined(MICRON_INTEROP_STD_FIRST)
#include "../../src/maps/hopscotch.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/std.hpp"
#include "../../src/string/string.hpp"

#include "../snowball/snowball.hpp"
#endif

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <map>
#include <new>
#include <set>
#include <string>
#include <sys/types.h>
#include <vector>

#if defined(MICRON_INTEROP_STD_FIRST)
#include "../../src/maps/hopscotch.hpp"
#include "../../src/sort/sort.hpp"
#include "../../src/std.hpp"
#include "../../src/string/string.hpp"

#include "../snowball/snowball.hpp"
#endif

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

static void
test_names_coexist(void)
{
  test_case("a.1: micron's and libc's time types are both visible and independent");
  {
    // ::time_t is glibc's -- micron must not have claimed the name
    static_assert(micron::is_same_v<::time_t, ::time_t>, "::time_t must name a type");
    micron::time_t mt = 1;
    ::time_t gt = 1;
    require_true(mt == 1);
    require_true(gt == 1);
  }
  end_test_case();

  test_case("a.2: the names micron used to publish globally are gone from global scope");
  {

    micron::clockid_t c = 0;
    micron::suseconds_t s = 0;
    micron::clock_t k = 0;
    require_true(c == 0 && s == 0 && k == 0);
  }
  end_test_case();
}

static void
test_widths(void)
{
  test_case("b.1: micron is time64 on every arch");
  {
    static_assert(sizeof(micron::time_t) == 8, "micron::time_t must be 64-bit on every target");
    static_assert(sizeof(micron::suseconds_t) == 8, "micron::suseconds_t must be 64-bit");
    require(sizeof(micron::time_t), static_cast<usize>(8));
  }
  end_test_case();

  test_case("b.2: glibc's time_t is untouched -- we yielded the name, we did not poison its guard");
  {
    // glibc's own width, whatever the target says it is. On ILP32 this is 4 and micron's is 8;
    // that difference is the whole point and must survive.
    static_assert(sizeof(::time_t) == sizeof(long), "::time_t must still be glibc's native type");
    require(sizeof(::time_t), sizeof(long));
#if __micron_arch_width_32
    // the case that used to be a hard compile error
    require_true(sizeof(::time_t) != sizeof(micron::time_t));
#else
    require_true(sizeof(::time_t) == sizeof(micron::time_t));
#endif
  }
  end_test_case();

  test_case("b.3: micron's timespec_t is time64 and is NOT glibc's struct timespec");
  {
    micron::timespec_t ts{};
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(1);
    static_assert(sizeof(ts.tv_sec) == 8, "micron timespec_t.tv_sec must be 64-bit");
    require_true(ts.tv_sec == 1);
  }
  end_test_case();
}

static void
test_stl_oracle(void)
{
  test_case("c.1: std::map oracle vs micron::hopscotch_map, std::string keys");
  {
    std::map<std::string, int> oracle;
    micron::hopscotch_map<micron::string, int> m;

    for ( int i = 0; i < 512; ++i ) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "key_%04d", i * 7 % 512);
      oracle[std::string(buf)] = i;
      m[micron::string(buf)] = i;
    }
    require(m.size(), static_cast<usize>(oracle.size()));

    for ( const auto &kv : oracle ) {
      auto *v = m.find(micron::string(kv.first.c_str()));
      require_true(v != nullptr);
      if ( v ) require(*v, kv.second);
    }
  }
  end_test_case();

  test_case("c.2: std::vector + std::sort agree with micron::vector + micron::sort::sort");
  {
    std::vector<u64> a;
    micron::vector<u64> b;
    u64 x = 0x9E3779B97F4A7C15ULL;
    for ( int i = 0; i < 1024; ++i ) {
      x ^= x << 13;
      x ^= x >> 7;
      x ^= x << 17;
      a.push_back(x);
      b.push_back(x);
    }
    std::sort(a.begin(), a.end());
    micron::sort::sort(b);
    require(b.size(), static_cast<usize>(a.size()));
    bool same = true;
    for ( usize i = 0; i < b.size(); ++i )
      if ( a[i] != b[i] ) same = false;
    require_true(same);
  }
  end_test_case();

  test_case("c.3: <chrono> and micron::chrono coexist in one TU");
  {
    const auto t0 = std::chrono::steady_clock::now();
    micron::timespec_t ts{};
    const auto rc = micron::clock_gettime(micron::clock_monotonic, ts);
    const auto t1 = std::chrono::steady_clock::now();
    require_true(rc >= 0);
    require_true(ts.tv_sec > 0);
    require_true(t1 >= t0);
  }
  end_test_case();
}

static void
test_size_types(void)
{
  test_case("d.1: micron's size types ARE libc's, not same-width lookalikes");
  {

    static_assert(micron::is_same_v<usize, std::size_t>, "micron::usize must BE std::size_t");
    static_assert(micron::is_same_v<::size_t, std::size_t>, "::size_t must BE std::size_t");
    static_assert(micron::is_same_v<uintptr_t, std::uintptr_t>, "uintptr_t must BE std::uintptr_t");
    static_assert(micron::is_same_v<ptrdiff_t, std::ptrdiff_t>, "ptrdiff_t must BE std::ptrdiff_t");
    static_assert(micron::is_same_v<usize, decltype(sizeof(0))>, "usize must BE the type of sizeof");
    require(sizeof(usize), sizeof(void *));
  }
  end_test_case();

  test_case("d.2: glibc's size types are untouched -- we agreed with them, we did not move them");
  {

    static_assert(micron::is_same_v<std::size_t, decltype(sizeof(0))>, "std::size_t must still be the type of sizeof");
    static_assert(sizeof(std::size_t) == sizeof(void *), "std::size_t must still be pointer-width");
    require(sizeof(std::size_t), sizeof(usize));
  }
  end_test_case();

  test_case("d.3: std::align_val_t is ONE type and micron's aligned operator new is the live one");
  {
    static_assert(micron::is_same_v<micron::underlying_type_t<std::align_val_t>, usize>,
                  "std::align_val_t's underlying type must be size_t");
    constexpr usize al = 128;
    void *p = ::operator new(64, static_cast<std::align_val_t>(al));
    require_true(p != nullptr);
    require_true((reinterpret_cast<uintptr_t>(p) & (al - 1)) == 0);
    ::operator delete(p, 64, static_cast<std::align_val_t>(al));

    struct alignas(128) cell {
      double v[16];
    };

    std::vector<cell> v(4);
    require_true((reinterpret_cast<uintptr_t>(v.data()) & (al - 1)) == 0);
  }
  end_test_case();
}

struct __interop_task {
  struct promise_type {
    int value = 0;

    __interop_task
    get_return_object(void) noexcept
    {
      return __interop_task{ std::coroutine_handle<promise_type>::from_promise(*this) };
    }

    std::suspend_always
    initial_suspend(void) const noexcept
    {
      return {};
    }

    std::suspend_always
    final_suspend(void) const noexcept
    {
      return {};
    }

    std::suspend_always
    yield_value(int v) noexcept
    {
      value = v;
      return {};
    }

    void
    return_void(void) const noexcept
    {
    }

    void
    unhandled_exception(void) const noexcept
    {
    }
  };

  std::coroutine_handle<promise_type> h{};

  ~__interop_task(void)
  {
    if ( h ) h.destroy();
  }
};

static __interop_task
__interop_gen(void)
{
  co_yield 41;
  co_yield 42;
}

static void
test_names_and_coroutine(void)
{
  test_case("e.1: pid_t/uid_t/gid_t are glibc's at global scope and micron's under micron::");
  {
    ::pid_t libc_pid = 0;
    micron::pid_t mc_pid = 0;
    micron::uid_t mc_uid = 0;
    micron::gid_t mc_gid = 0;
    (void)libc_pid;
    require(sizeof(micron::pid_t), static_cast<usize>(4));
    require(sizeof(micron::uid_t), static_cast<usize>(4));
    require_true(mc_pid == 0 && mc_uid == 0 && mc_gid == 0);

    require(sizeof(::pid_t), sizeof(micron::pid_t));
  }
  end_test_case();

  test_case("e.2: <coroutine> and a micron coroutine coexist, on ONE std::coroutine_handle");
  {

    static_assert(sizeof(std::coroutine_handle<>) == sizeof(void *), "coroutine_handle is one void *");
    std::coroutine_handle<> null_h = std::coroutine_handle<>::from_address(nullptr);
    require_true(!null_h);

    auto g = __interop_gen();
    int seen = 0;
    int last = 0;
    while ( !g.h.done() ) {
      g.h.resume();
      if ( g.h.done() ) break;
      last = g.h.promise().value;
      ++seen;
    }
    require(seen, 2);
    require(last, 42);

    std::coroutine_handle<> erased = g.h;
    require_true(erased.address() == g.h.address());
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron <-> libstdc++ interop suite");
#if defined(MICRON_INTEROP_STD_FIRST)
  sb::print("include order: libstdc++ FIRST, micron second");
#else
  sb::print("include order: micron FIRST, libstdc++ second");
#endif
  sb::print("==================================");
  test_names_coexist();
  test_widths();
  test_size_types();
  test_names_and_coroutine();
  test_stl_oracle();
  sb::print("==================================");
  sb::print("ALL LIBC INTEROP TESTS COMPLETED");
  return 1;
}
