//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../consts.hpp"
#include "../header.hpp"

#if !defined(__micron_arch_arm32)
#error "reloc_arm32.hpp included on a non-arm32 build"
#endif

namespace micron
{
namespace elf
{

inline constexpr word r_arm_none = 0;
inline constexpr word r_arm_pc24 = 1;
inline constexpr word r_arm_abs32 = 2;
inline constexpr word r_arm_rel32 = 3;
inline constexpr word r_arm_ldr_pc_g0 = 4;
inline constexpr word r_arm_abs16 = 5;
inline constexpr word r_arm_abs12 = 6;
inline constexpr word r_arm_thm_abs5 = 7;
inline constexpr word r_arm_abs8 = 8;
inline constexpr word r_arm_sbrel32 = 9;
inline constexpr word r_arm_thm_call = 10;
inline constexpr word r_arm_thm_pc8 = 11;
inline constexpr word r_arm_brel_adj = 12;
inline constexpr word r_arm_tls_desc = 13;
inline constexpr word r_arm_tls_dtpmod32 = 17;
inline constexpr word r_arm_tls_dtpoff32 = 18;
inline constexpr word r_arm_tls_tpoff32 = 19;
inline constexpr word r_arm_copy = 20;
inline constexpr word r_arm_glob_dat = 21;
inline constexpr word r_arm_jump_slot = 22;
inline constexpr word r_arm_relative = 23;
inline constexpr word r_arm_gotoff32 = 24;
inline constexpr word r_arm_base_prel = 25;
inline constexpr word r_arm_got_brel = 26;
inline constexpr word r_arm_plt32 = 27;
inline constexpr word r_arm_call = 28;
inline constexpr word r_arm_jump24 = 29;
inline constexpr word r_arm_thm_jump24 = 30;
inline constexpr word r_arm_base_abs = 31;
inline constexpr word r_arm_target1 = 38;
inline constexpr word r_arm_sbrel31 = 39;
inline constexpr word r_arm_v4bx = 40;
inline constexpr word r_arm_target2 = 41;
inline constexpr word r_arm_prel31 = 42;
inline constexpr word r_arm_movw_abs_nc = 43;
inline constexpr word r_arm_movt_abs = 44;
inline constexpr word r_arm_tls_gd32 = 104;
inline constexpr word r_arm_tls_ldm32 = 105;
inline constexpr word r_arm_tls_ldo32 = 106;
inline constexpr word r_arm_tls_ie32 = 107;
inline constexpr word r_arm_tls_le32 = 108;
inline constexpr word r_arm_irelative = 160;

// armv7's dynamic tables are DT_REL, same as i386
inline constexpr bool arch_uses_rel = true;

struct reloc_ctx_t;
struct reloc_view;

inline reloc_result apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept;

};      // namespace elf
};      // namespace micron
