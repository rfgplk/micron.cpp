//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../__special/coroutine"

#include "../../atomic/atomic.hpp"
#include "../../queue/crossbeam.hpp"
#include "../../sync/futex.hpp"
#include "../../sync/futex_future.hpp"
#include "../../sync/yield.hpp"
#include "../../tasks/coro_core.hpp"
#include "../../tasks/task.hpp"
#include "../../thread/cpu.hpp"
#include "../../thread/thread.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/sys/time.hpp"
#include "fiber.hpp"

// TODO: this is only here temporarily move it out to */coroutine (somewhere else?)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// continuation stealing scheduler

namespace micron
{
namespace coro
{

inline constexpr u32 __cl_max_workers = 32;

// resume a continuation handle on the current worker's fresh fiber
inline void
__cl_resume_entry(micron::fiber::fiber *self) noexcept
{
  static_cast<__frame_base *>(self->arg)->__self.resume();
}

struct engine {
  worker *workers = nullptr;
  micron::crossbeam<__frame_base *, 256> inbox;      // externally submitted roots
  micron::atomic_token<u32> stopping{ 0 };
  micron::atomic_token<u32> busy{ 0 };                // num of workers currently running a fiber
  micron::atomic_token<u32> pending_timers{ 0 };      // num of frames in the timer
  u32 n = 0;
  micron::__thread_pointer<micron::auto_thread<>> threads[__cl_max_workers];

  ~engine() { delete[] workers; }

  engine() noexcept = default;

  [[gnu::always_inline]] void
  __run(worker *w, __frame_base *cont, bool stolen) noexcept
  {
    // NOTE: busy counter is held by worker_main across __find + __run
    (void)w;
    if ( stolen ) cont->__steals.fetch_add(1, micron::memory_order_acq_rel);
    micron::fiber::fiber *f = micron::fiber::create_fiber(&__cl_resume_entry, cont, static_cast<usize>(small_stack_size));
    while ( f == nullptr ) [[unlikely]] {
      // stack/VA reservation is exhausted
      if ( stopping.get(micron::memory_order_acquire) ) return;
      micron::yield();
      f = micron::fiber::create_fiber(&__cl_resume_entry, cont, static_cast<usize>(small_stack_size));
    }
    micron::fiber::resume(f);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    // hard trap out if an exception escaped the coroutine
    if ( f->escaped ) [[unlikely]]
      __builtin_trap();
#endif
    micron::fiber::destroy_fiber(f);
  }

  __frame_base *
  __steal(worker *w, u32 &seed) noexcept
  {
    if ( n <= 1 ) return nullptr;
    for ( u32 attempt = 0; attempt < n * 2u; ++attempt ) {
      seed ^= seed << 13;
      seed ^= seed >> 17;
      seed ^= seed << 5;
      u32 v = seed % n;
      if ( v == w->id ) continue;
      __frame_base *c = workers[v].deque.steal_top();
      if ( c != nullptr ) return c;
    }
    return nullptr;
  }

  __frame_base *
  __find(worker *w, u32 &seed, bool &stolen) noexcept
  {
    __frame_base *cont = nullptr;
    if ( inbox.pop(cont) && cont != nullptr ) {
      stolen = false;
      return cont;
    }
    cont = __steal(w, seed);
    stolen = true;
    return cont;
  }

  void
  worker_main(u32 id) noexcept
  {
    worker *w = &workers[id];
    __cur_worker = w;
    u32 seed = id * 2654435761u + 1u;
    // ensure that theres no  interval where a continuation left the inbox/deque but is not yet counted in busy
    for ( ;; ) {
      if ( stopping.get(micron::memory_order_acquire) ) break;
      bool stolen = false;
      busy.fetch_add(1, micron::memory_order_acq_rel);
      __frame_base *cont = __find(w, seed, stolen);
      if ( cont != nullptr ) {
        __run(w, cont, stolen);
        busy.sub_fetch(1, micron::memory_order_acq_rel);
        continue;
      }
      busy.sub_fetch(1, micron::memory_order_acq_rel);
      // nothing found after a full steal sweep
      __cl_sleepers.fetch_add(1, micron::memory_order_acq_rel);      // announce parked
      const u32 sig = __cl_signal.get(micron::memory_order_acquire);
      busy.fetch_add(1, micron::memory_order_acq_rel);
      cont = __find(w, seed, stolen);
      if ( cont != nullptr ) {
        __cl_sleepers.sub_fetch(1, micron::memory_order_acq_rel);
        if ( !stopping.get(micron::memory_order_acquire) ) __run(w, cont, stolen);
        busy.sub_fetch(1, micron::memory_order_acq_rel);
        continue;
      }
      busy.sub_fetch(1, micron::memory_order_acq_rel);
      if ( !stopping.get(micron::memory_order_acquire) ) {
        timespec_t __ts{ 0, 100000000 };
        micron::__futex(__cl_signal.ptr(), futex_wait | futex_private_flag, sig, &__ts, nullptr, 0);
      }
      __cl_sleepers.sub_fetch(1, micron::memory_order_acq_rel);
    }
    micron::fiber::drain_freelist();
  }

