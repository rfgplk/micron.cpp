//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

namespace micron
{
namespace elf
{

inline constexpr half et_none = 0;
inline constexpr half et_rel = 1;
inline constexpr half et_exec = 2;
inline constexpr half et_dyn = 3;
inline constexpr half et_core = 4;

// TODO: eventually expand this when we add more arches
inline constexpr half em_x86_64 = 62;
inline constexpr half em_aarch64 = 183;

inline constexpr word ev_current = 1;

inline constexpr u8 elfclass64 = 2;
inline constexpr u8 elfdata2lsb = 1;

inline constexpr word pt_null = 0;
inline constexpr word pt_load = 1;
inline constexpr word pt_dynamic = 2;
inline constexpr word pt_interp = 3;
inline constexpr word pt_note = 4;
inline constexpr word pt_shlib = 5;
inline constexpr word pt_phdr = 6;
inline constexpr word pt_tls = 7;
inline constexpr word pt_gnu_eh_frame = 0x6474e550;
inline constexpr word pt_gnu_stack = 0x6474e551;
inline constexpr word pt_gnu_relro = 0x6474e552;
inline constexpr word pt_gnu_property = 0x6474e553;

inline constexpr word pf_x = 0x1;
inline constexpr word pf_w = 0x2;
inline constexpr word pf_r = 0x4;

inline constexpr sxword dt_null = 0;
inline constexpr sxword dt_needed = 1;
inline constexpr sxword dt_pltrelsz = 2;
inline constexpr sxword dt_pltgot = 3;
inline constexpr sxword dt_hash = 4;
inline constexpr sxword dt_strtab = 5;
inline constexpr sxword dt_symtab = 6;
inline constexpr sxword dt_rela = 7;
inline constexpr sxword dt_relasz = 8;
inline constexpr sxword dt_relaent = 9;
inline constexpr sxword dt_strsz = 10;
inline constexpr sxword dt_syment = 11;
inline constexpr sxword dt_init = 12;
inline constexpr sxword dt_fini = 13;
inline constexpr sxword dt_soname = 14;
inline constexpr sxword dt_rpath = 15;
inline constexpr sxword dt_symbolic = 16;
inline constexpr sxword dt_rel = 17;
inline constexpr sxword dt_relsz = 18;
inline constexpr sxword dt_relent = 19;
inline constexpr sxword dt_pltrel = 20;
inline constexpr sxword dt_debug = 21;
inline constexpr sxword dt_textrel = 22;
inline constexpr sxword dt_jmprel = 23;
inline constexpr sxword dt_bind_now = 24;
inline constexpr sxword dt_init_array = 25;
inline constexpr sxword dt_fini_array = 26;
inline constexpr sxword dt_init_arraysz = 27;
inline constexpr sxword dt_fini_arraysz = 28;
inline constexpr sxword dt_runpath = 29;
inline constexpr sxword dt_flags = 30;
inline constexpr sxword dt_preinit_array = 32;
inline constexpr sxword dt_preinit_arraysz = 33;

inline constexpr sxword dt_gnu_hash = 0x6ffffef5;
inline constexpr sxword dt_relacount = 0x6ffffff9;
inline constexpr sxword dt_relcount = 0x6ffffffa;
inline constexpr sxword dt_flags_1 = 0x6ffffffb;
inline constexpr sxword dt_verdef = 0x6ffffffc;
inline constexpr sxword dt_verdefnum = 0x6ffffffd;
inline constexpr sxword dt_verneed = 0x6ffffffe;
inline constexpr sxword dt_verneednum = 0x6fffffff;
inline constexpr sxword dt_versym = 0x6ffffff0;

inline constexpr u8 stb_local = 0;
inline constexpr u8 stb_global = 1;
inline constexpr u8 stb_weak = 2;
inline constexpr u8 stb_gnu_unique = 10;

inline constexpr u8 stt_notype = 0;
inline constexpr u8 stt_object = 1;
inline constexpr u8 stt_func = 2;
inline constexpr u8 stt_section = 3;
inline constexpr u8 stt_file = 4;
inline constexpr u8 stt_common = 5;
inline constexpr u8 stt_tls = 6;
inline constexpr u8 stt_gnu_ifunc = 10;

inline constexpr half shn_undef = 0;
inline constexpr half shn_abs = 0xfff1;
inline constexpr half shn_common = 0xfff2;

inline constexpr u8
elf_st_bind(u8 info) noexcept
{
  return info >> 4;
}

inline constexpr u8
elf_st_type(u8 info) noexcept
{
  return info & 0x0f;
}

inline constexpr u32
elf_r_sym(xword info) noexcept
{
  return static_cast<u32>(info >> 32);
}

inline constexpr u32
elf_r_type(xword info) noexcept
{
  return static_cast<u32>(info & 0xffffffff);
}

};      // namespace elf
};      // namespace micron
