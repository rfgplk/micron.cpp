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

inline constexpr half em_386 = 3;
inline constexpr half em_mips = 8;
inline constexpr half em_ppc = 20;
inline constexpr half em_ppc64 = 21;
inline constexpr half em_s390 = 22;
inline constexpr half em_arm = 40;
inline constexpr half em_sparcv9 = 43;
inline constexpr half em_ia_64 = 50;
inline constexpr half em_riscv = 243;
inline constexpr half em_loongarch = 258;

inline constexpr word ev_current = 1;

inline constexpr u8 elfclass32 = 1;
inline constexpr u8 elfclass64 = 2;
inline constexpr u8 elfdata2lsb = 1;
inline constexpr u8 elfdata2msb = 2;

// e_ident[EI_OSABI]
inline constexpr u8 elfosabi_sysv = 0;
inline constexpr u8 elfosabi_hpux = 1;
inline constexpr u8 elfosabi_netbsd = 2;
inline constexpr u8 elfosabi_gnu = 3;
inline constexpr u8 elfosabi_solaris = 6;
inline constexpr u8 elfosabi_aix = 7;
inline constexpr u8 elfosabi_irix = 8;
inline constexpr u8 elfosabi_freebsd = 9;
inline constexpr u8 elfosabi_openbsd = 12;

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

inline constexpr word pt_arm_exidx = 0x70000001;      // armv7 unwind index; == pt_loproc + 1

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

inline constexpr sxword dt_loos = 0x6000000d;
inline constexpr sxword dt_hios = 0x6ffff000;
inline constexpr sxword dt_loproc = 0x70000000;
inline constexpr sxword dt_hiproc = 0x7fffffff;

// armv7 puts the exception index table's address and count in the processor range
inline constexpr sxword dt_arm_exidx = 0x70000001;
inline constexpr sxword dt_arm_exidxsz = 0x70000002;

inline constexpr sxword dt_gnu_hash = 0x6ffffef5;
inline constexpr sxword dt_relacount = 0x6ffffff9;
inline constexpr sxword dt_relcount = 0x6ffffffa;
inline constexpr sxword dt_relrsz = 35;      // packed relative relocations (RELR)
inline constexpr sxword dt_relr = 36;
inline constexpr sxword dt_relrent = 37;
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

inline constexpr word stn_undef = 0;      // reserved .dynsym[0], and r_info's "no symbol"

// st_other & 0x3
inline constexpr u8 stv_default = 0;
inline constexpr u8 stv_internal = 1;
inline constexpr u8 stv_hidden = 2;
inline constexpr u8 stv_protected = 3;

inline constexpr half shn_undef = 0;
inline constexpr half shn_abs = 0xfff1;
inline constexpr half shn_common = 0xfff2;
inline constexpr half shn_xindex = 0xffff;

// sh_type
inline constexpr word sht_null = 0;
inline constexpr word sht_progbits = 1;
inline constexpr word sht_symtab = 2;
inline constexpr word sht_strtab = 3;
inline constexpr word sht_rela = 4;
inline constexpr word sht_hash = 5;
inline constexpr word sht_dynamic = 6;
inline constexpr word sht_note = 7;
inline constexpr word sht_nobits = 8;
inline constexpr word sht_rel = 9;
inline constexpr word sht_shlib = 10;
inline constexpr word sht_dynsym = 11;
inline constexpr word sht_init_array = 14;
inline constexpr word sht_fini_array = 15;
inline constexpr word sht_preinit_array = 16;
inline constexpr word sht_group = 17;
inline constexpr word sht_symtab_shndx = 18;
inline constexpr word sht_relr = 19;
inline constexpr word sht_gnu_attributes = 0x6ffffff5;
inline constexpr word sht_gnu_hash = 0x6ffffff6;
inline constexpr word sht_gnu_liblist = 0x6ffffff7;
inline constexpr word sht_gnu_verdef = 0x6ffffffd;
inline constexpr word sht_gnu_verneed = 0x6ffffffe;
inline constexpr word sht_gnu_versym = 0x6fffffff;

