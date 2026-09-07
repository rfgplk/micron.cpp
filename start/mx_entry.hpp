//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <micron/types.hpp>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mx entry descriptor
//
// what __micron_mxc hands a custom entry instead of (argc, argv, envp)
//
// THIS LIVES IN THE CRT, NOT IN src/, AND THAT IS THE FIX FOR A REAL BREAKAGE. It used to be
// src/attach/mx_entry.hpp, reached from start.cpp as <micron/attach/mx_entry.hpp>. The Phase 4
// prune deleted src/attach/ -- and on a machine that has an installed snapshot at
// /usr/include/micron, that include did not fail. It RESOLVED, to the stale snapshot, which then
// pulled /usr/include/micron/types.hpp alongside the in-tree ./micron/types.hpp the rest of the TU
// was using. Two copies of every declaration in one translation unit:
//
//     ./micron/bits/__pause.hpp:12:1: error: redefinition of 'void __cpu_pause()'
//     ./micron/memory/../type_traits.hpp:26:34: error: redefinition of 'struct micron::integral_constant<T, V>'
//     ... 789 of them
//
// so `duck build <x> -k --start ./start -i . -i ./src` -- the invocation CLAUDE.md documents --
// stopped working, and stopped working in a way that names none of its own cause. Without the
// snapshot it would have been one honest "no such file".
//
// As a crt sibling it comes from --start <dir>, the way __auxv.hpp / __stack.hpp / __tls.hpp
// already do, so it is present exactly when start.cpp is and cannot be shadowed by an install. It
// belongs here on the merits too: this struct IS the crt's ABI with a custom entry, versioned by
// `size`, and the crt is what fills it in.
//
// The <micron/types.hpp> above is the crt convention and is not the same hazard: it resolves
// through the same -i as every other include in the TU, so there is one copy.

extern "C" {

inline constexpr u32 micron_mx_entry_abi = 1u;

inline constexpr u32 micron_mx_entry_f_have_stack = 1u << 0;      // stack_lo/hi are real
inline constexpr u32 micron_mx_entry_f_have_tls = 1u << 1;        // tls_base is real
inline constexpr u32 micron_mx_entry_f_have_user = 1u << 2;       // user/user_len are real
inline constexpr u32 micron_mx_entry_f_all
    = micron_mx_entry_f_have_stack | micron_mx_entry_f_have_tls | micron_mx_entry_f_have_user;

// 128 bytes, 8-aligned, no interior padding.
//
// alignas(8) is REQUIRED, not decorative. The i386 SysV ABI aligns u64 to FOUR, so on a 32-bit x86
// build the natural alignment of this struct is 4 and the static_assert below fires:
//
//     start/mx_entry.hpp:68: error: static assertion failed: micron_mx_entry_args must be 8-aligned
//
// which is what happens the first time anything compiles the crt for --i386. Every offset and the
// total size are unaffected -- the u64 members already land on multiples of 8 -- so this only pins
// the alignment the descriptor's consumers on the other side of the ABI are entitled to assume.
struct alignas(8) micron_mx_entry_args {
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
