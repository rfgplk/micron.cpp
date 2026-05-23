//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../consts.hpp"
#include "../header.hpp"

#if !defined(__micron_arch_arm64)
#error "reloc_arm64.hpp included on a non-arm64 build"
#endif

namespace micron
{
namespace elf
{

inline constexpr word r_aarch64_none = 0;
inline constexpr word r_aarch64_abs64 = 257;
inline constexpr word r_aarch64_abs32 = 258;
inline constexpr word r_aarch64_abs16 = 259;
inline constexpr word r_aarch64_prel64 = 260;
inline constexpr word r_aarch64_prel32 = 261;
inline constexpr word r_aarch64_prel16 = 262;
inline constexpr word r_aarch64_copy = 1024;
inline constexpr word r_aarch64_glob_dat = 1025;
inline constexpr word r_aarch64_jump_slot = 1026;
inline constexpr word r_aarch64_relative = 1027;
inline constexpr word r_aarch64_tls_dtpmod = 1028;
inline constexpr word r_aarch64_tls_dtprel = 1029;
inline constexpr word r_aarch64_tls_tprel = 1030;
inline constexpr word r_aarch64_tlsdesc = 1031;
inline constexpr word r_aarch64_irelative = 1032;

struct reloc_ctx_t;

inline reloc_result apply_reloc(const reloc_ctx_t &ctx, const rela_t &r) noexcept;

};      // namespace elf
};      // namespace micron
