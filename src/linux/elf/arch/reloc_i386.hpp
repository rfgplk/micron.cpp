//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../consts.hpp"
#include "../header.hpp"

#if !defined(__micron_arch_x86)
#error "reloc_i386.hpp included on a non-i386 build"
#endif

namespace micron
{
namespace elf
{

inline constexpr word r_386_none = 0;
inline constexpr word r_386_32 = 1;
inline constexpr word r_386_pc32 = 2;
inline constexpr word r_386_got32 = 3;
inline constexpr word r_386_plt32 = 4;
inline constexpr word r_386_copy = 5;
inline constexpr word r_386_glob_dat = 6;
inline constexpr word r_386_jmp_slot = 7;
inline constexpr word r_386_relative = 8;
inline constexpr word r_386_gotoff = 9;
inline constexpr word r_386_gotpc = 10;
inline constexpr word r_386_32plt = 11;
inline constexpr word r_386_tls_tpoff = 14;
inline constexpr word r_386_tls_ie = 15;
inline constexpr word r_386_tls_gotie = 16;
inline constexpr word r_386_tls_le = 17;
inline constexpr word r_386_tls_gd = 18;
inline constexpr word r_386_tls_ldm = 19;
inline constexpr word r_386_16 = 20;
inline constexpr word r_386_pc16 = 21;
inline constexpr word r_386_8 = 22;
inline constexpr word r_386_pc8 = 23;
inline constexpr word r_386_tls_gd_32 = 24;
inline constexpr word r_386_tls_gd_push = 25;
inline constexpr word r_386_tls_gd_call = 26;
inline constexpr word r_386_tls_gd_pop = 27;
inline constexpr word r_386_tls_ldm_32 = 28;
inline constexpr word r_386_tls_ldm_push = 29;
inline constexpr word r_386_tls_ldm_call = 30;
inline constexpr word r_386_tls_ldm_pop = 31;
inline constexpr word r_386_tls_ldo_32 = 32;
inline constexpr word r_386_tls_ie_32 = 33;
inline constexpr word r_386_tls_le_32 = 34;
inline constexpr word r_386_tls_dtpmod32 = 35;
inline constexpr word r_386_tls_dtpoff32 = 36;
inline constexpr word r_386_tls_tpoff32 = 37;
inline constexpr word r_386_size32 = 38;
inline constexpr word r_386_tls_gotdesc = 39;
inline constexpr word r_386_tls_desc_call = 40;
inline constexpr word r_386_tls_desc = 41;
inline constexpr word r_386_irelative = 42;
inline constexpr word r_386_got32x = 43;

// i386's dynamic tables are DT_REL
inline constexpr bool arch_uses_rel = true;

struct reloc_ctx_t;
struct reloc_view;

inline reloc_result apply_reloc(const reloc_ctx_t &ctx, const reloc_view &r) noexcept;

};      // namespace elf
};      // namespace micron
