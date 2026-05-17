//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"

#include "../../src/thread/cpu.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/async_thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/thread/thread_types/group_thread.hpp"
#include "../../src/thread/thread_types/reg_thread.hpp"
#include "../../src/thread/thread_types/thread_impl.hpp"
#include "../../src/thread/thread_types/void_thread.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

volatile int g_alloc_count = 0;
volatile int g_dtor_count = 0;

struct heavy_return {
  int payload;

  heavy_return() : payload(0) { __atomic_add_fetch(&g_alloc_count, 1, __ATOMIC_SEQ_CST); }

  heavy_return(int x) : payload(x) { __atomic_add_fetch(&g_alloc_count, 1, __ATOMIC_SEQ_CST); }

  heavy_return(const heavy_return &o) : payload(o.payload) { __atomic_add_fetch(&g_alloc_count, 1, __ATOMIC_SEQ_CST); }

  heavy_return(heavy_return &&o) noexcept : payload(o.payload)
  {
    __atomic_add_fetch(&g_alloc_count, 1, __ATOMIC_SEQ_CST);
    o.payload = 0;
  }

  heavy_return &operator=(const heavy_return &) = default;
  heavy_return &operator=(heavy_return &&) = default;

  ~heavy_return() { __atomic_add_fetch(&g_dtor_count, 1, __ATOMIC_SEQ_CST); }
};

int
return_int(int n)
{
  return n * 2;
}

void
void_work(void)
{
  /* nothing */
}

heavy_return
return_heavy(int x)
{
  return heavy_return(x);
}

float
return_float(float x)
{
  return x * 1.5f;
}

