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
//
// NOTE (frame_sp): the unified frames-on-fiber model (M3) bump-allocates coroutine frames within this same
// segment via the frame_sp cursor; the precise stack/frame split + overflow geometry is finalized in M3. for
// now frame_sp is initialized to the segment base and otherwise unused.

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
  // arena reference count = (live coroutine frames on this fiber's arena) + (1 while execution is not retired).
  // a stolen continuation's frame lives on the fiber that CREATED it even after that fiber's execution ends, so
  // the segment must outlive execution; the last ref-drop finalizes (recycles on the owner worker, else releases).
  micron::atomic_token<u32> refs{ 1 };
  i32 owner_worker = -1;           // micthread::self() of the allocating worker (cross-worker free routing)
  bool escaped = false;            // an exception escaped entry(); the engine (cl_sched __run) fails fast on it
  fiber *free_next = nullptr;      // per-worker recycle free-list link
};

// host-worker TLS
inline thread_local ar::__fiber_ctx __worker_sched_ctx{};
inline thread_local fiber *__current_fiber = nullptr;

// per-worker recycle free-list, keyed by size class
inline thread_local fiber *__seg_freelist[__seg_class_count] = {};

// the C++ body the trampoline lands in (fn(arg) with arg == fiber*). runs the user entry then parks forever.
inline void
__micron_fiber_enter(void *p) noexcept
{
  fiber *f = static_cast<fiber *>(p);
  f->state.store(static_cast<u32>(fiber_state::running), micron::memory_order_release);
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
  f->owner_worker = static_cast<i32>(micthread::self());
  f->escaped = false;
  f->free_next = nullptr;
  f->state.store(static_cast<u32>(fiber_state::fresh), micron::memory_order_release);

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

// called FROM INSIDE a running fiber: park and hand control back to whoever resumed us.
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

// finalize a fiber whose arena refcount hit zero: recycle on the owner worker, else release the region.
inline void
__finalize_fiber(fiber *f) noexcept
{
  if ( f->owner_worker == static_cast<i32>(micthread::self()) ) {
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// coroutine-frame bump allocator (the unified cactus): promise_type::operator new draws a frame from the
// running fiber's arena (grows up, LIFO); sized operator delete pops it.
//
// each frame carries a one-word owner header (the fiber whose arena it lives on) so a CROSS-FIBER free — a
// stolen continuation completing on a thief's fiber while its frame physically lives on the victim's arena —
// routes the LIFO pop + ref-drop to the correct arena. per-arena frees are serialized by the fork/join
// happens-before chain (a child frees before its parent's continuation resumes), so no lock is needed.
//
// off-fiber creation (e.g. the sync_wait root before a driver fiber exists) falls back to the global allocator;
// such frames carry owner == nullptr and free via ::operator delete.

inline constexpr usize __frame_hdr = 16;      // owner pointer (8) padded to 16 to keep the frame 16-aligned

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
  if ( next > f->region.frame_limit ) [[unlikely]]
    return nullptr;      // arena exhausted -> get_return_object_on_allocation_failure
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

// retire a fiber's EXECUTION (it returned to the scheduler). drops the execution ref; the segment is reclaimed
// only once all frames on its arena are also freed (refs -> 0), so a stolen continuation's frame stays valid.
inline void
destroy_fiber(fiber *f) noexcept
{
  if ( f == nullptr ) return;
  __drop_ref(f);
}

// drop the per-worker recycle cache, releasing all cached regions (call at worker teardown).
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
