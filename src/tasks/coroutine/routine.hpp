//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../memory/actions.hpp"
#include "../../tuple.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "fiber.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// stackful coroutine porcelain
//
// spin / launch / jump / yield
//
// companion to the solo:: thread porcelain
//
// issues:
//   .. destroying a suspended routine abandons it: the stack is released with no unwinding
//   .. yield(v) with a type other than the routine's Y reads back type-confused (TODO: will be enforced later)
//   .. a suspended routine may be jumped or destroyed from another thread, but never concurrently.

// example:
//   auto r = coro::spin<int>(fn, args...);   // create, paused (lazy; ~create_fiber cost)
//   auto r = coro::launch<int>(fn, args...); // create + run to the first yield (eager)
//   int *v = coro::jump(r);                  // enter r; returns at its next yield (value) or finish (nullptr)
//   coro::yield(42);                         // inside fn: hand 42 to the jumper, park here
//   coro::yield();                           // inside fn: park without a value
//   r.result();                              // finish() + move out fn's return value
namespace micron
{
namespace coro
{

struct __spin_ctl {
  ar::__fiber_ctx jumper;                                  // jump() parks the jumper here; f->link -> &jumper
  void *xfer = nullptr;                                    // yielded-value pointer; nulled past completion
  void *res = nullptr;                                     // -> the block's __spin_result<R> base (exact upcast)
  void (*drop)(__spin_ctl *, u32) noexcept = nullptr;      // type-erased cleanup, see __drop_*
  const char *err = nullptr;                               // escaped-exception message (hosted/EH)
};

inline constexpr u32 __drop_block = 0;       // destroy fn + args (never consumed: fresh routine dropped)
inline constexpr u32 __drop_result = 1;      // destroy an unconsumed result (done routine dropped)

template<class R> struct __spin_result {
  alignas(R) byte __slot[sizeof(R)];
  bool __has = false;
};

template<> struct __spin_result<void> {
};

template<class R, class F, class... A> struct __spin_block: __spin_ctl, __spin_result<R> {
  F __fn;
  micron::tuple<A...> __args;      // by-value homes for the launch args; consumed as rvalues at invocation

  template<class Fw, class... Aw>
  explicit __spin_block(Fw &&__f, Aw &&...__a) : __fn(micron::forward<Fw>(__f)), __args(micron::forward<Aw>(__a)...)
  {
  }
};

// innermost spun routine running on this thread
inline thread_local __spin_ctl *__cur_routine = nullptr;

template<class B, usize... Is>
[[gnu::always_inline]] inline decltype(auto)
__spin_apply(B *__b, micron::index_sequence<Is...>)
{
  return __b->__fn(micron::move(micron::get<Is>(__b->__args))...);
}

template<class R, class F, class... A>
inline void
__spin_drop(__spin_ctl *__c, u32 __what) noexcept
{
  auto *__b = static_cast<__spin_block<R, F, A...> *>(__c);
  if ( __what == __drop_block ) {
    __b->__fn.~F();
    __b->__args.~tuple<A...>();
    return;
  }
  if constexpr ( !micron::is_void_v<R> ) {
    if ( __b->__has ) {
      reinterpret_cast<R *>(__b->__slot)->~R();
      __b->__has = false;
    }
  }
}

// runs on the routine's fiber
template<class R, class F, class... A>
inline void
__spin_entry(fiber::fiber *__f) noexcept
{
  auto *__b = static_cast<__spin_block<R, F, A...> *>(static_cast<__spin_ctl *>(__f->arg));
#if !defined(__micron_freestanding) || defined(__micron_eh)
  try {
#endif
    if constexpr ( micron::is_void_v<R> )
      __spin_apply(__b, micron::index_sequence_for<A...>{});
    else {
      ::new (static_cast<void *>(__b->__slot)) R(__spin_apply(__b, micron::index_sequence_for<A...>{}));
      __b->__has = true;
    }
#if !defined(__micron_freestanding) || defined(__micron_eh)
  } catch ( ... ) {
    __b->err = "micron::coro::routine: unhandled exception";
  }
#endif
  __b->xfer = nullptr;      // no yield value past completion
  __b->__fn.~F();           // consume the launch state on-fiber, before the final park
  __b->__args.~tuple<A...>();
}

template<class Y = void, class R = void>
class [[nodiscard("a dropped micron::coro::routine releases its fiber; jump it, finish it, or keep the handle")]] routine
{
  fiber::fiber *__f = nullptr;
  __spin_ctl *__c = nullptr;      // == __f->arg, cached

