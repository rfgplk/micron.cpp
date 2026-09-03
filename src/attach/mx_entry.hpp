//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mx entry descriptor
//
// what __micron_mxc hands a custom entry instead of (argc, argv, envp)

extern "C" {

inline constexpr u32 micron_mx_entry_abi = 1u;

inline constexpr u32 micron_mx_entry_f_have_stack = 1u << 0;      // stack_lo/hi are real
inline constexpr u32 micron_mx_entry_f_have_tls = 1u << 1;        // tls_base is real
inline constexpr u32 micron_mx_entry_f_have_user = 1u << 2;       // user/user_len are real
inline constexpr u32 micron_mx_entry_f_all
    = micron_mx_entry_f_have_stack | micron_mx_entry_f_have_tls | micron_mx_entry_f_have_user;

// 128 bytes, 8-aligned, no interior padding
struct micron_mx_entry_args {
  u32 abi;             // 0    == micron_mx_entry_abi
  u32 size;            // 4    == sizeof(micron_mx_entry_args) as the CRT compiled it
  u32 flags;           // 8    micron_mx_entry_f_*
  u32 argc;            // 12
  u64 argv;            // 16   char **
  u64 envp;            // 24   char **
  u64 auxv;            // 32   const micron::auxv_t *
  u64 self_base;       // 40   where the image was mapped (at_base); 0 when the loader gave none
  u64 page_size;       // 48   at_pagesz
  u64 stack_lo;        // 56   iff micron_mx_entry_f_have_stack
  u64 stack_hi;        // 64
  u64 tls_base;        // 72   the seated thread pointer; iff micron_mx_entry_f_have_tls
  u64 user;            // 80   loader-opaque; iff micron_mx_entry_f_have_user
  u64 user_len;        // 88
  u64 reserved[4];     // 96   must be zero
};      // 128

static_assert(sizeof(micron_mx_entry_args) == 128, "micron_mx_entry_args must be 128 bytes");
static_assert(alignof(micron_mx_entry_args) == 8, "micron_mx_entry_args must be 8-aligned");
static_assert(__builtin_offsetof(micron_mx_entry_args, abi) == 0);
static_assert(__builtin_offsetof(micron_mx_entry_args, size) == 4);
static_assert(__builtin_offsetof(micron_mx_entry_args, flags) == 8);
static_assert(__builtin_offsetof(micron_mx_entry_args, argc) == 12);
static_assert(__builtin_offsetof(micron_mx_entry_args, argv) == 16);
static_assert(__builtin_offsetof(micron_mx_entry_args, envp) == 24);
static_assert(__builtin_offsetof(micron_mx_entry_args, auxv) == 32);
static_assert(__builtin_offsetof(micron_mx_entry_args, self_base) == 40);
static_assert(__builtin_offsetof(micron_mx_entry_args, page_size) == 48);
static_assert(__builtin_offsetof(micron_mx_entry_args, stack_lo) == 56);
static_assert(__builtin_offsetof(micron_mx_entry_args, stack_hi) == 64);
static_assert(__builtin_offsetof(micron_mx_entry_args, tls_base) == 72);
static_assert(__builtin_offsetof(micron_mx_entry_args, user) == 80);
static_assert(__builtin_offsetof(micron_mx_entry_args, user_len) == 88);
static_assert(__builtin_offsetof(micron_mx_entry_args, reserved) == 96);

}      // extern "C"

namespace micron
{

inline __attribute__((always_inline)) bool
__mx_entry_args_valid(const micron_mx_entry_args *a) noexcept
{
  if ( a == nullptr ) return false;
  if ( a->abi != micron_mx_entry_abi ) return false;
  if ( a->size < sizeof(micron_mx_entry_args) ) return false;
  if ( (a->flags & ~micron_mx_entry_f_all) != 0 ) return false;
  if ( a->argv == 0 || a->envp == 0 || a->auxv == 0 ) return false;
  return true;
}

inline __attribute__((always_inline)) bool
__mx_entry_has(const micron_mx_entry_args *a, u32 end_off) noexcept
{
  return a != nullptr && a->size >= end_off;
}

};      // namespace micron
