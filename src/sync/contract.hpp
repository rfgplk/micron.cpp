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

template <contract_state S, typename T> class contract;

template <contract_state S, typename T, typename Fn>
void
violation(contract<S, T> *c, Fn *fn)
{
  if ( c->State == contract_state::lenient ) {
  } else if ( c->State == contract_state::enforcing ) {
    if ( fn == nullptr )
      exc<except::future_error>("contract::violation(): no enforcing function provided");
    if ( fn->call() )
      mc::abort();
  } else if ( c->State == contract_state::strict ) {
    exc<except::future_error>("contract::violation(): strict enforcement, contract violation detected");
  }
}

template <typename T> using requirement_t = micron::atomic_token<T>;

template <contract_state S, typename T> class contract
{

  struct fn_base_t {
    virtual bool call() = 0;
    virtual ~fn_base_t() = default;
  };

  template <typename Fn> struct fn_t : fn_base_t {
    Fn fn;

    fn_t(Fn &&f) : fn(micron::move(f)) {}

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
  fduration_t __duration;
  // the enforcing function runs within the lambda which is spawned off in a separate thread. will be called exclusively from violation(),
  // essentially acts as a callback
  fn_base_t *__enforcing_fn;
  // the condition function determines whether the contract succeeds/is satisfied or not
  fn_base_t *__condition_fn;
  // may be added to the contract, all must evaluate to true for the contract to succeed
  micron::queue<requirement_t<T> *> __requirements;

  void
  __check_reqs(void)
  {
    while ( !__requirements.empty() ) {
      auto &req = __requirements.front();
      if ( !req->get(memory_order_acquire) )
        violation(this, __enforcing_fn);
      __requirements.pop();
    }
    if ( !__condition_fn->call() )
      violation(this, __enforcing_fn);
  }

public:
  constexpr static contract_state State = S;

  ~contract()
  {
    // condition doesn't matter for the destructor, only lambdas
    // if condition has been freed, no need to recheck
    if ( __condition_fn ) {
      {
        //  if contract has been signed, cannot destroy, stall
        while ( is_signed.get(memory_order::acquire) )
          cpu_pause<5000>();
        __check_reqs();
        if ( __condition_fn )
          delete __condition_fn;
      }
      if ( __enforcing_fn )
        delete __enforcing_fn;
      if ( __condition_fn )
        delete __condition_fn;
    }
  }

  contract(void) = delete;

  template <typename Fn, typename... Args>
  contract(Fn &&fn, Args &&...args) : is_signed{ false }, __duration{ 0 }, __enforcing_fn(nullptr), __requirements{}
  {
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    __condition_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
  }

  contract(const contract &o) = delete;

  contract(contract &&o)
      : is_signed{ micron::move(o.is_signed) }, __duration{ micron::move(o.duration) }, __condition_fn(o.__condition_fn),
        __enforcing_fn(o.__enforcing_fn), __requirements{ micron::move(o.__requirements) }
  {
    o.__condition_fn = nullptr;
    o.__enforcing_fn = nullptr;
  };

  contract &operator=(const contract &) = delete;
  contract &operator=(contract &&o) = delete;

  void
  sign(void)
  {
    if ( is_signed.get(memory_order::acquire) ) [[unlikely]]
      exc<except::future_error>("contract::sign(): was already signed");
    is_signed.store(true, memory_order::acquire);
    go([this]() {
      if ( this->__condition_fn != nullptr ) {
      resleep:
        // if the condition hasn't been met cannot proceed
        sleep_duration(this->__duration);
        if ( !__condition_fn->call() )
          goto resleep;
        __check_reqs();
      }
      {
        satisfy();
        return;
        // always good, cntrct satisfied
      }
    });
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args...>)
  void
  sign(Fn &&fn, Args &&...args)
  {
    if ( is_signed.get(memory_order::acquire) ) [[unlikely]]
      exc<except::future_error>("contract::sign(): was already signed");
    if ( __enforcing_fn != nullptr )
      exc<except::future_error>("contract::sign(): already has a stored enforcing function");
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    __enforcing_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
    is_signed.store(true, memory_order::acquire);
    go([this]() {
      if ( this->__condition_fn != nullptr ) {
      resleep:
        sleep_duration(this->__duration);
        if ( !__condition_fn->call() )
          goto resleep;
        __check_reqs();
      }
      {
        satisfy();
        return;
      }
    });
  }

  // TODO: think about making this private
  void
  satisfy(void)
  {
    is_signed.store(false, memory_order_release);
  }

  void
  duration(fduration_t tm)
  {
    __duration = tm;
  }

  template <typename Fn, typename... Args>
  void
  enforce(Fn &&fn, Args &&...args)
  {
    if ( __enforcing_fn != nullptr )
      delete __enforcing_fn;
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { return f(a...); };
    __enforcing_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
  }

  template <typename... Args>
    requires(micron::is_same_v<Args, requirement_t<T>>, ...)
  void
  require(Args &&...args)
  {
    __requirements.emplace_back(micron::forward<Args &&>(args)...);
  }
};
};     // namespace micron
