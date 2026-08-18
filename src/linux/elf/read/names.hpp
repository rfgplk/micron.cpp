//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../consts.hpp"
#include "../header.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// spelling. every one of these returns nullptr for a value it does not know, so a caller can print
// the raw number rather than a wrong name -- these tables are for humans reading a dump, and a
// confidently wrong mnemonic is worse than a hex value.

namespace micron
{
namespace elf
{
namespace read
{

constexpr const char *
etype_name(half et) noexcept
{
  switch ( et ) {
  case et_none:
    return "NONE (No file type)";
  case et_rel:
    return "REL (Relocatable file)";
  case et_exec:
    return "EXEC (Executable file)";
  case et_dyn:
    return "DYN (Shared object file)";
  case et_core:
    return "CORE (Core file)";
  default:
    return nullptr;
  }
}

constexpr const char *
machine_name(half em) noexcept
{
  switch ( em ) {
  case em_x86_64:
    return "x86-64";
  case em_aarch64:
    return "AArch64";
  case em_386:
    return "Intel 80386";
  case em_arm:
    return "ARM";
  case em_mips:
    return "MIPS";
  case em_ppc:
    return "PowerPC";
  case em_ppc64:
    return "PowerPC64";
  case em_s390:
    return "IBM S/390";
  case em_sparcv9:
    return "SPARC V9";
  case em_ia_64:
    return "Intel IA-64";
  case em_riscv:
    return "RISC-V";
  case em_loongarch:
    return "LoongArch";
  default:
    return nullptr;
  }
}

constexpr const char *
osabi_name(u8 abi) noexcept
{
  switch ( abi ) {
  case elfosabi_sysv:
    return "UNIX - System V";
  case elfosabi_hpux:
    return "UNIX - HP-UX";
  case elfosabi_netbsd:
    return "UNIX - NetBSD";
  case elfosabi_gnu:
    return "UNIX - GNU";
  case elfosabi_solaris:
    return "UNIX - Solaris";
  case elfosabi_aix:
    return "UNIX - AIX";
  case elfosabi_irix:
    return "UNIX - IRIX";
  case elfosabi_freebsd:
    return "UNIX - FreeBSD";
  case elfosabi_openbsd:
    return "UNIX - OpenBSD";
  default:
    return nullptr;
  }
}

constexpr const char *
pt_name(word pt) noexcept
{
  switch ( pt ) {
  case pt_null:
    return "NULL";
  case pt_load:
    return "LOAD";
  case pt_dynamic:
    return "DYNAMIC";
  case pt_interp:
    return "INTERP";
  case pt_note:
    return "NOTE";
  case pt_shlib:
    return "SHLIB";
  case pt_phdr:
    return "PHDR";
  case pt_tls:
    return "TLS";
  case pt_gnu_eh_frame:
    return "GNU_EH_FRAME";
  case pt_gnu_stack:
    return "GNU_STACK";
  case pt_gnu_relro:
    return "GNU_RELRO";
  case pt_gnu_property:
    return "GNU_PROPERTY";
  case pt_arm_exidx:
    return "ARM_EXIDX";
  default:
    return nullptr;
  }
}

constexpr const char *
sht_name(word sht) noexcept
{
  switch ( sht ) {
  case sht_null:
    return "NULL";
  case sht_progbits:
    return "PROGBITS";
  case sht_symtab:
    return "SYMTAB";
  case sht_strtab:
    return "STRTAB";
  case sht_rela:
    return "RELA";
  case sht_hash:
    return "HASH";
  case sht_dynamic:
    return "DYNAMIC";
  case sht_note:
    return "NOTE";
  case sht_nobits:
    return "NOBITS";
  case sht_rel:
    return "REL";
  case sht_shlib:
    return "SHLIB";
  case sht_dynsym:
    return "DYNSYM";
  case sht_init_array:
    return "INIT_ARRAY";
  case sht_fini_array:
    return "FINI_ARRAY";
  case sht_preinit_array:
    return "PREINIT_ARRAY";
  case sht_group:
    return "GROUP";
  case sht_symtab_shndx:
    return "SYMTAB_SHNDX";
  case sht_relr:
    return "RELR";
  case sht_gnu_attributes:
    return "GNU_ATTRIBUTES";
  case sht_gnu_hash:
    return "GNU_HASH";
  case sht_gnu_liblist:
    return "GNU_LIBLIST";
  case sht_gnu_verdef:
    return "GNU_verdef";
  case sht_gnu_verneed:
    return "GNU_verneed";
  case sht_gnu_versym:
    return "GNU_versym";
  default:
    return nullptr;
  }
}

constexpr const char *
dt_tag_name(i64 tag) noexcept
{
  switch ( tag ) {
  case dt_null:
    return "DT_NULL";
  case dt_needed:
    return "DT_NEEDED";
  case dt_pltrelsz:
    return "DT_PLTRELSZ";
  case dt_pltgot:
    return "DT_PLTGOT";
  case dt_hash:
    return "DT_HASH";
  case dt_strtab:
    return "DT_STRTAB";
  case dt_symtab:
    return "DT_SYMTAB";
  case dt_rela:
    return "DT_RELA";
  case dt_relasz:
    return "DT_RELASZ";
  case dt_relaent:
    return "DT_RELAENT";
  case dt_strsz:
    return "DT_STRSZ";
  case dt_syment:
    return "DT_SYMENT";
  case dt_init:
    return "DT_INIT";
  case dt_fini:
    return "DT_FINI";
  case dt_soname:
    return "DT_SONAME";
  case dt_rpath:
    return "DT_RPATH";
  case dt_symbolic:
    return "DT_SYMBOLIC";
  case dt_rel:
    return "DT_REL";
  case dt_relsz:
    return "DT_RELSZ";
  case dt_relent:
    return "DT_RELENT";
  case dt_pltrel:
    return "DT_PLTREL";
  case dt_debug:
    return "DT_DEBUG";
  case dt_textrel:
    return "DT_TEXTREL";
  case dt_jmprel:
    return "DT_JMPREL";
  case dt_bind_now:
    return "DT_BIND_NOW";
  case dt_init_array:
    return "DT_INIT_ARRAY";
  case dt_fini_array:
    return "DT_FINI_ARRAY";
  case dt_init_arraysz:
    return "DT_INIT_ARRAYSZ";
  case dt_fini_arraysz:
    return "DT_FINI_ARRAYSZ";
  case dt_runpath:
    return "DT_RUNPATH";
  case dt_flags:
    return "DT_FLAGS";
  case dt_preinit_array:
    return "DT_PREINIT_ARRAY";
  case dt_preinit_arraysz:
    return "DT_PREINIT_ARRAYSZ";
  case dt_relrsz:
    return "DT_RELRSZ";
  case dt_relr:
    return "DT_RELR";
  case dt_relrent:
    return "DT_RELRENT";
  case dt_gnu_hash:
    return "DT_GNU_HASH";
  case dt_relacount:
    return "DT_RELACOUNT";
  case dt_relcount:
    return "DT_RELCOUNT";
  case dt_flags_1:
    return "DT_FLAGS_1";
  case dt_verdef:
    return "DT_VERDEF";
  case dt_verdefnum:
    return "DT_VERDEFNUM";
  case dt_verneed:
    return "DT_VERNEED";
  case dt_verneednum:
    return "DT_VERNEEDNUM";
  case dt_versym:
    return "DT_VERSYM";
  default:
    // DT_LOPROC..DT_HIPROC is a PER-MACHINE namespace: 0x70000001 is DT_ARM_EXIDX on arm and
    // DT_X86_64_PLTSZ on amd64. it cannot be spelled without e_machine -- use the overload.
    return nullptr;
  }
}

// the processor range, which only e_machine can disambiguate. falls back to the generic table for
// every tag below DT_LOPROC.
constexpr const char *
dt_tag_name(half machine, i64 tag) noexcept
{
  if ( tag < dt_loproc || tag > dt_hiproc ) return dt_tag_name(tag);
  switch ( machine ) {
  case em_arm:
    switch ( tag ) {
    case dt_arm_exidx:
      return "DT_ARM_EXIDX";
    case dt_arm_exidxsz:
      return "DT_ARM_EXIDXSZ";
    default:
      return nullptr;
    }
  case em_x86_64:
    switch ( tag ) {
    case 0x70000000:
      return "DT_X86_64_PLT";
    case 0x70000001:
      return "DT_X86_64_PLTSZ";
    case 0x70000003:
      return "DT_X86_64_PLTENT";
    default:
      return nullptr;
    }
  default:
    return nullptr;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// relocation types. these are per-machine namespaces: type 8 is R_X86_64_RELATIVE on amd64 and
// R_386_RELATIVE on i386 by coincidence, but type 23 is R_386_PC8 and R_ARM_RELATIVE. always pair
// the number with e_machine.

namespace __impl
{

constexpr const char *
r_name_x86_64(u32 t)
{
  switch ( t ) {
  case 0:
    return "R_X86_64_NONE";
  case 1:
    return "R_X86_64_64";
  case 2:
    return "R_X86_64_PC32";
  case 3:
    return "R_X86_64_GOT32";
  case 4:
    return "R_X86_64_PLT32";
  case 5:
    return "R_X86_64_COPY";
  case 6:
    return "R_X86_64_GLOB_DAT";
  case 7:
    return "R_X86_64_JUMP_SLOT";
  case 8:
    return "R_X86_64_RELATIVE";
  case 9:
    return "R_X86_64_GOTPCREL";
  case 10:
    return "R_X86_64_32";
  case 11:
    return "R_X86_64_32S";
  case 12:
    return "R_X86_64_16";
  case 13:
    return "R_X86_64_PC16";
  case 14:
    return "R_X86_64_8";
  case 15:
    return "R_X86_64_PC8";
  case 16:
    return "R_X86_64_DTPMOD64";
  case 17:
    return "R_X86_64_DTPOFF64";
  case 18:
    return "R_X86_64_TPOFF64";
  case 19:
    return "R_X86_64_TLSGD";
  case 20:
    return "R_X86_64_TLSLD";
  case 21:
    return "R_X86_64_DTPOFF32";
  case 22:
    return "R_X86_64_GOTTPOFF";
  case 23:
    return "R_X86_64_TPOFF32";
  case 24:
    return "R_X86_64_PC64";
  case 25:
    return "R_X86_64_GOTOFF64";
  case 26:
    return "R_X86_64_GOTPC32";
  case 27:
    return "R_X86_64_GOT64";
  case 28:
    return "R_X86_64_GOTPCREL64";
  case 29:
    return "R_X86_64_GOTPC64";
  case 30:
    return "R_X86_64_GOTPLT64";
  case 31:
    return "R_X86_64_PLTOFF64";
  case 32:
    return "R_X86_64_SIZE32";
  case 33:
    return "R_X86_64_SIZE64";
  case 34:
    return "R_X86_64_GOTPC32_TLSDESC";
  case 35:
    return "R_X86_64_TLSDESC_CALL";
  case 36:
    return "R_X86_64_TLSDESC";
  case 37:
    return "R_X86_64_IRELATIVE";
  case 38:
    return "R_X86_64_RELATIVE64";
  case 41:
    return "R_X86_64_GOTPCRELX";
  case 42:
    return "R_X86_64_REX_GOTPCRELX";
  default:
    return nullptr;
  }
}

constexpr const char *
r_name_386(u32 t)
{
  switch ( t ) {
  case 0:
    return "R_386_NONE";
  case 1:
    return "R_386_32";
  case 2:
    return "R_386_PC32";
  case 3:
    return "R_386_GOT32";
  case 4:
    return "R_386_PLT32";
  case 5:
    return "R_386_COPY";
  case 6:
    return "R_386_GLOB_DAT";
  case 7:
    return "R_386_JMP_SLOT";
  case 8:
    return "R_386_RELATIVE";
  case 9:
    return "R_386_GOTOFF";
  case 10:
    return "R_386_GOTPC";
  case 11:
    return "R_386_32PLT";
  case 14:
    return "R_386_TLS_TPOFF";
  case 15:
    return "R_386_TLS_IE";
  case 16:
    return "R_386_TLS_GOTIE";
  case 17:
    return "R_386_TLS_LE";
  case 18:
    return "R_386_TLS_GD";
  case 19:
    return "R_386_TLS_LDM";
  case 20:
    return "R_386_16";
  case 21:
    return "R_386_PC16";
  case 22:
    return "R_386_8";
  case 23:
    return "R_386_PC8";
  case 35:
    return "R_386_TLS_DTPMOD32";
  case 36:
    return "R_386_TLS_DTPOFF32";
  case 37:
    return "R_386_TLS_TPOFF32";
  case 38:
    return "R_386_SIZE32";
  case 39:
    return "R_386_TLS_GOTDESC";
  case 40:
    return "R_386_TLS_DESC_CALL";
  case 41:
    return "R_386_TLS_DESC";
  case 42:
    return "R_386_IRELATIVE";
  case 43:
    return "R_386_GOT32X";
  default:
    return nullptr;
  }
}

constexpr const char *
r_name_arm(u32 t)
{
  switch ( t ) {
  case 0:
    return "R_ARM_NONE";
  case 1:
    return "R_ARM_PC24";
  case 2:
    return "R_ARM_ABS32";
  case 3:
    return "R_ARM_REL32";
  case 4:
    return "R_ARM_LDR_PC_G0";
  case 5:
    return "R_ARM_ABS16";
  case 6:
    return "R_ARM_ABS12";
  case 7:
    return "R_ARM_THM_ABS5";
  case 8:
    return "R_ARM_ABS8";
  case 9:
    return "R_ARM_SBREL32";
  case 10:
    return "R_ARM_THM_CALL";
  case 11:
    return "R_ARM_THM_PC8";
  case 13:
    return "R_ARM_TLS_DESC";
  case 17:
    return "R_ARM_TLS_DTPMOD32";
  case 18:
    return "R_ARM_TLS_DTPOFF32";
  case 19:
    return "R_ARM_TLS_TPOFF32";
  case 20:
    return "R_ARM_COPY";
  case 21:
    return "R_ARM_GLOB_DAT";
  case 22:
    return "R_ARM_JUMP_SLOT";
  case 23:
    return "R_ARM_RELATIVE";
  case 24:
    return "R_ARM_GOTOFF32";
  case 25:
    return "R_ARM_BASE_PREL";
  case 26:
    return "R_ARM_GOT_BREL";
  case 27:
    return "R_ARM_PLT32";
  case 28:
    return "R_ARM_CALL";
  case 29:
    return "R_ARM_JUMP24";
  case 30:
    return "R_ARM_THM_JUMP24";
  case 31:
    return "R_ARM_BASE_ABS";
  case 38:
    return "R_ARM_TARGET1";
  case 39:
    return "R_ARM_SBREL31";
  case 40:
    return "R_ARM_V4BX";
  case 41:
    return "R_ARM_TARGET2";
  case 42:
    return "R_ARM_PREL31";
  case 43:
    return "R_ARM_MOVW_ABS_NC";
  case 44:
    return "R_ARM_MOVT_ABS";
  case 104:
    return "R_ARM_TLS_GD32";
  case 105:
    return "R_ARM_TLS_LDM32";
  case 106:
    return "R_ARM_TLS_LDO32";
  case 107:
    return "R_ARM_TLS_IE32";
  case 108:
    return "R_ARM_TLS_LE32";
  case 160:
    return "R_ARM_IRELATIVE";
  default:
    return nullptr;
  }
}

constexpr const char *
r_name_aarch64(u32 t)
{
  switch ( t ) {
  case 0:
    return "R_AARCH64_NONE";
  case 257:
    return "R_AARCH64_ABS64";
  case 258:
    return "R_AARCH64_ABS32";
  case 259:
    return "R_AARCH64_ABS16";
  case 260:
    return "R_AARCH64_PREL64";
  case 261:
    return "R_AARCH64_PREL32";
  case 262:
    return "R_AARCH64_PREL16";
  case 274:
    return "R_AARCH64_ADR_PREL_LO21";
  case 275:
    return "R_AARCH64_ADR_PREL_PG_HI21";
  case 277:
    return "R_AARCH64_ADD_ABS_LO12_NC";
  case 282:
    return "R_AARCH64_JUMP26";
  case 283:
    return "R_AARCH64_CALL26";
  case 286:
    return "R_AARCH64_LDST64_ABS_LO12_NC";
  case 311:
    return "R_AARCH64_ADR_GOT_PAGE";
  case 312:
    return "R_AARCH64_LD64_GOT_LO12_NC";
  case 1024:
    return "R_AARCH64_COPY";
  case 1025:
    return "R_AARCH64_GLOB_DAT";
  case 1026:
    return "R_AARCH64_JUMP_SLOT";
  case 1027:
    return "R_AARCH64_RELATIVE";
  case 1028:
    return "R_AARCH64_TLS_DTPMOD";
  case 1029:
    return "R_AARCH64_TLS_DTPREL";
  case 1030:
    return "R_AARCH64_TLS_TPREL";
  case 1031:
    return "R_AARCH64_TLSDESC";
  case 1032:
    return "R_AARCH64_IRELATIVE";
  default:
    return nullptr;
  }
}

};      // namespace __impl

// nullptr when the pair is unknown
constexpr const char *
reloc_type_name(half machine, u32 type)
{
  switch ( machine ) {
  case em_x86_64:
    return __impl::r_name_x86_64(type);
  case em_386:
    return __impl::r_name_386(type);
  case em_arm:
    return __impl::r_name_arm(type);
  case em_aarch64:
    return __impl::r_name_aarch64(type);
  default:
    return nullptr;
  }
}

};      // namespace read
};      // namespace elf
};      // namespace micron
