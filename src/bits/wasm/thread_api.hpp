//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../memory/new.hpp"
#include "../../memory/ptrs.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../tags.hpp"
#include "../../tuple.hpp"
#include "threads.hpp"

namespace micron
{
namespace wasm
{
struct thread_packet {
  unsigned long long value = 0;
};

template<typename F, typename... A> struct invocation_packet: thread_packet {
  F function;
  tuple<A...> arguments;

  template<typename Fn, typename... Args>
  invocation_packet(Fn &&f, Args &&...a) : function(micron::forward<Fn>(f)), arguments(micron::forward<Args>(a)...)
  {
  }

  static int
  run(void *p)
  {
    auto &self = *static_cast<invocation_packet *>(p);
    if constexpr ( is_void_v<decltype(micron::apply(self.function, self.arguments))> ) {
      micron::apply(self.function, self.arguments);
    } else {
      auto value = micron::apply(self.function, self.arguments);
      if constexpr ( is_trivially_copyable_v<decltype(value)> && sizeof(value) <= sizeof(self.value) )
        __builtin_memcpy(&self.value, &value, sizeof(value));
    }
    return 0;
  }

  static void
  destroy(thread_packet *p)
  {
    delete static_cast<invocation_packet *>(p);
  }
};
}      // namespace wasm

template<usize Stack_Size = thread_stack_size> class thread
{
  wasm::thread_handle __handle{};
  wasm::thread_packet *__packet = nullptr;
  void (*__destroy)(wasm::thread_packet *) = nullptr;
  unsigned long long __result = 0;
  int __status = 0;

  template<typename Fn, typename... Args>
  void
  launch(Fn &&fn, Args &&...args)
  {
    static_assert(Stack_Size <= MICRON_WASM_STACK_SIZE, "thread stack exceeds MICRON_WASM_STACK_SIZE");
    using packet = wasm::invocation_packet<decay_t<Fn>, decay_t<Args>...>;
    auto *p = new packet(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
    const int r = wasm::spawn(packet::run, p, __handle);
    if ( r ) {
      delete p;
      exc<except::thread_error>("micron: browser worker pool is unavailable or full");
    }
    __packet = p;
    __destroy = packet::destroy;
  }

public:
  using thread_type = thread_tag;
  thread() = default;

  template<typename Fn, typename... Args>
    requires(is_invocable_v<decay_t<Fn>, decay_t<Args> &...>)
  explicit thread(Fn &&fn, Args &&...args)
  {
    launch(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  thread(const thread &) = delete;
  thread &operator=(const thread &) = delete;

  thread(thread &&o) noexcept
      : __handle(o.__handle), __packet(o.__packet), __destroy(o.__destroy), __result(o.__result), __status(o.__status)
  {
    o.__handle = {};
    o.__packet = nullptr;
  }

  thread &
  operator=(thread &&o) noexcept
  {
    if ( this == &o ) return *this;
    join();
    __handle = o.__handle;
    __packet = o.__packet;
    __destroy = o.__destroy;
    __result = o.__result;
    __status = o.__status;
    o.__handle = {};
    o.__packet = nullptr;
    return *this;
  }

  ~thread()
  {
    if ( join() < 0 && __handle.slot ) wasm::panic();
  }

  bool
  alive() const noexcept
  {
    if ( !__handle.slot ) return false;
    const auto state = __atomic_load_n(&wasm::__threads[__handle.slot].state, __ATOMIC_ACQUIRE);
    return state == 1 || state == 2;
  }

  bool
  active() const noexcept
  {
    return alive();
  }

  int
  can_join() const noexcept
  {
    return __handle.slot ? (alive() ? 16 : 1) : 0;
  }

  void
  wait_for() const noexcept
  {
    if ( !__handle.slot ) return;
    auto &slot = wasm::__threads[__handle.slot];
    unsigned state;
    while ( (state = __atomic_load_n(&slot.state, __ATOMIC_ACQUIRE)) == 1 || state == 2 )
      if ( wasm::wait32(&slot.state, state) == -95 ) wasm::panic();
  }

  int
  join() noexcept
  {
    if ( !__handle.slot ) return __status;
    __status = wasm::join(__handle);
    if ( __handle.slot ) return __status;
    if ( __packet ) {
      __result = __packet->value;
      __destroy(__packet);
      __packet = nullptr;
    }
    return __status;
  }

  int
  dismiss() noexcept
  {
    return join();
  }

  int
  try_join() noexcept
  {
    return alive() ? 16 : join();
  }

  unsigned
  thread_id() const noexcept
  {
    return __handle.slot ? __handle.slot + 1 : 0;
  }

  wasm::thread_handle
  native_handle() const noexcept
  {
    return __handle;
  }

  int
  cancel() noexcept
  {
    if ( !__handle.slot ) return -22;
    if ( __handle.slot + 1 == wasm::thread_id() ) return -35;
    const int r = wasm::request_cancel(__handle);
    return r < 0 ? r : join();
  }

  int
  sleep() noexcept
  {
    return wasm::__set_park(__handle, true);
  }

  int
  sleep_second() noexcept
  {
    return sleep();
  }

  int
  awaken() noexcept
  {
    return wasm::__set_park(__handle, false);
  }

  template<typename R>
    requires(is_trivially_copyable_v<R> && sizeof(R) <= sizeof(void *))
  R
  result()
  {
    wait_for();
    const auto value = __packet ? __packet->value : __result;
    R r{};
    __builtin_memcpy(&r, &value, sizeof(R));
    return r;
  }

  template<typename Fn, typename... Args>
  thread &
  operator[](Fn &&fn, Args &&...args)
  {
    join();
    launch(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
    return *this;
  }

  addr_t *
  stack() noexcept
  {
    return __handle.slot ? reinterpret_cast<addr_t *>(wasm::__threads[__handle.slot].stack) : nullptr;
  }

  constexpr usize
  stack_size() const noexcept
  {
    return MICRON_WASM_STACK_SIZE;
  }

  int terminate() = delete;
  int kill() = delete;
};

template<usize N = auto_thread_stack_size> using auto_thread = thread<N>;

inline bool
threads_available() noexcept
{
#if defined(__micron_wasm_threads)
  return wasm::can_wait();
#else
  return false;
#endif
}

template<typename Tr> using __thread_pointer = unique_pointer<Tr>;

namespace solo
{
template<typename Tr = auto_thread<>, typename Fn, typename... Args>
auto
spawn(Fn &&fn, Args &&...args) -> __thread_pointer<Tr>
{
  return __thread_pointer<Tr>(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
}

template<typename Tr>
int
join(Tr &t)
{
  return t.join();
}

template<typename Tr>
int
join(__thread_pointer<Tr> &t)
{
  if ( !t ) return -22;
  int r = t->join();
  t.clear();
  return r;
}

template<typename Tr>
int
dismiss(Tr &t)
{
  return join(t);
}

template<typename Tr>
int
try_join(Tr &t)
{
  return t.try_join();
}

template<typename Tr>
bool
is_joinable(Tr &t)
{
  return t.can_join();
}

template<typename Tr>
void
wait_for(Tr &t)
{
  t.wait_for();
}

template<typename Tr>
int
cancel(Tr &t)
{
  return t.cancel();
}

template<typename Tr>
int
sleep(Tr &t)
{
  return t.sleep();
}

template<typename Tr>
int
awaken(Tr &t)
{
  return t.awaken();
}
}      // namespace solo
}      // namespace micron