  [[gnu::always_inline]] u32
  __state() const noexcept
  {
    return __f->state.get(micron::memory_order_acquire);
  }

  void
  __destroy() noexcept
  {
    if ( __f == nullptr ) return;
    const u32 __st = __state();
    if ( __st == static_cast<u32>(fiber::fiber_state::running) ) __builtin_trap();      // destroying a live activation
    if ( __st == static_cast<u32>(fiber::fiber_state::fresh) )
      __c->drop(__c, __drop_block);      // fn/args were never consumed; run their dtors exactly once
    else if ( __st == static_cast<u32>(fiber::fiber_state::done) )
      __c->drop(__c, __drop_result);      // unconsumed result, if any
    fiber::destroy_fiber(__f);
    __f = nullptr;
    __c = nullptr;
  }

public:
  using yield_type = Y;
  using result_type = R;

  // internal: handles are minted by spin()/launch()
  routine(fiber::fiber *__ff, __spin_ctl *__cc) noexcept : __f(__ff), __c(__cc) { }

  routine() noexcept = default;
  routine(const routine &) = delete;
  routine &operator=(const routine &) = delete;

  routine(routine &&__o) noexcept : __f(__o.__f), __c(__o.__c)
  {
    __o.__f = nullptr;
    __o.__c = nullptr;
  }

  routine &
  operator=(routine &&__o) noexcept
  {
    if ( this != &__o ) {
      __destroy();
      __f = __o.__f;
      __c = __o.__c;
      __o.__f = nullptr;
      __o.__c = nullptr;
    }
    return *this;
  }

  ~routine() { __destroy(); }

  bool
  done() const noexcept
  {
    return __f == nullptr || __state() == static_cast<u32>(fiber::fiber_state::done);
  }

  bool
  alive() const noexcept
  {
    return __f != nullptr && __state() != static_cast<u32>(fiber::fiber_state::done);
  }

  bool
  started() const noexcept
  {
    return __f != nullptr && __state() != static_cast<u32>(fiber::fiber_state::fresh);
  }

  explicit
  operator bool() const noexcept
  {
    return alive();
  }

  auto
  jump() noexcept(false) -> micron::conditional_t<micron::is_void_v<Y>, bool, Y *>
  {
    constexpr auto __none = [] {
      if constexpr ( micron::is_void_v<Y> )
        return false;
      else
        return static_cast<Y *>(nullptr);
    }();
    if ( __f == nullptr ) return __none;
    const u32 __st = __state();      // acquire: pairs with the suspend/done release for cross-thread jumps
    if ( __st == static_cast<u32>(fiber::fiber_state::done) ) return __none;
    if ( __st == static_cast<u32>(fiber::fiber_state::running) ) __builtin_trap();      // self/concurrent jump
    __f->state.store(static_cast<u32>(fiber::fiber_state::running), micron::memory_order_relaxed);
    __spin_ctl *__prev = __cur_routine;
    __cur_routine = __c;
    fiber::resume(__f, &__c->jumper);
    __cur_routine = __prev;
    if ( __c->err != nullptr ) [[unlikely]] {
      const char *__e = __c->err;
      __c->err = nullptr;
      micron::exc<except::library_error>(__e);
    }
    if ( __f->state.get(micron::memory_order_relaxed) == static_cast<u32>(fiber::fiber_state::done) ) return __none;
    if constexpr ( micron::is_void_v<Y> )
      return true;
    else
      return static_cast<Y *>(__c->xfer);
  }

  Y *
  value() const noexcept
    requires(!micron::is_void_v<Y>)
  {
    return __f != nullptr ? static_cast<Y *>(__c->xfer) : nullptr;
  }

  void
  finish()
  {
    while ( !done() ) (void)jump();
  }

  R
  result()
    requires(!micron::is_void_v<R>)
  {
    finish();
    if ( __f == nullptr ) [[unlikely]]
      micron::exc<except::library_error>("micron::coro::routine: result() on an empty handle");
    auto *__res = static_cast<__spin_result<R> *>(__c->res);
    if ( !__res->__has ) [[unlikely]]
      micron::exc<except::library_error>("micron::coro::routine: no result (fn did not complete)");
    __res->__has = false;
    R __r = micron::move(*reinterpret_cast<R *>(__res->__slot));
    reinterpret_cast<R *>(__res->__slot)->~R();
    return __r;
  }