  void
  submit(__frame_base *fb) noexcept
  {
    while ( !inbox.push(fb) ) micron::yield();      // inbox full
    __cl_signal.fetch_add(1, micron::memory_order_release);
    micron::wake_futex(__cl_signal.ptr(), static_cast<int>(n));      // wake all
  }
};

inline engine *__global_engine = nullptr;
// 0 = uninitialized
// 1 = a thread is constructing the engine
// 2 = engine published + ready
inline micron::atomic_token<u32> __engine_state{ 0 };

struct __timer_node {
  timespec_t __deadline;
  __frame_base *__frame = nullptr;
  __timer_node *__next = nullptr;
};

inline __timer_node *__timer_head = nullptr;
inline micron::atomic_token<u32> __timer_lk{ 0 };
inline micron::__thread_pointer<micron::auto_thread<>> __timer_thread;

inline void
__timer_lock() noexcept
{
  u32 __e = 0;
  while ( !__timer_lk.compare_exchange_weak(__e, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __e = 0;
    micron::cpu_pause<1>();
  }
}

inline void
__timer_unlock() noexcept
{
  __timer_lk.store(0u, micron::memory_order_release);
}

[[gnu::always_inline]] inline bool
__ts_le(const timespec_t &__a, const timespec_t &__b) noexcept
{
  return __a.tv_sec < __b.tv_sec || (__a.tv_sec == __b.tv_sec && __a.tv_nsec <= __b.tv_nsec);
}

inline void
__timer_main() noexcept
{
  const timespec_t __tick{ 0, 1000000 };      // 1ms poll
  for ( ;; ) {
    engine *__e = __global_engine;
    if ( __e == nullptr || __e->stopping.get(micron::memory_order_acquire) ) break;
    timespec_t __now{};
    micron::clock_gettime(micron::clock_monotonic, __now);
    __timer_node *__fired = nullptr;
    __timer_lock();
    __timer_node **__pp = &__timer_head;
    while ( *__pp != nullptr ) {
      if ( __ts_le((*__pp)->__deadline, __now) ) {
        __timer_node *__n = *__pp;
        *__pp = __n->__next;
        __n->__next = __fired;
        __fired = __n;
      } else {
        __pp = &(*__pp)->__next;
      }
    }
    __timer_unlock();
    while ( __fired != nullptr ) {
      __timer_node *__nx = __fired->__next;      // read before submit (the resumed frame may free the node)
      __frame_base *__f = __fired->__frame;
      __e->pending_timers.sub_fetch(1, micron::memory_order_acq_rel);
      __e->submit(__f);
      __fired = __nx;
    }
    micron::nanosleep(__tick);
  }
}

inline void
start_coroutine_runtime(u32 nworkers = 0) noexcept
{
  if ( __engine_state.get(micron::memory_order_acquire) == 2u ) return;

  u32 __exp = 0u;
  if ( !__engine_state.compare_exchange_strong(__exp, 1u, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    while ( __engine_state.get(micron::memory_order_acquire) != 2u ) micron::yield();      // loser: wait for ready
    return;
  }

  if ( nworkers == 0 ) nworkers = micron::cpu_count();
  if ( nworkers > __cl_max_workers ) nworkers = __cl_max_workers;
  if ( nworkers < 1 ) nworkers = 1;

  engine *e = new engine();
  e->n = nworkers;
  e->workers = new worker[nworkers];
  for ( u32 i = 0; i < nworkers; ++i ) e->workers[i].id = i;
  __global_engine = e;
  for ( u32 i = 0; i < nworkers; ++i )
    e->threads[i] = micron::solo::spawn<micron::auto_thread<>>([](engine *eng, u32 id) { eng->worker_main(id); }, e, i);
  __timer_thread = micron::solo::spawn<micron::auto_thread<>>([]() { __timer_main(); });
  __engine_state.store(2u, micron::memory_order_release);
}

inline void
stop_coroutine_runtime() noexcept
{
  engine *e = __global_engine;
  if ( e == nullptr ) return;

  {
    u32 __quiet = 0;
    while ( __quiet < 4 ) {
      bool __q
          = e->inbox.empty() && e->busy.get(micron::memory_order_acquire) == 0 && e->pending_timers.get(micron::memory_order_acquire) == 0;
      for ( u32 __i = 0; __q && __i < e->n; ++__i )
        if ( !e->workers[__i].deque.empty() ) __q = false;
      if ( __q )
        ++__quiet;
      else
        __quiet = 0;
      micron::yield();
    }
  }

  e->stopping.store(1, micron::memory_order_release);
  __timer_thread.reset();
  __cl_signal.fetch_add(1, micron::memory_order_release);
  micron::wake_futex(__cl_signal.ptr(), static_cast<int>(e->n));
  for ( u32 i = 0; i < e->n; ++i ) e->threads[i].reset();
  __global_engine = nullptr;
  delete e;
  __engine_state.store(0u, micron::memory_order_release);
}

struct __sync_bridge {
  struct promise_type: __frame_base {
    micron::atomic_token<u32> *__done = nullptr;

    __sync_bridge
    get_return_object() noexcept
    {
      auto __h = std::coroutine_handle<promise_type>::from_promise(*this);
      this->__self = __h;
      return __sync_bridge{ __h };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    auto
    final_suspend() noexcept
    {
      struct __fa {
        bool
        await_ready() const noexcept
        {
          return false;
        }

        void
        await_suspend(std::coroutine_handle<promise_type> __h) const noexcept
        {
          micron::atomic_token<u32> *__d = __h.promise().__done;
          __d->store(1, micron::memory_order_release);
          micron::wake_futex(__d->ptr());
        }

        void
        await_resume() const noexcept
        {
        }
      };

      return __fa{};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
      __builtin_trap();
    }

    static void *
    operator new(usize __n) noexcept
    {
      return micron::fiber::__frame_alloc(__n);
    }

    static void
    operator delete(void *__p, usize __n) noexcept
    {
      micron::fiber::__frame_free(__p, __n);
    }

    static __sync_bridge get_return_object_on_allocation_failure() noexcept;
  };

  std::coroutine_handle<promise_type> __h{};
  __sync_bridge() noexcept = default;

  explicit __sync_bridge(std::coroutine_handle<promise_type> __hh) noexcept : __h(__hh) { }
};

inline __sync_bridge
__sync_bridge::promise_type::get_return_object_on_allocation_failure() noexcept
{
  return __sync_bridge{};
}

template<class T>
decltype(auto)
sync_wait(task<T> &&root)
{
  start_coroutine_runtime();
  micron::atomic_token<u32> done{ 0 };

  if constexpr ( micron::is_void_v<T> ) {
    __sync_bridge bridge = [](task<void> __r) -> __sync_bridge { co_await micron::move(__r); }(micron::move(root));
    bridge.__h.promise().__done = &done;
    __global_engine->submit(&bridge.__h.promise());
    micron::wait_futex(done.ptr(), 0u);
    bridge.__h.destroy();
    return;
  } else {
    alignas(T) unsigned char storage[sizeof(T)];
    T *slot = reinterpret_cast<T *>(storage);
    __sync_bridge bridge = [](task<T> __r, T *__s) -> __sync_bridge {
      // NOTE: the co_await is split out of the placement-new to dodge a GCC ICE (co_await inside a new-expr)
      T __v = co_await micron::move(__r);
      ::new (static_cast<void *>(__s)) T(micron::move(__v));
    }(micron::move(root), slot);
    bridge.__h.promise().__done = &done;
    __global_engine->submit(&bridge.__h.promise());
    micron::wait_futex(done.ptr(), 0u);
    bridge.__h.destroy();
    T r = micron::move(*slot);
    slot->~T();
    return r;
  }
}

struct __detached_bridge {
  struct promise_type: __frame_base {
    __detached_bridge
    get_return_object() noexcept
    {
      auto __h = std::coroutine_handle<promise_type>::from_promise(*this);
      this->__self = __h;
      return __detached_bridge{ __h };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    std::suspend_never
    final_suspend() noexcept
    {
      return {};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
      __builtin_trap();
    }

    static void *
    operator new(usize __n) noexcept
    {
      return ::operator new(__n);
    }

    static void
    operator delete(void *__p, usize) noexcept
    {
      ::operator delete(__p);
    }

    static __detached_bridge
    get_return_object_on_allocation_failure() noexcept
    {
      return {};
    }
  };

  std::coroutine_handle<promise_type> __h{};
  __detached_bridge() noexcept = default;

  explicit __detached_bridge(std::coroutine_handle<promise_type> __hh) noexcept : __h(__hh) { }
};

template<class T>
void
detach(task<T> &&__root)
{
  start_coroutine_runtime();
  __detached_bridge __b = [](task<T> __r) -> __detached_bridge { co_await micron::move(__r); }(micron::move(__root));
  if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
}

template<class T>
[[nodiscard]] micron::futex_future<T>
schedule(task<T> &&__root)
{
  start_coroutine_runtime();
  micron::futex_promise<T> __prom;
  micron::futex_future<T> __fut = __prom.get_future();
  if constexpr ( micron::is_void_v<T> ) {
    __detached_bridge __b = [](task<void> __r, micron::futex_promise<void> __p) -> __detached_bridge {
      co_await micron::move(__r);
      __p.set_value();
    }(micron::move(__root), micron::move(__prom));
    if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
  } else {
    __detached_bridge __b = [](task<T> __r, micron::futex_promise<T> __p) -> __detached_bridge {
      T __v = co_await micron::move(__r);      // split co_await out of set_value (GCC ICE on co_await in a call arg)
      __p.set_value(micron::move(__v));
    }(micron::move(__root), micron::move(__prom));
    if ( __b.__h ) __global_engine->submit(&__b.__h.promise());
  }
  return __fut;
}

template<class T> struct __futex_future_awaiter {
  micron::futex_shared_state<T> *__s;

  bool
  await_ready() const noexcept
  {
    return __s == nullptr || __s->is_ready();
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();
    bool __ready
        = __s->__arm(reinterpret_cast<usize>(__f), +[](usize __t) { __global_engine->submit(reinterpret_cast<__frame_base *>(__t)); });
    return !__ready;
  }

  decltype(auto)
  await_resume()
  {
    if ( __s == nullptr ) [[unlikely]]
      exc<except::future_error>("micron::futex_future: co_await on a future with no shared state");
    return __s->get();
  }
};

struct __reschedule_awaitable {
  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __frame_base *__f = &__h.promise();      // already suspended here; safe to publish (same rule as fork)
    worker *__w = current_worker();
    if ( __w != nullptr && __w->deque.push_bottom(__f) ) {
      __notify_work();
      return true;      // suspended; a worker (maybe this one) resumes it
    }
    return false;      // off-engine or full deque: no-op yield (resume inline)
  }

  void
  await_resume() const noexcept
  {
  }
};

[[nodiscard]] inline __reschedule_awaitable
reschedule() noexcept
{
  return {};
}

struct __sel_state {
  micron::atomic_token<i32> __winner{ -1 };
  micron::atomic_token<usize> __refs;
  __frame_base *__parent = nullptr;

  struct __node {
    __sel_state *__st;
    i32 __idx;
  } *__nodes = nullptr;      // heap array sized to n (no fixed cap); freed with the state on the last __rel

  explicit __sel_state(usize __r, usize __n) noexcept : __refs(__r), __nodes(__n ? new __node[__n] : nullptr) { }

  ~__sel_state() noexcept
  {
    if ( __nodes != nullptr ) delete[] __nodes;
  }

  void
  __rel() noexcept
  {
    if ( __refs.sub_fetch(1, micron::memory_order_acq_rel) == 0 ) delete this;      // ~__sel_state frees __nodes
  }
};

inline void
__sel_fire(usize __t) noexcept
{
  __sel_state::__node *__n = reinterpret_cast<__sel_state::__node *>(__t);
  __sel_state *__s = __n->__st;
  i32 __exp = -1;
  if ( __s->__winner.compare_exchange_strong(__exp, __n->__idx, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    __s->__rel();
    return;
  }
  if ( __exp == -2 ) {
    i32 __e2 = -2;
    if ( __s->__winner.compare_exchange_strong(__e2, __n->__idx, micron::memory_order_acq_rel, micron::memory_order_acquire) )
      __global_engine->submit(__s->__parent);
  }
  __s->__rel();
}

template<class T> struct [[nodiscard]] __when_any_awaiter {
  micron::futex_future<T> *__futs;
  usize __n;
  __sel_state *__st = nullptr;

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    for ( usize __i = 0; __i < __n; ++__i )
      if ( __futs[__i].__shared() == nullptr ) [[unlikely]]
        exc<except::future_error>("micron::when_any: co_await on a future with no shared state");
    __st = new __sel_state(__n + 1u, __n);
    __st->__parent = &__h.promise();
    for ( usize __i = 0; __i < __n; ++__i ) {
      __st->__nodes[__i] = { __st, static_cast<i32>(__i) };
      bool __ready = __futs[__i].__shared()->__arm(reinterpret_cast<usize>(&__st->__nodes[__i]), __sel_fire);
      if ( __ready ) __sel_fire(reinterpret_cast<usize>(&__st->__nodes[__i]));      // already ready: fire inline
    }
    i32 __exp = -1;
    if ( __st->__winner.compare_exchange_strong(__exp, -2, micron::memory_order_acq_rel, micron::memory_order_acquire) ) return true;
    return false;
  }

  i32
  await_resume() noexcept
  {
    i32 __w = __st->__winner.get(micron::memory_order_acquire);
    __st->__rel();
    return (__w >= 0) ? __w : -1;
  }
};

// co_await when_any(futs, n) -> index of the first ready future
template<class T>
[[nodiscard]] __when_any_awaiter<T>
when_any(micron::futex_future<T> *__futs, usize __n) noexcept
{
  return __when_any_awaiter<T>{ __futs, __n };
}

// TODO: implement select over arbitrary tasks
struct [[nodiscard]] __sleep_awaiter {
  timespec_t __deadline;
  __timer_node __node{};

  bool
  await_ready() noexcept
  {
    timespec_t __now{};
    micron::clock_gettime(micron::clock_monotonic, __now);
    return __ts_le(__deadline, __now);      // already elapsed -> don't suspend
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __node.__deadline = __deadline;
    __node.__frame = &__h.promise();
    __global_engine->pending_timers.fetch_add(1, micron::memory_order_acq_rel);
    __timer_lock();
    __node.__next = __timer_head;
    __timer_head = &__node;
    __timer_unlock();
    return true;
  }

  void
  await_resume() const noexcept
  {
  }
};

[[nodiscard]] inline __sleep_awaiter
sleep_until(const timespec_t &__deadline_monotonic) noexcept
{
  return __sleep_awaiter{ __deadline_monotonic };
}

[[nodiscard]] inline __sleep_awaiter
sleep_for(u64 __nanos) noexcept
{
  timespec_t __dl{};
  micron::clock_gettime(micron::clock_monotonic, __dl);
  __dl.tv_sec += static_cast<decltype(__dl.tv_sec)>(__nanos / 1000000000ull);
  __dl.tv_nsec += static_cast<decltype(__dl.tv_nsec)>(__nanos % 1000000000ull);
  if ( __dl.tv_nsec >= 1000000000l ) {
    __dl.tv_sec += 1;
    __dl.tv_nsec -= 1000000000l;
  }
  return __sleep_awaiter{ __dl };
}

[[nodiscard]] inline __sleep_awaiter
sleep_for_ms(u64 __ms) noexcept
{
  return sleep_for(__ms * 1000000ull);
}

};      // namespace coro

// ADL operator co_await for a futex_future
template<class T>
inline coro::__futex_future_awaiter<T>
operator co_await(const micron::futex_future<T> &__f) noexcept
{
  return coro::__futex_future_awaiter<T>{ __f.__shared() };
}

};      // namespace micron
