//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/spinlock.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

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

template <contract_state S, typename T> class contract
{
  struct fn_base_t {
    virtual void call() = 0;
    virtual ~fn_base_t() = default;
  };

  template <typename Fn> struct fn_t : fn_base_t {
    Fn fn;

    fn_t(Fn &&f) : fn(micron::move(f)) {}

    void
    call() override
    {
      fn();
    }
  };

  bool is_signed;
  fduration_t __duration;
  fn_base_t *__enforcing_fn;
  fn_base_t *__condition_fn;
  atomic_token<T> *__requirement;

public:
  ~contract() {}

  template <typename Fn, typename... Args>
  contract(Fn &&fn, Args &&...args) : is_signed{ false }, __duration{ 0 }, __enforcing_fn(nullptr), __requirement{ nullptr }
  {
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { f(a...); };
    __condition_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
  }

  contract(const contract &o) = delete;

  contract(contract &&o)
      : is_signed(o.is_signed), __duration(o.__duration), __enforcing_fn(o.__enforcing_fn), __condition_fn(o.__condition_fn),
        __requirement(o.__requirement)
  {
    o.is_signed = false;
    o.__duration = 0;
    o.__enforcing_fn = nullptr;
    o.__condition_fn = nullptr;
    o.__requirement = nullptr;
  }

  contract &operator=(const contract &) = delete;

  contract &
  operator=(contract &&o)
  {
    is_signed = o.is_signed;
    __duration = o.__duration;
    __enforcing_fn = o.__enforcing_fn;
    __condition_fn = o.__condition_fn;
    __requirement = o.__requirement;
    o.is_signed = false;
    o.__duration = 0;
    o.__enforcing_fn = nullptr;
    o.__condition_fn = nullptr;
    o.__requirement = nullptr;
    return *this;
  }

  void
  sign(void)
  {
    if ( is_signed ) [[unlikely]]
      exc<except::future_error>("contract::sign(): was already signed");
    is_signed = true;
    go([this]() {
      if ( this->__requirement != nullptr ) {
        // a requirement has been set
        sleep_duration(this->__duration);
        if ( this->__requirement.get(memory_order_acquire) )
          return;
        // contract violation
      } else {
        return;
        // always good
      }
    });
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args...>)
  void
  sign(Fn &&fn, Args &&...args)
  {
    if ( is_signed ) [[unlikely]]
      exc<except::future_error>("contract::sign(): was already signed");
    if ( __enforcing_fn != nullptr )
      exc<except::future_error>("contract::sign(): already has a stored enforcing function");
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { f(a...); };
    __enforcing_fn = new fn_t<decltype(__fn)>(micron::move(__fn));
    is_signed = true;
    go([this]() {
      if ( this->__requirement != nullptr ) {
        // a requirement has been set
        sleep_duration(this->__duration);
        if ( this->__enforcing_fn->call() == this->__requirement.get(memory_order_acquire) )
          return;
        // contract violation
      } else {
        // no requirement, just enforce
        sleep_duration(this->__duration);
        if ( this->__enforcing_fn->call() )
          return;
        // contract violation
      }
    });
  }

  void
  duration(fduration_t tm)
  {
    __duration = tm;
  }

  // requirement that needs to be true in order to fire ensures
  template <typename... Args>
  void
  require(Args &&...args)
  {
    if ( __requirement != nullptr )
      exc<except::future_error>("contract::require(): requirement already stored");
    __requirement = new atomic_token<T>(micron::forward<Args &&>(args)...);
  }
};

};
