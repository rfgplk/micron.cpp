//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ELF32 on-disk records variant;
// compiled on every target

namespace micron
{
namespace elf
{

struct ehdr32_t {
  u8 ident[ident_size];
  half type;
  half machine;
  word version;
  addr32 entry;
  off32 phoff;
  off32 shoff;
  word flags;
  half ehsize;
  half phentsize;
  half phnum;
  half shentsize;
  half shnum;
  half shstrndx;
};

// WARNING: field order differs from phdr_t
struct phdr32_t {
  word type;
  off32 offset;
  addr32 vaddr;
  addr32 paddr;
  word filesz;
  word memsz;
  word flags;
  word align;
};

struct shdr32_t {
  word name;
  word type;
  word flags;
  addr32 addr;
  off32 offset;
  word size;
  word link;
  word info;
  word addralign;
  word entsize;
};

// PT_DYNAMIC. d_tag is a plain signed word here, not the 64-bit sxword
struct dyn32_t {
  sword tag;

  union {
    word val;
    addr32 ptr;
  } un;
};

// WARNING: field order differs from sym_t
struct sym32_t {
  word name;
  addr32 value;
  word size;
  u8 info;
  u8 other;
  half shndx;
};

struct rel32_t {
  addr32 offset;
  word info;
};

struct rela32_t {
  addr32 offset;
  word info;
  sword addend;
};

};      // namespace elf
};      // namespace micron
