//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../memory/cstring.hpp"
#include "../../../vector.hpp"

#include "image.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// section headers

namespace micron
{
namespace elf
{
namespace read
{

struct section_row {
  micron::string name{};
  u32 type = 0;
  u64 flags = 0;
  u64 addr = 0;
  u64 offset = 0;
  u64 size = 0;
  u32 link = 0;
  u32 info = 0;
  u64 addralign = 0;
  u64 entsize = 0;
};

namespace __impl
{

inline bool
read_one_section(const image &img, u64 idx, u32 &name_off, section_row &row)
{
  const u64 off = img.hdr.shoff + idx * static_cast<u64>(img.hdr.shentsize);

  if ( img.is64() ) {
    const span_t raw = img.at(off, sizeof(shdr_t));
    if ( raw.len < sizeof(shdr_t) ) return false;
    name_off = rd<u32>(raw.ptr, __builtin_offsetof(shdr_t, name), img.hdr.data);
    row.type = rd<u32>(raw.ptr, __builtin_offsetof(shdr_t, type), img.hdr.data);
    row.flags = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, flags), img.hdr.data);
    row.addr = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, addr), img.hdr.data);
    row.offset = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, offset), img.hdr.data);
    row.size = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, size), img.hdr.data);
    row.link = rd<u32>(raw.ptr, __builtin_offsetof(shdr_t, link), img.hdr.data);
    row.info = rd<u32>(raw.ptr, __builtin_offsetof(shdr_t, info), img.hdr.data);
    row.addralign = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, addralign), img.hdr.data);
    row.entsize = rd<u64>(raw.ptr, __builtin_offsetof(shdr_t, entsize), img.hdr.data);
  } else {
    const span_t raw = img.at(off, sizeof(shdr32_t));
    if ( raw.len < sizeof(shdr32_t) ) return false;
    name_off = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, name), img.hdr.data);
    row.type = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, type), img.hdr.data);
    row.flags = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, flags), img.hdr.data);
    row.addr = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, addr), img.hdr.data);
    row.offset = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, offset), img.hdr.data);
    row.size = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, size), img.hdr.data);
    row.link = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, link), img.hdr.data);
    row.info = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, info), img.hdr.data);
    row.addralign = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, addralign), img.hdr.data);
    row.entsize = rd<u32>(raw.ptr, __builtin_offsetof(shdr32_t, entsize), img.hdr.data);
  }
  return true;
}

};      // namespace __impl

inline micron::vector<section_row>
walk_sections(const image &img)
{
  micron::vector<section_row> out{};
  if ( !img.ok() || img.hdr.shoff == 0 ) return out;

  if ( img.hdr.shentsize == 0 ) return out;

  u64 shnum = img.hdr.shnum;
  u64 shstrndx = img.hdr.shstrndx;

  if ( shnum == 0 || shstrndx == shn_xindex ) {
    section_row sec0{};
    u32 noff0 = 0;
    if ( __impl::read_one_section(img, 0, noff0, sec0) ) {
      if ( shnum == 0 ) shnum = sec0.size;
      if ( shstrndx == shn_xindex ) shstrndx = sec0.link;
    }
  }
  shnum = clamp_records(img, img.hdr.shoff, img.hdr.shentsize, shnum);
  if ( shnum == 0 ) return out;

  out.reserve(static_cast<usize>(shnum));
  micron::vector<u32> name_offs{};
  name_offs.reserve(static_cast<usize>(shnum));

  for ( u64 i = 0; i < shnum; i++ ) {
    section_row row{};
    u32 noff = 0;
    if ( !__impl::read_one_section(img, i, noff, row) ) break;
    name_offs.push_back(noff);
    out.push_back(micron::move(row));
  }

  if ( shstrndx < out.size() && img.src ) {
    const u64 strtab_off = out[static_cast<usize>(shstrndx)].offset;
    const u64 strtab_size = out[static_cast<usize>(shstrndx)].size;
    for ( usize i = 0; i < out.size(); i++ ) {
      const u32 noff = name_offs[i];
      const u64 lim = strtab_size > noff ? strtab_size - noff : 0;
      out[i].name = read_cstr_at(*img.src, strtab_off + noff, lim);
    }
  }
  return out;
}

inline const section_row *
find_section(const micron::vector<section_row> &secs, const char *name)
{
  for ( const auto &s : secs )
    if ( micron::strcmp(s.name.c_str(), name) == 0 ) return &s;
  return nullptr;
}

inline const section_row *
find_section_by_type(const micron::vector<section_row> &secs, u32 sht)
{
  for ( const auto &s : secs )
    if ( s.type == sht ) return &s;
  return nullptr;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
