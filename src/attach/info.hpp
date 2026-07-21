//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "../version.hpp"

namespace micron
{
struct __tls_frame;
}

extern "C" {

inline constexpr u32 micron_attach_magic = 0x4D435441u;      // 'MCTA'
inline constexpr u16 micron_attach_version = 1u;

// info.flags
inline constexpr u32 attach_f_io_buffers = 1u << 0;      // guest may boot its own stdio buffers
inline constexpr u32 attach_f_have_stack = 1u << 1;      // host_stack_lo/hi are valid

// _attach return codes
inline constexpr int attach_ok = 0;
inline constexpr int attach_ebadinfo = -1;      // magic/version/abi/size/null fn ptr
inline constexpr int attach_enospace = -2;      // (reserved; surplus exhaustion is reported by the landlord at assign)
inline constexpr int attach_ealready = -3;      // already attached in this image

struct micron_attach_info {
  u32 magic;        // == micron_attach_magc
  u16 version;      // == micron_attach_version
  u16 abi;          // == MICRON_ABI
  u32 size;         // == sizeof(micron_attach_info) as the HOST compiled it
  u32 flags;        // attach_f_*

  // [surplus (guests tdata) | host image | TCB]
  // canary seeded from the live TCB; does not install the TP
  int (*make_child_frame)(micron::__tls_frame *out) noexcept;
  void (*free_frame)(const micron::__tls_frame *f) noexcept;

  // called instead of SYS_exit_group on any guest fatal path
  void (*fatal)(int code) noexcept;

  int (*thread_atexit)(void (*dtor)(void *), void *obj) noexcept;
  void (*run_thread_dtors)(void) noexcept;

  u64 surplus_size;          // host compile-time surplus S (guest self-check)
  u64 host_image_memsz;      // host PT_TLS memsz (diagnostics)
  u64 host_stack_lo;         // valid iff attach_f_have_stack
  u64 host_stack_hi;
  char **host_environ;      // may be null
};

}      // extern "C"

namespace micron
{
inline const micron_attach_info *__micron_attach_info = nullptr;

inline __attribute__((always_inline)) bool
__attach_info_valid(const micron_attach_info *i) noexcept
{
  if ( i == nullptr ) return false;
  if ( i->magic != micron_attach_magic ) return false;
  if ( i->version != micron_attach_version ) return false;
  if ( i->abi != static_cast<u32>(MICRON_ABI) ) return false;
  // must cover every field the guest reads
  if ( i->size < __builtin_offsetof(micron_attach_info, host_image_memsz) ) return false;
  if ( i->make_child_frame == nullptr || i->free_frame == nullptr || i->fatal == nullptr ) return false;
  if ( i->thread_atexit == nullptr || i->run_thread_dtors == nullptr ) return false;
  if ( i->surplus_size == 0 ) return false;
  return true;
}

};      // namespace micron
