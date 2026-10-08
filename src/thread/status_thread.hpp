//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "thread_types/thread_impl.hpp"

namespace micron
{
// ^^^^^^^^^^^^^^^^^^ bounded freestanding callback thread, no allocated launch payload.
// The callback and context remain valid until join. One owner starts/joins; the
// address-bound control block cannot move. Cancellation belongs to the callback.
template<usize Stack_Size = auto_thread_stack_size, usize TLS_Limit = 1024 * 1024> class status_thread
{
  static_assert(Stack_Size >= 4 * __micron_page_size_default && Stack_Size % __micron_page_size_default == 0);
  static_assert(TLS_Limit >= __micron_page_size_default && TLS_Limit <= 16 * 1024 * 1024);
  addr_t *__stack = nullptr;
  __tls_frame __tls{};
  int __ctid = 0;
  pid_t __tid = 0;
  __thread_payload __payload{};
  void (*__callback)(void *) = nullptr;
  void *__context = nullptr;

  static int
  __entry(void *opaque)
  {
    auto &self = *static_cast<status_thread *>(opaque);
    return __thread_kernel(&self.__payload, self.__callback, self.__context);
  }

  void
  __release() noexcept
  {
    micron::try_unmap(__stack, Stack_Size); __stack = nullptr;
    micron::__tls_release_frame(__tls); __tls = {};
    __ctid = 0; __tid = 0; __callback = nullptr; __context = nullptr;
    __payload.clear();
  }

public:
  status_thread() = default;
  ~status_thread() { (void)join(); }
  status_thread(const status_thread &) = delete;
  status_thread &operator=(const status_thread &) = delete;
  status_thread(status_thread &&) = delete;
  status_thread &operator=(status_thread &&) = delete;

  static constexpr usize reservation_bytes = Stack_Size + TLS_Limit;

  // Zero succeeds; failures are negative errno and leave an empty, reusable owner.
  [[nodiscard]] int
  start(void (*callback)(void *), void *context) noexcept
  {
    if ( __tid || __stack ) return -error::busy;
    if ( !callback ) return -error::invalid_arg;
#if defined(__micron_freestanding)
#if defined(__micron_attach_capable)
    if ( __micron_attach_info != nullptr ) return -error::op_not_supported;
#endif
    if ( !__tls_ensure_template() ) return -error::op_not_supported;
    const auto &t = __micron_tls_template;
    if ( !t.pagesz || t.pagesz > Stack_Size / 4 || Stack_Size % t.pagesz ) return -error::invalid_arg;
    // The frame adds at most two pages for its TCB, alignment and rounding.
    if ( t.pagesz > TLS_Limit / 2 || t.memsz > TLS_Limit - 2 * t.pagesz ) return -error::out_of_memory;
    __stack = micron::addrmap(Stack_Size);
    if ( !__stack || micron::mmap_failed(__stack) ) { __stack = nullptr; return -error::out_of_memory; }
    const int guarded = micron::mprotect(__stack, t.pagesz, micron::prot_none);
    if ( guarded < 0 ) { __release(); return guarded; }
    __tls = micron::__tls_make_child_frame();
    if ( !__tls.base || __tls.size > TLS_Limit ) { __release(); return -error::out_of_memory; }
    __callback = callback; __context = context;
    __payload.alive.store(true, memory_order_release);
    const auto id = posix::clone3_spawn(__entry, this, reinterpret_cast<byte *>(__stack) + t.pagesz,
        Stack_Size - t.pagesz, reinterpret_cast<unsigned long>(__tls.tp), nullptr, &__ctid);
    if ( id < 0 ) { __release(); return id; }
    __tid = id; return 0;
#else
    (void)context;
    return -error::op_not_supported;
#endif
  }

  [[nodiscard]] int
  try_join() noexcept
  {
    if ( !__tid ) return 0;
    if ( micthread::try_join(&__ctid) != 0 ) return -error::busy;
    __release(); return 0;
  }

  int
  join() noexcept
  {
    if ( __tid ) { micthread::join(&__ctid); __release(); }
    return 0;
  }

  pid_t id() const noexcept { return __tid; }
  usize mapped_bytes() const noexcept { return __stack ? Stack_Size + __tls.size : 0; }
};
}
