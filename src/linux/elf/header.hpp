//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "consts.hpp"
#include "header32.hpp"

namespace micron
{
namespace elf
{

struct ehdr_t {
  u8 ident[ident_size];
  half type;
  half machine;
  word version;
  addr64 entry;
  off64 phoff;
  off64 shoff;
  word flags;
  half ehsize;
  half phentsize;
  half phnum;
  half shentsize;
  half shnum;
  half shstrndx;
};

struct phdr_t {
  word type;
  word flags;
  off64 offset;
  addr64 vaddr;
  addr64 paddr;
  xword filesz;
  xword memsz;
  xword align;
};

struct shdr_t {
  word name;
  word type;
  xword flags;
  addr64 addr;
  off64 offset;
  xword size;
  word link;
  word info;
  xword addralign;
  xword entsize;
};

// PT_DYNAMIC
struct dyn_t {
  sxword tag;

  union {
    xword val;
    addr64 ptr;
  } un;
};

struct sym_t {
  word name;
  u8 info;
  u8 other;
  half shndx;
  addr64 value;
  xword size;
};

struct rel_t {
  addr64 offset;
  xword info;
};

struct rela_t {
  addr64 offset;
  xword info;
  sxword addend;
};

struct verdef_t {
  half version;
  half flags;
  half ndx;
  half cnt;
  word hash;
  word aux;
  word next;
};

struct verdaux_t {
  word name;
  word next;
};

struct verneed_t {
  half version;
  half cnt;
  word file;
  word aux;
  word next;
};

struct vernaux_t {
  word hash;
  half flags;
  half other;
  word name;
  word next;
};

static_assert(sizeof(verdef_t) == 20, "verdef must stay 20 bytes; the chain is walked by offset");
static_assert(sizeof(verdaux_t) == 8, "verdaux must stay 8 bytes");
static_assert(sizeof(verneed_t) == 16, "verneed must stay 16 bytes");
static_assert(sizeof(vernaux_t) == 16, "vernaux must stay 16 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// elf_traits

template<fmt_class C> struct elf_traits;

template<> struct elf_traits<fmt_class::elf32> {
  static constexpr fmt_class cls = fmt_class::elf32;
  static constexpr u8 ident_class = elfclass32;

  using addr = addr32;
  using off = off32;
  using uword = word;
  using sxwd = sword;
  using ehdr = ehdr32_t;
  using phdr = phdr32_t;
  using shdr = shdr32_t;
  using dyn = dyn32_t;
  using sym = sym32_t;
  using rel = rel32_t;
  using rela = rela32_t;

  static constexpr usize bloom_bits = 32;      // DT_GNU_HASH bloom words are 32-bit here
  static constexpr usize relr_bits = 31;       // slots named by one DT_RELR bitmap word

  static constexpr u32
  r_sym(uword info) noexcept
  {
    return info >> 8;
  }

  static constexpr u32
  r_type(uword info) noexcept
  {
    return info & 0xffu;
  }
};

template<> struct elf_traits<fmt_class::elf64> {
  static constexpr fmt_class cls = fmt_class::elf64;
  static constexpr u8 ident_class = elfclass64;

  using addr = addr64;
  using off = off64;
  using uword = xword;
  using sxwd = sxword;
  using ehdr = ehdr_t;
  using phdr = phdr_t;
  using shdr = shdr_t;
  using dyn = dyn_t;
  using sym = sym_t;
  using rel = rel_t;
  using rela = rela_t;

  static constexpr usize bloom_bits = 64;
  static constexpr usize relr_bits = 63;

  static constexpr u32
  r_sym(uword info) noexcept
  {
    return static_cast<u32>(info >> 32);
  }

  static constexpr u32
  r_type(uword info) noexcept
  {
    return static_cast<u32>(info & 0xffffffffu);
  }
};

using native_traits = elf_traits<native_class>;

inline constexpr half expected_machine
#if defined(__micron_arch_amd64)
    = em_x86_64;
#elif defined(__micron_arch_arm64)
    = em_aarch64;
#elif defined(__micron_arch_x86)
    = em_386;
#elif defined(__micron_arch_arm32)
    = em_arm;
#else
    = 0;
#endif

using nehdr_t = native_traits::ehdr;
using nphdr_t = native_traits::phdr;
using nshdr_t = native_traits::shdr;
using ndyn_t = native_traits::dyn;
using nsym_t = native_traits::sym;
using nrel_t = native_traits::rel;
using nrela_t = native_traits::rela;
using naddr_t = native_traits::addr;
using nword_t = native_traits::uword;

struct symbol_info_t {
  const char *name;      // cstring into module's .dynstr
  void *address;         // load_base + sym.value
  xword size;
  u8 type;            // stt_func or stt_object or stt_notype
  u8 binding;         // stb_global or stb_weak or stb_local
  u8 visibility;      // stv_default / stv_hidden / stv_protected, from st_other & 0x3
  bool defined;
};

template<fmt_class C> struct dyn_info {
  using tr = elf_traits<C>;

  const typename tr::sym *symtab = nullptr;
  const char *strtab = nullptr;
  xword strsz = 0;
  xword symcount = 0;      // entries in .dynsym

  const word *hash = nullptr;
  const word *gnu_hash = nullptr;

  const typename tr::rela *rela = nullptr;
  xword relasz = 0;
  xword relaent = 0;
  xword rela_count = 0;      // DT_RELACOUNT

  const typename tr::rel *rel = nullptr;
  xword relsz = 0;
  xword relent = 0;
  xword rel_count = 0;      // DT_RELCOUNT

  const typename tr::uword *relr = nullptr;      // DT_RELR (packed relative relocations)
  xword relrsz = 0;                              // DT_RELRSZ

  const void *jmprel = nullptr;      // PLT relocations; REL or RELA per DT_PLTREL
  xword pltrelsz = 0;
  sxword pltrel = 0;      // dt_rela or dt_rel
  typename tr::addr *pltgot = nullptr;

  void (*const *init_array)() = nullptr;
  xword init_arraysz = 0;
  void (*const *fini_array)() = nullptr;
  xword fini_arraysz = 0;
  void (*const *preinit_array)() = nullptr;
  xword preinit_arraysz = 0;
  void (*init)() = nullptr;
  void (*fini)() = nullptr;

  const half *versym = nullptr;
  const verdef_t *verdef = nullptr;
  word verdefnum = 0;
  const verneed_t *verneed = nullptr;
  word verneednum = 0;

  xword flags = 0;
  xword flags1 = 0;

  const char *soname = nullptr;
  const char *rpath = nullptr;
  const char *runpath = nullptr;

  static constexpr usize max_needed = 64;
  word needed[max_needed] = {};
  usize needed_count = 0;
  bool needed_truncated = false;
};

using dyn_info_t = dyn_info<native_class>;

};      // namespace elf
};      // namespace micron
