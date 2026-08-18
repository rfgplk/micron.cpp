//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../vector.hpp"

#include "image.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// program/segment headers

namespace micron
{
namespace elf
{
namespace read
{

struct segment_row {
  u32 type = 0;
  u32 flags = 0;
  u64 offset = 0;
  u64 vaddr = 0;
  u64 paddr = 0;
  u64 filesz = 0;
  u64 memsz = 0;
  u64 align = 0;
};

enum class link_kind : u8 { static_exec, dynamic, static_pie };

inline micron::vector<segment_row>
walk_segments(const image &img)
{
  micron::vector<segment_row> out{};
  if ( !img.ok() || img.hdr.phoff == 0 || img.hdr.phnum == 0 ) return out;

  out.reserve(img.hdr.phnum);
  for ( u16 i = 0; i < img.hdr.phnum; i++ ) {
    const u64 off = img.hdr.phoff + static_cast<u64>(i) * static_cast<u64>(img.hdr.phentsize);
    segment_row row{};

    if ( img.is64() ) {
      const span_t raw = img.at(off, sizeof(phdr_t));
      if ( raw.len < sizeof(phdr_t) ) break;
      row.type = rd<u32>(raw.ptr, __builtin_offsetof(phdr_t, type), img.hdr.data);
      row.flags = rd<u32>(raw.ptr, __builtin_offsetof(phdr_t, flags), img.hdr.data);
      row.offset = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, offset), img.hdr.data);
      row.vaddr = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, vaddr), img.hdr.data);
      row.paddr = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, paddr), img.hdr.data);
      row.filesz = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, filesz), img.hdr.data);
      row.memsz = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, memsz), img.hdr.data);
      row.align = rd<u64>(raw.ptr, __builtin_offsetof(phdr_t, align), img.hdr.data);
    } else {
      // WARNING: phdr32_t moves p_flags from field 2 to field 7
      const span_t raw = img.at(off, sizeof(phdr32_t));
      if ( raw.len < sizeof(phdr32_t) ) break;
      row.type = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, type), img.hdr.data);
      row.flags = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, flags), img.hdr.data);
      row.offset = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, offset), img.hdr.data);
      row.vaddr = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, vaddr), img.hdr.data);
      row.paddr = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, paddr), img.hdr.data);
      row.filesz = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, filesz), img.hdr.data);
      row.memsz = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, memsz), img.hdr.data);
      row.align = rd<u32>(raw.ptr, __builtin_offsetof(phdr32_t, align), img.hdr.data);
    }
    out.push_back(row);
  }
  return out;
}

inline const segment_row *
find_segment(const micron::vector<segment_row> &segs, u32 pt)
{
  for ( const auto &s : segs )
    if ( s.type == pt ) return &s;
  return nullptr;
}

inline micron::option<u64, const char *>
vaddr_to_offset(const micron::vector<segment_row> &segs, u64 vaddr)
{
  using res = micron::option<u64, const char *>;
  for ( const auto &s : segs ) {
    if ( s.type != pt_load ) continue;
    if ( vaddr >= s.vaddr && vaddr < s.vaddr + s.filesz ) return res{ s.offset + (vaddr - s.vaddr) };
  }
  return res{ "vaddr not backed by a PT_LOAD segment" };
}

inline micron::option<micron::string, const char *>
read_interp(const image &img, const micron::vector<segment_row> &segs)
{
  using res = micron::option<micron::string, const char *>;
  const segment_row *seg = find_segment(segs, pt_interp);
  if ( seg == nullptr || img.src == nullptr ) return res{ "no PT_INTERP segment" };
  return res{ read_cstr_at(*img.src, seg->offset, seg->filesz) };
}

inline link_kind
classify_link(const micron::vector<segment_row> &segs)
{
  if ( find_segment(segs, pt_interp) != nullptr ) return link_kind::dynamic;
  if ( find_segment(segs, pt_dynamic) != nullptr ) return link_kind::static_pie;
  return link_kind::static_exec;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
