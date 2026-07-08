//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../tasks/coroutine/cl_sched.hpp"
#include "../../tasks/frame.hpp"
#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pluggable executor
//
// default backend is the continuation stealing work_stealing_executor

namespace micron
{
namespace coro
{

template<class E>
concept executor = requires(E &__e, __frame_base *__fb) {
  { __e.submit(__fb) } -> micron::same_as<void>;
  { __e.worker_count() } -> micron::convertible_to<u32>;
};

struct work_stealing_executor {
  using executor_tag = void;
  engine *__e = nullptr;

  work_stealing_executor() noexcept = default;

  explicit work_stealing_executor(engine *__eng) noexcept : __e(__eng) { }

  void
  submit(__frame_base *__fb) noexcept
  {
    if ( __e == nullptr ) [[unlikely]]
      __builtin_trap();
    __e->submit(__fb);
  }

  u32
  worker_count() const noexcept
  {
    return __e != nullptr ? __e->n : 0u;
  }
};

struct any_executor {
  void *__self = nullptr;
  void (*__submit_fn)(void *, __frame_base *) = nullptr;
  u32 (*__wc_fn)(void *) = nullptr;

  any_executor() noexcept = default;

  template<executor E>
  static any_executor
  make(E &__ex) noexcept
  {
    any_executor __a;
    __a.__self = &__ex;
    __a.__submit_fn = [](void *__s, __frame_base *__fb) { static_cast<E *>(__s)->submit(__fb); };
    __a.__wc_fn = [](void *__s) { return static_cast<E *>(__s)->worker_count(); };
    return __a;
  }

  void
  submit(__frame_base *__fb) noexcept
  {
    if ( __submit_fn == nullptr ) [[unlikely]]
      __builtin_trap();
    __submit_fn(__self, __fb);
  }

  u32
  worker_count() const noexcept
  {
    return __wc_fn != nullptr ? __wc_fn(__self) : 0u;
  }
};

inline work_stealing_executor
default_executor() noexcept
{
  start_coroutine_runtime();
  return work_stealing_executor{ __global_engine };
}

};      // namespace coro
};      // namespace micron
