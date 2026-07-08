//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../defs.hpp"
#include "../../new.hpp"
#include "../../types.hpp"

#include "../../bits/__ar.hpp"
#include "fiber_stack.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// stackful cactus fiber
//
// fiber is a schedulable execution context that runs on its own contiguous stack segment
// can be suspended at any point (including inside arbitrary blocking code)
//
// the fiber control block (FCB) is embedded at the top of its own stack region (highest address)

namespace micron
{
namespace fiber
{

enum class fiber_state : u32 { fresh = 0, running, suspended, done };

struct alignas(64) fiber {
  ar::__fiber_ctx ctx;                   // this fiber's saved register context (resume point)
  ar::__fiber_ctx *link = nullptr;       // where to swap on suspend/finish (host worker's scheduler ctx)
  __region region;                       // backing stack run (FCB is embedded inside it)
  byte *frame_sp = nullptr;              // coroutine-frame bump cursor (M3)
  fiber *parent = nullptr;               // cactus spine link
  void (*entry)(fiber *) = nullptr;      // user entry, invoked by __micron_fiber_enter
  void *arg = nullptr;                   // user arg
  void *coro = nullptr;                  // std::coroutine_handle<>::address() (M3); void* to avoid <coroutine> here
  micron::atomic_token<u32> state{ static_cast<u32>(fiber_state::fresh) };
  micron::atomic_token<u32> refs{ 1 };
  i32 owner_worker = -1;           // micthread::self() of the allocating worker (cross-worker free routing)
  bool escaped = false;            // an exception escaped entry(); the engine (cl_sched __run) fails fast on it
  fiber *free_next = nullptr;      // per-worker recycle free-list link
};

// host-worker TLS
inline thread_local ar::__fiber_ctx __worker_sched_ctx{};
inline thread_local fiber *__current_fiber = nullptr;

// kernel tid, fetched once per thread now
inline thread_local i32 __cached_tid = -1;

[[gnu::always_inline]] inline i32
__self_tid() noexcept
{
  i32 t = __cached_tid;
  if ( t < 0 ) [[unlikely]]
    t = __cached_tid = static_cast<i32>(micthread::self());
  return t;
}

// per-worker recycle free-list, keyed by size class
inline thread_local fiber *__seg_freelist[__seg_class_count] = {};

inline void
__micron_fiber_enter(void *p) noexcept
{
  fiber *f = static_cast<fiber *>(p);
  f->state.store(static_cast<u32>(fiber_state::running), micron::memory_order_relaxed);
#if !defined(__micron_freestanding) || defined(__micron_eh)
  try {
    f->entry(f);
  } catch ( ... ) {
    f->escaped = true;      // an exception escaped the noexcept coroutine chain; cl_sched __run fails fast on it
  }
#else
  f->entry(f);
#endif
  f->state.store(static_cast<u32>(fiber_state::done), micron::memory_order_release);
  ar::__micron_swap_context(&f->ctx, f->link);      // final park back to the scheduler; never resumed
  __builtin_unreachable();
}

// create a fiber that will run entry(fcb) on a fresh (or recycled) guard-paged stack segment.
[[nodiscard]] inline fiber *
create_fiber(void (*entry)(fiber *), void *arg, usize stack_size = static_cast<usize>(micro_stack_size), fiber *parent = nullptr) noexcept
{
  const u32 cls = __seg_class_for(stack_size);
  fiber *f = nullptr;

  if ( __seg_freelist[cls] != nullptr ) {      // recycle a same-class FCB+region
    f = __seg_freelist[cls];
    __seg_freelist[cls] = f->free_next;
  } else {
    __region reg;
    if ( !__carve_region(reg, cls) ) return nullptr;
    // embed the FCB at the top of the C-stack region, cache-line aligned
    const usize fcb_addr = (reinterpret_cast<usize>(reg.stack_top) - sizeof(fiber)) & ~static_cast<usize>(63);
    f = reinterpret_cast<fiber *>(fcb_addr);
    new (static_cast<void *>(f)) fiber();
    f->region = reg;
  }

  // execution stack top sits just below the embedded FCB (16-aligned)
  byte *stack_top = reinterpret_cast<byte *>(reinterpret_cast<usize>(f) & ~static_cast<usize>(15));

  f->link = &__worker_sched_ctx;
  f->parent = parent;
  f->entry = entry;
  f->arg = arg;
  f->coro = nullptr;
  f->frame_sp = f->region.frame_base;                  // coroutine-frame arena bump cursor (grows up)
  f->refs.store(1, micron::memory_order_relaxed);      // execution holds the initial ref
  f->owner_worker = __self_tid();
  f->escaped = false;
  f->free_next = nullptr;
  // relaxed: the fiber is unpublished until the creating thread resumes or hands it off
  f->state.store(static_cast<u32>(fiber_state::fresh), micron::memory_order_relaxed);

  ar::make_context(&f->ctx, stack_top, &__micron_fiber_enter, f);
  return f;
}

// run f from the current (host) context; returns when f suspends or finishes.
[[gnu::always_inline]] inline void
resume(fiber *f, ar::__fiber_ctx *host = &__worker_sched_ctx) noexcept
{
  f->link = host;
  fiber *prev = __current_fiber;
  __current_fiber = f;
  ar::__micron_swap_context(host, &f->ctx);      // -> trampoline (fresh) or the saved resume point
  __current_fiber = prev;                        // control returned to the host
}

[[gnu::always_inline]] inline void
suspend_to_scheduler() noexcept
{
  fiber *f = __current_fiber;
  f->state.store(static_cast<u32>(fiber_state::suspended), micron::memory_order_release);
  ar::__micron_swap_context(&f->ctx, f->link);
}

// the currently-running fiber on this worker (nullptr if none).
[[gnu::always_inline]] inline fiber *
current() noexcept
{
  return __current_fiber;
}

inline void
__finalize_fiber(fiber *f) noexcept
{
  if ( f->owner_worker == __self_tid() ) {
    f->free_next = __seg_freelist[f->region.cls];
    __seg_freelist[f->region.cls] = f;      // keep the FCB constructed for reuse
    return;
  }
  __region reg = f->region;      // cross-worker final drop: release directly (forgoes recycle)
  f->~fiber();
  __release_region(reg);
}

[[gnu::always_inline]] inline void
__drop_ref(fiber *f) noexcept
{
  // acq_rel so the release pairs with the acquire that observes refs==0 -> all prior frame writes are visible
  if ( f->refs.sub_fetch(1, micron::memory_order_acq_rel) == 0 ) __finalize_fiber(f);
}

inline constexpr usize __frame_hdr = 16;

[[gnu::always_inline]] inline void *
__frame_alloc(usize n) noexcept
{
  fiber *f = __current_fiber;
  const usize need = __frame_hdr + ((n + 15) & ~static_cast<usize>(15));
  if ( f == nullptr ) {      // off-fiber: heap-backed frame, owner == nullptr
    byte *h = static_cast<byte *>(::operator new(need));
    *reinterpret_cast<fiber **>(h) = nullptr;
    return h + __frame_hdr;
  }
  byte *h = reinterpret_cast<byte *>((reinterpret_cast<usize>(f->frame_sp) + 15) & ~static_cast<usize>(15));
  byte *next = h + need;
  if ( next > f->region.frame_limit ) [[unlikely]] {
    byte *hh = static_cast<byte *>(::operator new(need));
    *reinterpret_cast<fiber **>(hh) = nullptr;
    return hh + __frame_hdr;
  }
  f->frame_sp = next;
  *reinterpret_cast<fiber **>(h) = f;
  f->refs.fetch_add(1, micron::memory_order_relaxed);      // a live frame holds a ref
  return h + __frame_hdr;
}

[[gnu::always_inline]] inline void
__frame_free(void *p, usize) noexcept
{
  if ( p == nullptr ) return;
  byte *h = reinterpret_cast<byte *>(p) - __frame_hdr;
  fiber *owner = *reinterpret_cast<fiber **>(h);
  if ( owner == nullptr ) {      // off-fiber / heap frame
    ::operator delete(static_cast<void *>(h));
    return;
  }
  owner->frame_sp = h;      // LIFO pop on the OWNING arena (serialized by fork/join happens-before)
  __drop_ref(owner);
}

inline void
destroy_fiber(fiber *f) noexcept
{
  if ( f == nullptr ) return;
  __drop_ref(f);
}

inline void
drain_freelist() noexcept
{
  for ( u32 c = 0; c < __seg_class_count; ++c ) {
    fiber *f = __seg_freelist[c];
    __seg_freelist[c] = nullptr;
    while ( f != nullptr ) {
      fiber *nx = f->free_next;
      __region reg = f->region;
      f->~fiber();
      __release_region(reg);
      f = nx;
    }
  }
}

};      // namespace fiber
};      // namespace micron
