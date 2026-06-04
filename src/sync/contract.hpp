//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/locks/spin_lock.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../queue/lambda_queue.hpp"
#include "../queue/queue.hpp"

#include "../atomic/atomic.hpp"

#include "../chrono.hpp"
#include "pause.hpp"

#include "../thread/spawn.hpp"

namespace micron
{

// contract{} has nothing to do with cpp26 contracts other than borrowing it's name and gen. terminology (because it sounds nice).
// contracts are a high-level process synchronization system, by which you are able to (with near zero abstraction cost) make certain
// guarantees as to the state of your process. can be used either in multithreaded environments, or singlethreaded. on contract
// construction, achieved by passing in a function + args, a new contract is created, but it isn't yet signed off. once sign(contract_state)
// is called (either with void add. arg, or another function - 'the enforcing func') the contract is then being enforced. on destructor,
// or a valid trigger condition of the enforcing func, the contract runs, resulting in either having fulfilled your obligation, or a
// contract violation.

enum class contract_state : i32 { lenient, enforcing, strict, __end };

template<contract_state S, typename T> class contract;

template<contract_state S, typename T, typename Fn>
void
violation(contract<S, T> *c, Fn *fn)
{
  (void)c;
  if constexpr ( S == contract_state::lenient ) {
    (void)fn;
  } else if constexpr ( S == contract_state::enforcing ) {
    // enforcing: run the (optional) enforcing callback; if it returns true the breach is fatal -> abort; abort() is noreturn and safe to
    // call from anywhere (incl. a destructor), unlike exc<>
    if ( fn != nullptr && fn->call() ) micron::abort();
  } else if constexpr ( S == contract_state::strict ) {
    exc<except::future_error>("contract::violation(): strict enforcement, contract violation detected");
  }
}

template<typename T> using requirement_t = micron::atomic_token<T>;

template<contract_state S, typename T> class contract
{

  struct fn_base_t {
    virtual bool call() = 0;
    virtual ~fn_base_t() = default;
  };

  template<typename Fn> struct fn_t: fn_base_t {
    Fn fn;

    fn_t(Fn &&f) : fn(micron::move(f)) { }

    bool
    call() override
    {
      if constexpr ( !micron::is_same_v<micron::invoke_result_t<Fn>, void> )
        return fn();
      else
        static_assert(false, "contract function must return");
    }
  };

  micron::atomic_token<bool> is_signed;
  micron::atomic_token<bool> __violated;
  fduration_t __duration;
  // the enforcing function runs within the lambda which is spawned off in a separate thread. will be called exclusively from violation(),
  // essentially acts as a callback
  fn_base_t *__enforcing_fn;
  // the condition function determines whether the contract succeeds/is satisfied or not
  fn_base_t *__condition_fn;
  // may be added to the contract, all must evaluate to true for the contract to succeed
  micron::queue<requirement_t<T> *> __requirements;

  // non-throwing predicate: are all requirements + the condition currently satisfied?  callable from the
  // detached worker (records a violation flag, never throws / never calls the throwing violation())
  bool
  __all_satisfied(void)
  {
    for ( requirement_t<T> *const *it = __requirements.begin(); it != __requirements.end(); ++it ) {
      requirement_t<T> *req = *it;
      if ( req == nullptr || !req->get(memory_order_acquire) ) return false;
    }
    if ( __condition_fn != nullptr && !__condition_fn->call() ) return false;
    return true;
  }

  void
  __spawn_worker(void)
  {
    go([this]() {
      // __duration <= 0 => evaluate once, no waiting. otherwise treat __duration as the total budget (ms)
      const fduration_t budget = this->__duration;
      const fduration_t deadline = micron::system_clock<>::now() + (budget > 0 ? budget : 0);
      // poll slice: a fraction of the budget, clamped to a sane [1ms, 50ms] range
      const fduration_t slice = budget > 0 ? (budget > 50.0 ? 50.0 : (budget < 1.0 ? budget : budget / 4.0)) : 0;
      bool ok = this->__all_satisfied();
      while ( !ok && budget > 0 && micron::system_clock<>::now() < deadline ) {
        sleep_duration(slice);
        ok = this->__all_satisfied();
      }
      if ( !ok ) this->__violated.store(true, memory_order_release);
      // release the contract last; ~contract/wait() observe this with acquire and then surface a breach
      this->is_signed.store(false, memory_order_release);
    });
  }

public:
  constexpr static contract_state State = S;

  ~contract()
  {
    // if contract has been signed, the worker is in flight; it is deadline-bounded so this terminates;
    // it clears is_signed on completion. we never re-enter the worker, only observe its result
    while ( is_signed.get(memory_order::acquire) ) cpu_pause<5000>();
    if ( __violated.get(memory_order::acquire) || (__condition_fn != nullptr && !__all_satisfied()) ) {
      if constexpr ( S == contract_state::enforcing ) violation(this, __enforcing_fn);
      // strict/lenient breach is recorded but not thrown from the destructor (use wait() to surface it)
    }
    if ( __enforcing_fn != nullptr ) {
      delete __enforcing_fn;
      __enforcing_fn = nullptr;
    }
    if ( __condition_fn != nullptr ) {
      delete __condition_fn;
      __condition_fn = nullptr;
    }
  }

  contract(void) = delete;

  template<typename Fn, typename... Args>
  contract(Fn &&fn, Args &&...args)
      : is_signed{ false }, __violated{ false }, __duration{ 0 }, __enforcing_fn(nullptr), __condition_fn(nullptr), __requirements{}
  {
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    __condition_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
  }

  contract(const contract &o) = delete;

  contract(contract &&o)
      : is_signed{ micron::move(o.is_signed) }, __violated{ micron::move(o.__violated) }, __duration{ o.__duration },
        __enforcing_fn(o.__enforcing_fn), __condition_fn(o.__condition_fn), __requirements{ micron::move(o.__requirements) }
  {
    o.__condition_fn = nullptr;
    o.__enforcing_fn = nullptr;
  };

  contract &operator=(const contract &) = delete;
  contract &operator=(contract &&o) = delete;

  void
  sign(void)
  {
    bool expected = false;
    if ( !is_signed.compare_exchange_strong(expected, true, memory_order_acq_rel) ) [[unlikely]]
      exc<except::future_error>("contract::sign(): was already signed");
    __spawn_worker();
  }

  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args...>)
  void
  sign(Fn &&fn, Args &&...args)
  {
    if ( __enforcing_fn != nullptr ) exc<except::future_error>("contract::sign(): already has a stored enforcing function");
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    fn_base_t *enf = new fn_t<decltype(__fn)>(micron::move(__fn));
    bool expected = false;
    if ( !is_signed.compare_exchange_strong(expected, true, memory_order_acq_rel) ) [[unlikely]] {
      delete enf;
      exc<except::future_error>("contract::sign(): was already signed");
    }
    __enforcing_fn = enf;
    __spawn_worker();
  }

  // TODO: think about making this private
  void
  satisfy(void)
  {
    is_signed.store(false, memory_order_release);
  }

  void
  wait(void)
  {
    while ( is_signed.get(memory_order::acquire) ) cpu_pause<5000>();
    if ( __violated.get(memory_order::acquire) ) violation(this, __enforcing_fn);
  }

  bool
  violated(void) const
  {
    return __violated.get(memory_order::acquire);
  }

  void
  duration(fduration_t tm)
  {
    __duration = tm;
  }

  template<typename Fn, typename... Args>
  void
  enforce(Fn &&fn, Args &&...args)
  {
    if ( __enforcing_fn != nullptr ) delete __enforcing_fn;
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    __enforcing_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
  }

  template<typename... Args>
    requires((micron::is_same_v<micron::__remove_cvref_t<Args>, requirement_t<T>> && ...))
  void
  require(Args &...args)
  {
    (__requirements.push(micron::addressof(args)), ...);
  }
};
};      // namespace micron
