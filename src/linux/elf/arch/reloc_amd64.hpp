//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../consts.hpp"
#include "../header.hpp"

#if !defined(__micron_arch_amd64)
#error "reloc_amd64.hpp included on a non-amd64 build"
#endif

namespace micron
{
namespace elf
{

inline constexpr word r_x86_64_none = 0;
inline constexpr word r_x86_64_64 = 1;
inline constexpr word r_x86_64_pc32 = 2;
inline constexpr word r_x86_64_got32 = 3;
inline constexpr word r_x86_64_plt32 = 4;
inline constexpr word r_x86_64_copy = 5;
inline constexpr word r_x86_64_glob_dat = 6;
inline constexpr word r_x86_64_jump_slot = 7;
inline constexpr word r_x86_64_relative = 8;
inline constexpr word r_x86_64_gotpcrel = 9;
inline constexpr word r_x86_64_32 = 10;
inline constexpr word r_x86_64_32s = 11;
inline constexpr word r_x86_64_16 = 12;
inline constexpr word r_x86_64_pc16 = 13;
inline constexpr word r_x86_64_8 = 14;
inline constexpr word r_x86_64_pc8 = 15;
inline constexpr word r_x86_64_dtpmod64 = 16;
inline constexpr word r_x86_64_dtpoff64 = 17;
inline constexpr word r_x86_64_tpoff64 = 18;
inline constexpr word r_x86_64_tlsgd = 19;
inline constexpr word r_x86_64_tlsld = 20;
inline constexpr word r_x86_64_dtpoff32 = 21;
inline constexpr word r_x86_64_gottpoff = 22;
inline constexpr word r_x86_64_tpoff32 = 23;
inline constexpr word r_x86_64_pc64 = 24;
inline constexpr word r_x86_64_size32 = 32;
inline constexpr word r_x86_64_size64 = 33;
inline constexpr word r_x86_64_irelative = 37;

struct reloc_ctx_t;

inline reloc_result apply_reloc(const reloc_ctx_t &ctx, const rela_t &r) noexcept;

};      // namespace elf
};      // namespace micron