  void
  dismiss() noexcept
  {
    __destroy();
  }

  fiber::fiber *
  native_handle() const noexcept
  {
    return __f;
  }

  usize
  stack_size() const noexcept
  {
    return __f != nullptr ? __f->region.stack_bytes : 0u;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// verbs

// create a stackful coroutine running fn(args...) on its own fiber
template<class Y = void, usize Stack = static_cast<usize>(micro_stack_size), class F, class... Args>
  requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args>...>)
[[nodiscard]] auto
spin(F &&__fn, Args &&...__args)
{
  using DF = micron::decay_t<F>;
  using R = micron::invoke_result_t<DF, micron::decay_t<Args>...>;
  using B = __spin_block<R, DF, micron::decay_t<Args>...>;

  fiber::fiber *__f = fiber::create_fiber(&__spin_entry<R, DF, micron::decay_t<Args>...>, nullptr, Stack);
  if ( __f == nullptr ) [[unlikely]]
    micron::exc<except::memory_error>("micron::coro::spin: fiber segment exhausted");
  const usize __p = (reinterpret_cast<usize>(__f->frame_sp) + alignof(B) - 1) & ~(alignof(B) - 1);
  if ( __p + sizeof(B) > reinterpret_cast<usize>(__f->region.frame_limit) ) [[unlikely]] {
    fiber::destroy_fiber(__f);
    micron::exc<except::memory_error>("micron::coro::spin: launch state exceeds the frame arena");
  }
  B *__b;
#if !defined(__micron_freestanding) || defined(__micron_eh)
  try {
    __b = ::new (reinterpret_cast<void *>(__p)) B(micron::forward<F>(__fn), micron::forward<Args>(__args)...);
  } catch ( ... ) {
    fiber::destroy_fiber(__f);      // a throwing fn/arg copy must not leak the fresh segment
    throw;
  }
#else
  __b = ::new (reinterpret_cast<void *>(__p)) B(micron::forward<F>(__fn), micron::forward<Args>(__args)...);
#endif
  __b->drop = &__spin_drop<R, DF, micron::decay_t<Args>...>;
  __b->res = static_cast<__spin_result<R> *>(__b);
  __f->frame_sp = reinterpret_cast<byte *>((__p + sizeof(B) + 15) & ~static_cast<usize>(15));
  __f->arg = static_cast<__spin_ctl *>(__b);
  return routine<Y, R>{ __f, static_cast<__spin_ctl *>(__b) };
}

// launch: eager spin
template<class Y = void, usize Stack = static_cast<usize>(micro_stack_size), class F, class... Args>
  requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args>...>)
[[nodiscard]] auto
launch(F &&__fn, Args &&...__args)
{
  auto __r = spin<Y, Stack>(micron::forward<F>(__fn), micron::forward<Args>(__args)...);
  (void)__r.jump();
  return __r;
}

// jump: free-function form
template<class Y, class R>
[[gnu::always_inline]] inline auto
jump(routine<Y, R> &__r)
{
  return __r.jump();
}

[[gnu::noinline]] inline void
__yield_park(void *__xfer) noexcept
{
  __spin_ctl *__c = __cur_routine;
  if ( __c == nullptr ) [[unlikely]]
    __builtin_trap();      // yield outside a spun routine
  __c->xfer = __xfer;
  fiber::suspend_to_scheduler();      // f->link == &ctl->jumper: straight back to the jumper
}

template<class Y>
inline void
yield(Y &&__v) noexcept
{
  __yield_park(const_cast<void *>(static_cast<const void *>(__builtin_addressof(__v))));
}

inline void
yield() noexcept
{
  __yield_park(nullptr);
}

[[gnu::noinline]] inline bool
in_routine() noexcept
{
  return __cur_routine != nullptr;
}

template<class... Rt>
inline bool
done(const Rt &...__rs) noexcept
{
  return (... && __rs.done());
}

template<class... Rt>
inline void
finish(Rt &...__rs)
{
  (__rs.finish(), ...);
}

template<class... Rt>
inline void
dismiss(Rt &...__rs) noexcept
{
  (__rs.dismiss(), ...);
}

template<class Y, class R>
inline R
result(routine<Y, R> &__r)
{
  return __r.result();
}

};      // namespace coro
};      // namespace micron
