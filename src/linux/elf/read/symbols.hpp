//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../vector.hpp"

#include "image.hpp"
#include "sections.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// static (.symtab/.strtab)
// dynamic (.dynsym/.dynstr) symbol walks

namespace micron
{
namespace elf
{
namespace read
{

enum class sym_table_kind : u8 { static_symtab, dynamic_dynsym };

struct symbol_row {
  micron::string name{};
  u64 value = 0;      // st_value
  u64 size = 0;
  u8 bind = 0;
  u8 type = 0;
  u8 visibility = 0;
  u16 shndx = 0;
  bool defined = false;
  sym_table_kind table = sym_table_kind::static_symtab;
};

namespace __impl
{

inline void
read_one_symbol(const image &img, const u8 *raw, symbol_row &row, u64 &name_off)
{
  u8 info = 0, other = 0;
  if ( img.is64() ) {
    name_off = rd<u32>(raw, __builtin_offsetof(sym_t, name), img.hdr.data);
    row.value = rd<u64>(raw, __builtin_offsetof(sym_t, value), img.hdr.data);
    row.size = rd<u64>(raw, __builtin_offsetof(sym_t, size), img.hdr.data);
    info = raw[__builtin_offsetof(sym_t, info)];
    other = raw[__builtin_offsetof(sym_t, other)];
    row.shndx = rd<u16>(raw, __builtin_offsetof(sym_t, shndx), img.hdr.data);
  } else {
    name_off = rd<u32>(raw, __builtin_offsetof(sym32_t, name), img.hdr.data);
    row.value = rd<u32>(raw, __builtin_offsetof(sym32_t, value), img.hdr.data);
    row.size = rd<u32>(raw, __builtin_offsetof(sym32_t, size), img.hdr.data);
    info = raw[__builtin_offsetof(sym32_t, info)];
    other = raw[__builtin_offsetof(sym32_t, other)];
    row.shndx = rd<u16>(raw, __builtin_offsetof(sym32_t, shndx), img.hdr.data);
  }
  row.bind = elf_st_bind(info);
  row.type = elf_st_type(info);
  row.visibility = elf_st_visibility(other);
  row.defined = row.shndx != shn_undef;
}

inline void
walk_one_table(const image &img, const section_row *symtab_sec, const micron::vector<section_row> &secs, sym_table_kind kind,
               micron::vector<symbol_row> &out)
{
  if ( symtab_sec == nullptr || img.src == nullptr ) return;
  const u64 entsize = img.is64() ? sizeof(sym_t) : sizeof(sym32_t);
  if ( symtab_sec->size < entsize ) return;
  const u64 count = clamp_records(img, symtab_sec->offset, entsize, symtab_sec->size / entsize);

  const section_row *strtab_sec = symtab_sec->link < secs.size() ? &secs[symtab_sec->link] : nullptr;

  out.reserve(out.size() + static_cast<usize>(count));
  for ( u64 i = 0; i < count; i++ ) {
    const u64 off = symtab_sec->offset + i * entsize;
    const span_t raw = img.at(off, entsize);
    if ( raw.len < entsize ) break;

    symbol_row row{};
    row.table = kind;
    u64 name_off = 0;
    read_one_symbol(img, raw.ptr, row, name_off);
    if ( strtab_sec != nullptr && name_off != 0 ) {
      const u64 lim = strtab_sec->size > name_off ? strtab_sec->size - name_off : 0;
      row.name = read_cstr_at(*img.src, strtab_sec->offset + name_off, lim);
    }
    out.push_back(micron::move(row));
  }
}

};      // namespace __impl

inline micron::vector<symbol_row>
walk_symbols(const image &img, const micron::vector<section_row> &secs)
{
  micron::vector<symbol_row> out{};
  if ( !img.ok() ) return out;
  __impl::walk_one_table(img, find_section_by_type(secs, sht_symtab), secs, sym_table_kind::static_symtab, out);
  __impl::walk_one_table(img, find_section_by_type(secs, sht_dynsym), secs, sym_table_kind::dynamic_dynsym, out);
  return out;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
