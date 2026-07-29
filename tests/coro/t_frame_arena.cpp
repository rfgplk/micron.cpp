//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fiber coroutine-frame arena: the bump cursor is a stack, and only the top may pop.
//
// __frame_free used to do `owner->frame_sp = h` unconditionally. that is a rewind past every frame
// allocated after the one being freed, so the next __frame_alloc hands out bytes that are still
// live. frames DO retire out of order (~task, ~__call_awaitable, the fork_group bridge), and the
// same store also raced across threads: a stolen continuation reaching join destroys children that
// were bump-allocated out of a different worker's fiber.
//
// driven directly rather than through coroutines so the ordering is exact instead of incidental.

#include "../../src/tasks/coroutine/fiber.hpp"
#include "../snowball/snowball.hpp"

namespace fb = micron::fiber;

static int FAILS = 0;

static bool
overlaps(const void *p, usize pn, const void *q, usize qn)
{
  const byte *a = static_cast<const byte *>(p), *b = static_cast<const byte *>(q);
  return a < b + qn && b < a + pn;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 1. out-of-order free must not rewind over a live frame

static void
entry_ooo(fb::fiber *)
{
  constexpr usize SMALL = 64, BIG = 200;
  void *a = fb::__frame_alloc(SMALL);
  void *b = fb::__frame_alloc(SMALL);
  sb::check(a != nullptr && b != nullptr);
  sb::check(!overlaps(a, SMALL, b, SMALL));

  fb::__frame_free(a, SMALL);      // NOT the top of the arena; b is still live

  // big enough that a rewind to a's header would span b's slot as well
  void *c = fb::__frame_alloc(BIG);
  sb::check(c != nullptr);
  sb::check(!overlaps(c, BIG, b, SMALL));      // <- the corruption the guard prevents

  fb::__frame_free(c, BIG);
  fb::__frame_free(b, SMALL);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 2. strict LIFO must still fully reclaim (the guard must not disable the arena)

static void
entry_lifo(fb::fiber *)
{
  constexpr usize N = 96;
  void *a = fb::__frame_alloc(N);
  void *b = fb::__frame_alloc(N);
  fb::__frame_free(b, N);
  void *b2 = fb::__frame_alloc(N);
  sb::check(b2 == b);      // top popped and reused
  fb::__frame_free(b2, N);
  void *a2_probe = fb::__frame_alloc(N);
  sb::check(a2_probe == b);      // still just below the old top
  fb::__frame_free(a2_probe, N);
  fb::__frame_free(a, N);

  void *a3 = fb::__frame_alloc(N);
  sb::check(a3 == a);      // whole arena unwound
  fb::__frame_free(a3, N);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 3. a free arriving from another thread must not touch the owner's cursor

// the fiber allocates and returns to its host with both frames live; the host then frees one.
// the host is not running that fiber, so __current_fiber != owner -- the same condition a worker
// hits when a stolen continuation reaches join and destroys another worker's children.
static void *g_low = nullptr;
static void *g_high = nullptr;

static void
entry_cross(fb::fiber *)
{
  g_low = fb::__frame_alloc(128);
  g_high = fb::__frame_alloc(128);
  sb::check(g_low != nullptr && g_high != nullptr);
}

static void
cross_thread_free_case()
{
  fb::fiber *f = fb::create_fiber(entry_cross, nullptr, static_cast<usize>(micron::small_stack_size));
  sb::require(f != nullptr);
  fb::resume(f);      // returns with both frames still live

  byte *const sp_before = f->frame_sp;
  sb::check(fb::__current_fiber != f);      // we are the host, not the fiber

  fb::__frame_free(g_low, 128);             // foreign free of a non-top block
  sb::check(f->frame_sp == sp_before);      // cursor untouched

  fb::__frame_free(g_high, 128);
  fb::destroy_fiber(f);
}

static void
run(void (*e)(fb::fiber *))
{
  fb::fiber *f = fb::create_fiber(e, nullptr, static_cast<usize>(micron::small_stack_size));
  sb::require(f != nullptr);
  fb::resume(f);
  fb::destroy_fiber(f);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  sb::test_case("frame arena: out-of-order free does not rewind over a live frame");
  run(entry_ooo);
  sb::end_test_case();

  sb::test_case("frame arena: strict LIFO still reclaims");
  run(entry_lifo);
  sb::end_test_case();

  sb::test_case("frame arena: foreign-thread free leaves the owner cursor alone");
  cross_thread_free_case();
  sb::end_test_case();

  if ( FAILS != 0 ) return 0;
  sb::print("=== ALL FRAME ARENA TESTS PASSED ===");
  return 1;
}
