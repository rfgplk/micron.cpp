//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__abc_mt.hpp"

#if defined(MICRON_CORO_URING)

#include "aio.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// coro waves
//
// (many batched sqes, one enter)
//
// WARNING: the awaitable owns the __io_wop array; it may NEVER be destroyed while __wv.__left != 0
//
// NOTE: a wave doesnt register with the __io_cxl cancellation registry; cancelling a task that is
// awaiting a wave therefore does not cut the batch short; safe

namespace micron
{
namespace coro
{
namespace io
{

[[nodiscard]] inline bool
wave_available() noexcept
{
  __wring *__r = __io_own_ring();
  return __r != nullptr && __io_files_reg(*__r);
}

[[nodiscard]] inline u32
wave_room() noexcept
{
  __wring *__r = __io_own_ring();
  return __r == nullptr ? 0u : __io_sq_room(*__r);
}

[[nodiscard]] inline i32
wave_ring_id() noexcept
{
  worker *__w = current_worker();
  return __w == nullptr ? -1 : static_cast<i32>(__w->id);
}

[[nodiscard]] inline __wring *
wave_ring_of(i32 __rid) noexcept
{
  if ( __rid < 0 || static_cast<u32>(__rid) >= __io_ring_cap ) return nullptr;
  __wring &__wr = __io_rings[__rid];
  return __wr.__live.get(micron::memory_order_acquire) != 0 ? &__wr : nullptr;
}

[[nodiscard]] inline i32
wave_slot_acquire() noexcept
{
  __wring *__r = __io_own_ring();
  return __r == nullptr ? -1 : __io_slot_acquire(*__r);
}

inline void
wave_slot_release(i32 __rid, i32 __slot) noexcept
{
  __wring *__r = wave_ring_of(__rid);
  if ( __r != nullptr ) __io_slot_release(*__r, __slot);
}

// evict a direct descriptor whose close never ran (severed chain, cancelled batch)
[[nodiscard]] inline bool
wave_slot_close(i32 __rid, i32 __slot) noexcept
{
  // WARNING: the slot may only go back in the free bitmap once this succeeds
  __wring *__r = wave_ring_of(__rid);
  if ( __r == nullptr ) return false;
  if ( __slot < 0 || static_cast<u32>(__slot) >= __io_file_slots ) return false;
  const i32 __none = -1;
  return __r->__r.register_files_update(static_cast<u32>(__slot), &__none, 1) >= 0;
}

struct [[nodiscard]] __wave_awaitable {
  micron::uring::sqe *__q = nullptr;
  __io_wop *__nodes = nullptr;
  __io_wave *__wv = nullptr;
  u32 __k = 0;
  i32 __err = 0;

  bool
  await_ready() noexcept
  {
    if ( __k == 0 ) {
      __err = 0;
      return true;
    }
    if ( __io.any_live.get(micron::memory_order_acquire) == 0 ) {
      __err = -38;      // no ring on this system
      return true;
    }
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();
    __wv->__f = __f;
    __wv->__fin.store(0, micron::memory_order_relaxed);
    __wv->__st.store(__io_st_submitted, micron::memory_order_relaxed);
    __wv->__left.store(__k, micron::memory_order_release);

    __wring *__r = __io_own_ring();
    if ( __r == nullptr ) [[unlikely]] {
      __err = -38;
      return false;
    }
    for ( u32 __i = 0; __i < __k; ++__i ) {
      if ( reinterpret_cast<u64>(&__nodes[__i]) >> __io_ud_tag_shift != 0 ) [[unlikely]]
        __builtin_trap();
      __nodes[__i].__w = __wv;
      __nodes[__i].__res = 0;
      __q[__i].user_data = __io_ud_make(__io_ud_wop, reinterpret_cast<u64>(&__nodes[__i]));
    }

    __r->__pending.fetch_add(1, micron::memory_order_acq_rel);
    if ( !__io_submit_own_n(*__r, __q, __k) ) [[unlikely]] {
      __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
      __wv->__left.store(0, micron::memory_order_release);
      __err = -105;
      return false;
    }

    if ( __io_cq_acquire(*__r) ) {
      micron::uring::cqe __c{};
      u32 __budget = __k + 8u;
      while ( __budget-- != 0 && __wv->__left.get(micron::memory_order_acquire) != 0 && __r->__r.peek_cqe(&__c) )
        __global_engine->__dispatch_cqe(*__r, __c);
      __io_unlock(__r->__cq_lk);
    }

    u32 __exp = __io_st_submitted;
    if ( __wv->__st.compare_exchange_strong(__exp, __io_st_suspended, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      return true;
    __r->__pending.sub_fetch(1, micron::memory_order_acq_rel);
    return false;
  }

  i32
  await_resume() const noexcept
  {
    return __err;
  }
};

inline bool
__wave_abandon(__io_wave &__wv) noexcept
{
  u32 __e = __io_st_suspended;
  return __wv.__st.compare_exchange_strong(__e, __io_st_abandoned, micron::memory_order_acq_rel, micron::memory_order_acquire);
}

[[nodiscard]] inline bool
__wave_settle(__io_wave &__wv) noexcept
{
  if ( __wv.__st.get(micron::memory_order_acquire) != __io_st_resumed ) return true;
  for ( u64 __i = 0; __wv.__fin.get(micron::memory_order_acquire) == 0; ++__i ) {
    if ( __i >= (1ull << 24) ) {
      __io_wave_stranded.fetch_add(1, micron::memory_order_acq_rel);
      return false;
    }
    if ( (__i & 1023u) == 1023u )
      micron::yield();
    else
      micron::cpu_pause<1>();
  }
  return true;
}

inline void
__wave_drain_cancel(__io_wave &__wv, __io_wop *__nodes, u32 __k, i32 __rid) noexcept
{
  if ( __wv.__left.get(micron::memory_order_acquire) == 0 ) return;
  __wring *__r = wave_ring_of(__rid);
  if ( __r == nullptr || __global_engine == nullptr ) {
    __wv.__left.store(0, micron::memory_order_release);      // never reached a real ring
    return;
  }
  for ( u32 __i = 0; __i < __k; ++__i ) __io_sync_cancel_ud(__r->__r.fd, __io_ud_make(__io_ud_wop, reinterpret_cast<u64>(&__nodes[__i])));
  while ( __wv.__left.get(micron::memory_order_acquire) != 0 ) {
    if ( __io_cq_acquire(*__r) ) {
      micron::uring::cqe __c{};
      while ( __r->__r.peek_cqe(&__c) ) __global_engine->__dispatch_cqe(*__r, __c);
      __io_unlock(__r->__cq_lk);
      continue;
    }
    if ( __r->__live.get(micron::memory_order_acquire) == 0 ) break;      // ring torn down
    micron::cpu_pause<1>();
  }
}

};      // namespace io
};      // namespace coro
};      // namespace micron

#endif
