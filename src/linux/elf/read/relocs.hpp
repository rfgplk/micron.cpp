//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../vector.hpp"

#include "image.hpp"
#include "sections.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// relocation walks of SHT_RELA, SHT_REL and SHT_RELR

namespace micron
{
namespace elf
{
namespace read
{

struct reloc_row {
  u64 offset = 0;      // r_offset: the vaddr being patched
  i64 addend = 0;      // r_addend; 0 and meaningless unless has_addend
  u32 sym = 0;         // index into the sh_link symbol table; stn_undef is "no symbol"
  u32 type = 0;
  bool has_addend = false;      // RELA rather than REL
};

inline micron::vector<reloc_row>
walk_relocs(const image &img, const section_row &sec)
{
  micron::vector<reloc_row> out{};
  const bool rela = sec.type == sht_rela;
  if ( !rela && sec.type != sht_rel ) return out;

  const u64 esz = img.is64() ? (rela ? sizeof(rela_t) : sizeof(rel_t)) : (rela ? sizeof(rela32_t) : sizeof(rel32_t));
  if ( esz == 0 || sec.size < esz ) return out;
  const u64 count = clamp_records(img, sec.offset, esz, sec.size / esz);

  out.reserve(static_cast<usize>(count));
  for ( u64 i = 0; i < count; i++ ) {
    const span_t raw = img.at(sec.offset + i * esz, esz);
    if ( raw.len < esz ) break;      // a partial record is not a row

    reloc_row r{};
    r.has_addend = rela;
    if ( img.is64() ) {
      const u64 info = rela ? rd<u64>(raw.ptr, __builtin_offsetof(rela_t, info), img.hdr.data)
                            : rd<u64>(raw.ptr, __builtin_offsetof(rel_t, info), img.hdr.data);
      r.offset = rela ? rd<u64>(raw.ptr, __builtin_offsetof(rela_t, offset), img.hdr.data)
                      : rd<u64>(raw.ptr, __builtin_offsetof(rel_t, offset), img.hdr.data);
      r.sym = elf64_r_sym(info);
      r.type = elf64_r_type(info);
      if ( rela ) r.addend = static_cast<i64>(rd<u64>(raw.ptr, __builtin_offsetof(rela_t, addend), img.hdr.data));
    } else {
      const u32 info = rela ? rd<u32>(raw.ptr, __builtin_offsetof(rela32_t, info), img.hdr.data)
                            : rd<u32>(raw.ptr, __builtin_offsetof(rel32_t, info), img.hdr.data);
      r.offset = rela ? rd<u32>(raw.ptr, __builtin_offsetof(rela32_t, offset), img.hdr.data)
                      : rd<u32>(raw.ptr, __builtin_offsetof(rel32_t, offset), img.hdr.data);
      r.sym = elf32_r_sym(info);
      r.type = elf32_r_type(info);
      if ( rela ) r.addend = static_cast<i64>(static_cast<i32>(rd<u32>(raw.ptr, __builtin_offsetof(rela32_t, addend), img.hdr.data)));
    }
    out.push_back(r);
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SHT_RELR

inline constexpr u64 relr_max_rows = 1u << 22;

struct relr_out {
  micron::vector<u64> at{};
  bool capped = false;
};

inline relr_out
walk_relr(const image &img, const section_row &sec)
{
  relr_out out{};
  if ( sec.type != sht_relr ) return out;

  const u64 esz = img.is64() ? 8 : 4;
  const u64 step = esz;              // one pointer slot per bit, and a slot is a pointer wide
  const u64 bits = esz * 8 - 1;      // 63 on a 64-bit file, 31 on a 32-bit one
  if ( sec.size < esz ) return out;
  const u64 count = sec.size / esz;

  u64 where = 0;
  for ( u64 i = 0; i < count; i++ ) {
    const span_t raw = img.at(sec.offset + i * esz, esz);
    if ( raw.len < esz ) break;
    const u64 e = img.is64() ? rd<u64>(raw.ptr, 0, img.hdr.data) : rd<u32>(raw.ptr, 0, img.hdr.data);

    if ( (e & 1) == 0 ) {
      where = e;
      if ( out.at.size() >= relr_max_rows ) {
        out.capped = true;
        break;
      }
      out.at.push_back(where);
      where += step;
      continue;
    }

    u64 b = e >> 1;
    for ( u64 k = 0; b != 0; k++, b >>= 1 ) {
      if ( (b & 1) == 0 ) continue;
      if ( out.at.size() >= relr_max_rows ) {
        out.capped = true;
        break;
      }
      out.at.push_back(where + k * step);
    }
    if ( out.capped ) break;
    where += bits * step;
  }
  return out;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