// sh_flags
inline constexpr xword shf_write = 0x1;
inline constexpr xword shf_alloc = 0x2;
inline constexpr xword shf_execinstr = 0x4;
inline constexpr xword shf_merge = 0x10;
inline constexpr xword shf_strings = 0x20;
inline constexpr xword shf_info_link = 0x40;
inline constexpr xword shf_link_order = 0x80;
inline constexpr xword shf_os_nonconforming = 0x100;
inline constexpr xword shf_group = 0x200;
inline constexpr xword shf_tls = 0x400;
inline constexpr xword shf_compressed = 0x800;

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

inline constexpr u8
elf_st_visibility(u8 other) noexcept
{
  return other & 0x03;
}

// ELF32 is (sym << 8 | type) in a word
// ELF64 is (sym << 32 | type) in an xword
inline constexpr u32
elf32_r_sym(word info) noexcept
{
  return info >> 8;
}

inline constexpr u32
elf32_r_type(word info) noexcept
{
  return info & 0xffu;
}

inline constexpr word
elf32_r_info(u32 sym, u32 type) noexcept
{
  return static_cast<word>((sym << 8) | (type & 0xffu));
}

inline constexpr u32
elf64_r_sym(xword info) noexcept
{
  return static_cast<u32>(info >> 32);
}

inline constexpr u32
elf64_r_type(xword info) noexcept
{
  return static_cast<u32>(info & 0xffffffff);
}

inline constexpr xword
elf64_r_info(u32 sym, u32 type) noexcept
{
  return (static_cast<xword>(sym) << 32) | static_cast<xword>(type);
}

inline constexpr u32
elf_r_sym(xword info) noexcept
{
  if constexpr ( native_class == fmt_class::elf32 )
    return elf32_r_sym(static_cast<word>(info));
  else
    return elf64_r_sym(info);
}

inline constexpr u32
elf_r_type(xword info) noexcept
{
  if constexpr ( native_class == fmt_class::elf32 )
    return elf32_r_type(static_cast<word>(info));
  else
    return elf64_r_type(info);
}

// DT_FLAGS / DT_FLAGS_1
inline constexpr xword df_origin = 0x01;
inline constexpr xword df_symbolic = 0x02;
inline constexpr xword df_textrel = 0x04;
inline constexpr xword df_bind_now = 0x08;
inline constexpr xword df_static_tls = 0x10;

inline constexpr xword df_1_now = 0x00000001;
inline constexpr xword df_1_global = 0x00000002;
inline constexpr xword df_1_group = 0x00000004;
inline constexpr xword df_1_nodelete = 0x00000008;
inline constexpr xword df_1_loadfltr = 0x00000010;
inline constexpr xword df_1_initfirst = 0x00000020;
inline constexpr xword df_1_noopen = 0x00000040;
inline constexpr xword df_1_origin = 0x00000080;
inline constexpr xword df_1_direct = 0x00000100;
inline constexpr xword df_1_interpose = 0x00000400;
inline constexpr xword df_1_nodeflib = 0x00000800;
inline constexpr xword df_1_nodump = 0x00001000;
inline constexpr xword df_1_confalt = 0x00002000;
inline constexpr xword df_1_noreloc = 0x00400000;
inline constexpr xword df_1_pie = 0x08000000;

// these live in .gnu.version, one half per .dynsym entry
inline constexpr half ver_ndx_local = 0;       // the symbol is not exported
inline constexpr half ver_ndx_global = 1;      // exported, unversioned
inline constexpr half ver_ndx_eliminate = 0xff01;
inline constexpr half ver_ndx_hidden = 0x8000;

inline constexpr half ver_flg_base = 0x1;
inline constexpr half ver_flg_weak = 0x2;

inline constexpr half
elf_ver_ndx(half versym) noexcept
{
  return static_cast<half>(versym & ~ver_ndx_hidden);
}

};      // namespace elf
};      // namespace micron
