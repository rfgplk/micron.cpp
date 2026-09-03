//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// continuation run argument
//
// WARNING: this is a mirror of mx::mx_cont_args (mxf src/format/cont.hh); make sure it's in sync

extern "C" {

inline constexpr u32 micron_cont_args_abi = 2u;

inline constexpr u32 micron_cont_args_flag_graph = 1u << 0;        // ops names a real table
inline constexpr u32 micron_cont_args_flag_replace = 1u << 1;      // this call does not return
inline constexpr u32 micron_cont_args_flag_all = micron_cont_args_flag_graph | micron_cont_args_flag_replace;

// _continue return codes
inline constexpr i64 cont_ok = 0;
inline constexpr i64 cont_efault = -1;            // null args
inline constexpr i64 cont_ebadargs = -2;          // abi/flags/span
inline constexpr i64 cont_enotattached = -3;      // the direct form was entered before _attach

// 64 bytes, 8-aligned, no interior padding
struct micron_cont_args {
  u32 abi;            // 0    == micron_cont_args_abi
  u32 flags;          // 4    micron_cont_args_flag_*
  u64 self_base;      // 8    where this continuation was placed
  u64 self_span;      // 16
  u64 host_base;      // 24   the image that called mx_continue()
  u64 host_span;      // 32
  u64 ops;            // 40   a const mx::mx_graph_ops *; valid iff micron_cont_args_flag_graph
  u64 user;           // 48   the host's own argument, opaque to micron
  u64 user_len;       // 56
};      // 64

static_assert(sizeof(micron_cont_args) == 64, "micron_cont_args must be 64 bytes");
static_assert(alignof(micron_cont_args) == 8, "micron_cont_args must be 8-aligned");
static_assert(__builtin_offsetof(micron_cont_args, abi) == 0);
static_assert(__builtin_offsetof(micron_cont_args, flags) == 4);
static_assert(__builtin_offsetof(micron_cont_args, self_base) == 8);
static_assert(__builtin_offsetof(micron_cont_args, self_span) == 16);
static_assert(__builtin_offsetof(micron_cont_args, host_base) == 24);
static_assert(__builtin_offsetof(micron_cont_args, host_span) == 32);
static_assert(__builtin_offsetof(micron_cont_args, ops) == 40);
static_assert(__builtin_offsetof(micron_cont_args, user) == 48);
static_assert(__builtin_offsetof(micron_cont_args, user_len) == 56);

}      // extern "C"

namespace micron
{

inline const micron_cont_args *__micron_cont_args = nullptr;

inline __attribute__((always_inline)) bool
__cont_args_valid(const micron_cont_args *a) noexcept
{
  if ( a == nullptr ) return false;
  if ( a->abi != micron_cont_args_abi ) return false;
  if ( (a->flags & ~micron_cont_args_flag_all) != 0 ) return false;
  if ( (a->flags & micron_cont_args_flag_graph) != 0 && a->ops == 0 ) return false;
  if ( a->self_span == 0 || a->host_span == 0 ) return false;
  return true;
}

};      // namespace micron