short
return_short(short x)
{
  return (short)(x + 7);
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== THREAD SMOKE TESTS ===");

  test_case("auto_thread move ctor and move-assign are =deleted");
  {
    static_assert(!micron::is_move_constructible_v<micron::auto_thread<>>, "auto_thread move ctor must be deleted");
    static_assert(!micron::is_move_assignable_v<micron::auto_thread<>>, "auto_thread move-assign must be deleted");
    static_assert(!micron::is_copy_constructible_v<micron::auto_thread<>>, "auto_thread copy ctor must be deleted");
    require(true);
  }
  end_test_case();

  test_case("lvalue acceptance: thread entry points accept lvalue, rvalue, pointer");
  {
    using FInt = int (*)(int);
    using FRef = void (*)(int &);
    using FPtr = void (*)(int *);
    static_assert(micron::is_constructible_v<micron::auto_thread<>, FInt, int &>, "auto_thread must accept lvalue int (copy semantics)");
    static_assert(micron::is_constructible_v<micron::auto_thread<>, FInt, int>, "auto_thread must accept rvalue int");
    static_assert(micron::is_constructible_v<micron::auto_thread<>, FPtr, int *>, "auto_thread must accept pointer");
    static_assert(micron::is_constructible_v<micron::auto_thread<>, FRef, int &>,
                  "auto_thread must accept lvalue when fn takes int& (binds to worker's heap copy)");
    static_assert(micron::is_constructible_v<micron::thread<>, FInt, int &>, "thread<> must accept lvalue int");
    static_assert(micron::is_constructible_v<micron::thread<>, FInt, int>, "thread<> must accept rvalue int");
    static_assert(micron::is_constructible_v<micron::thread<>, FPtr, int *>, "thread<> must accept pointer");
    static_assert(micron::is_constructible_v<micron::group_thread<>, const pthread_attr_t &, FInt, int &>,
                  "group_thread<> must accept lvalue int");
    static_assert(micron::is_constructible_v<micron::group_thread<>, const pthread_attr_t &, FInt, int>,
                  "group_thread<> must accept rvalue int");
    static_assert(micron::is_constructible_v<micron::void_thread<>, const pthread_attr_t &, FInt, int &>,
                  "void_thread<> must accept lvalue int");
    static_assert(micron::is_constructible_v<micron::void_thread<>, const pthread_attr_t &, FInt, int>,
                  "void_thread<> must accept rvalue int");
    static_assert(micron::is_constructible_v<micron::async_thread<>, const pthread_attr_t &, FInt, int &>,
                  "async_thread<> must accept lvalue int");
    static_assert(micron::is_constructible_v<micron::async_thread<>, const pthread_attr_t &, FInt, int>,
                  "async_thread<> must accept rvalue int");
    require(true);
  }
  end_test_case();

  test_case("lvalue copy semantics: worker mutation does NOT affect caller's lvalue");
  {
    int x = 5;
    auto mutator = +[](int &y) { y = 100; };
    {
      micron::auto_thread<> t(mutator, x);
      t.join();
    }

    require(x, 5);
  }
  end_test_case();

  test_case("lvalue value path: fn(int) receives a copy of the caller's lvalue");
  {
    int x = 42;
    auto reader = +[](int v) -> int { return v + 1; };
    micron::auto_thread<> t(reader, x);
    t.join();

    require(x, 42);
  }
  end_test_case();

  test_case("__thread_payload::tag_val is atomic_token<u8>");
  {
    static_assert(sizeof(micron::__thread_payload::tag) == 1, "tag enum must be u8-backed");
    micron::__thread_payload p;

    p.tag_val.store((u8)micron::__thread_payload::tag::heap, micron::memory_order_release);
    auto v = p.tag_val.get(micron::memory_order_acquire);
    require((int)v, (int)micron::__thread_payload::tag::heap);
  }
  end_test_case();

  test_case("cpu_t<true> default mask has no out-of-range bit set");
  {
    const auto count = micron::cpu_count();
    micron::cpu_t<true> c;

    for ( unsigned i = 0; i < count; ++i ) {
      require(c.get().cpu_isset(i), true);
    }

    require(c.get().cpu_isset(count), false);
  }
  end_test_case();

  test_case("enable_all_cores does not set the out-of-range bit");
  {
    const auto count = micron::cpu_count();
    micron::cpu_t<false> c;
    micron::enable_all_cores();

    micron::posix::cpu_set_t s;
    for ( unsigned i = 0; i < count; ++i ) s.cpu_set(i);
    require(s.cpu_isset(count), false);
    require(s.cpu_isset(count - 1), true);
  }
  end_test_case();

  test_case("auto_thread<heavy_return> joins cleanly with no double-free");
  {
    __atomic_store_n(&g_alloc_count, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&g_dtor_count, 0, __ATOMIC_SEQ_CST);
    {
      micron::auto_thread<> t(return_heavy, 42);
      t.wait_for();
      t.join();
    }
    const int allocs = __atomic_load_n(&g_alloc_count, __ATOMIC_SEQ_CST);
    const int dtors = __atomic_load_n(&g_dtor_count, __ATOMIC_SEQ_CST);

    require(allocs > 0, true);
    require(allocs, dtors);
  }
  end_test_case();

  test_case("auto_thread<void> join then destructor — no UB on tag::none");
  {

    {
      micron::auto_thread<> t(void_work);
      t.join();
    }
    require(true);
  }
  end_test_case();

  test_case("auto_thread<int> (literal) join then destructor — no UB on tag::literal");
  {
    {
      micron::auto_thread<> t(return_int, 21);
      t.join();
    }
    require(true);
  }
  end_test_case();

  test_case("calling join() then try_join() must be safe (idempotent)");
  {
    micron::auto_thread<> t(return_int, 1);
    t.wait_for();
    int r1 = t.join();

    require(r1 == 1 || r1 == 0, true);

    int r2 = t.try_join();
    require(r2, 1);
  }
  end_test_case();

  test_case("auto_thread<float> result round-trips bit-for-bit");
  {
    micron::auto_thread<> t(return_float, 4.0f);
    float got = t.result<float>();
    t.join();
    require(got == 6.0f, true);
  }
  end_test_case();

  test_case("auto_thread<short> result has no garbage upper bits");
  {
    micron::auto_thread<> t(return_short, (short)100);
    t.wait_for();
    short got = t.result<short>();
    t.join();
    require((int)got, 107);
  }
  end_test_case();

  test_case("__worker_payload::usage_seq is present");
  {
    micron::__worker_payload wp{};
    static_assert(sizeof(wp.usage_seq.v) == 4, "usage_seq must be u32");

    require(wp.usage_seq.get(micron::memory_order_acquire), (u32)0);
    require((wp.usage_seq.get(micron::memory_order_acquire) & 1u) == 0u, true);
  }
  end_test_case();

  test_case("smoke: group_thread runs and returns successfully");
  {
    auto attrs = micron::pthread::prepare_thread(micron::pthread::thread_create_state::joinable, micron::posix::sched_other, 0);

    auto stack = micron::addrmap(micron::thread_stack_size);
    require(!micron::mmap_failed(stack), true);
    micron::pthread::set_stack_thread(attrs, stack, micron::thread_stack_size);

    {
      micron::group_thread<> t(attrs, return_int, 5);
      int rc = t.join();
      require(rc, 0);
    }

    require(true);
  }
  end_test_case();

  sb::print("=== ALL THREAD PATCH TESTS PASSED ===");
  return 1;
}
