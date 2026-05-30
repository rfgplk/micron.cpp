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

struct symbol_info_t {
  const char *name;      // cstring into module's .dynstr
  void *address;         // load_base + sym.value
  xword size;
  u8 type;         // stt_func or stt_object or stt_notype
  u8 binding;      // stb_global or stb_weak or stb_local
  bool defined;
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

struct dyn_info_t {
  const sym_t *symtab = nullptr;
  const char *strtab = nullptr;
  xword strsz = 0;
  xword symcount = 0;      // entries in .dynsym

  const word *hash = nullptr;          // DT_HASH (SysV)
  const word *gnu_hash = nullptr;      // DT_GNU_HASH

  const rela_t *rela = nullptr;
  xword relasz = 0;
  xword relaent = 0;
  xword rela_count = 0;      // DT_RELACOUNT

  const xword *relr = nullptr;      // DT_RELR (packed relative relocations)
  xword relrsz = 0;                 // DT_RELRSZ

  const rela_t *jmprel = nullptr;      // PLT relocations (always RELA on amd64/arm64)
  xword pltrelsz = 0;
  sxword pltrel = 0;      // DT_RELA or DT_REL

  void (*const *init_array)() = nullptr;
  xword init_arraysz = 0;
  void (*const *fini_array)() = nullptr;
  xword fini_arraysz = 0;
  void (*init)() = nullptr;
  void (*fini)() = nullptr;

  const half *versym = nullptr;
  const void *verdef = nullptr;
  word verdefnum = 0;
  const void *verneed = nullptr;
  word verneednum = 0;

  xword flags = 0;
  xword flags1 = 0;

  const char *soname = nullptr;
  const char *rpath = nullptr;
  const char *runpath = nullptr;
};

};      // namespace elf
};      // namespace micron
