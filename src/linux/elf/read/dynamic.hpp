//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../vector.hpp"

#include "image.hpp"
#include "sections.hpp"
#include "segments.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// PT_DYNAMIC / .dynamic

namespace micron
{
namespace elf
{
namespace read
{

struct dyn_row {
  i64 tag = 0;
  u64 val = 0;
};

struct dynamic_info {
  micron::vector<dyn_row> entries{};

  micron::vector<micron::string> needed{};
  micron::string soname{};
  micron::string rpath{};
  micron::string runpath{};
  bool present = false;
  bool from_section = false;      // PT_DYNAMIC was absent and the .dynamic SECTION answered instead
};

constexpr bool
dt_is_strtab_offset(i64 tag)
{
  return tag == dt_needed || tag == dt_soname || tag == dt_rpath || tag == dt_runpath;
}

inline dynamic_info
read_dynamic(const image &img, const micron::vector<segment_row> &segs, const micron::vector<section_row> &secs)
{
  dynamic_info out{};
  if ( !img.ok() || img.src == nullptr ) return out;

  u64 base = 0, span = 0;
  const segment_row *dynseg = find_segment(segs, pt_dynamic);
  if ( dynseg != nullptr ) {
    base = dynseg->offset;
    span = dynseg->filesz;
  } else {
    // a relocatable object or a stripped-phdr file still carries .dynamic as a section
    const section_row *dynsec = find_section_by_type(secs, sht_dynamic);
    if ( dynsec == nullptr || dynsec->type == sht_nobits ) return out;
    base = dynsec->offset;
    span = dynsec->size;
    out.from_section = true;
  }
  out.present = true;

  const u64 entsize = img.is64() ? sizeof(dyn_t) : sizeof(dyn32_t);
  if ( span < entsize ) return out;
  const u64 count = span / entsize;

  micron::vector<dyn_row> entries{};
  entries.reserve(static_cast<usize>(count));
  for ( u64 i = 0; i < count; i++ ) {
    const u64 off = base + i * entsize;
    dyn_row e{};
    if ( img.is64() ) {
      const span_t raw = img.at(off, sizeof(dyn_t));
      if ( raw.len < sizeof(dyn_t) ) break;
      e.tag = static_cast<i64>(rd<u64>(raw.ptr, __builtin_offsetof(dyn_t, tag), img.hdr.data));
      e.val = rd<u64>(raw.ptr, __builtin_offsetof(dyn_t, un), img.hdr.data);
    } else {
      const span_t raw = img.at(off, sizeof(dyn32_t));
      if ( raw.len < sizeof(dyn32_t) ) break;
      // d_tag is a SIGNED word on ELF32; the OS/proc-range tags are negative when read unsigned
      e.tag = static_cast<i64>(static_cast<i32>(rd<u32>(raw.ptr, __builtin_offsetof(dyn32_t, tag), img.hdr.data)));
      e.val = rd<u32>(raw.ptr, __builtin_offsetof(dyn32_t, un), img.hdr.data);
    }
    entries.push_back(e);
    if ( e.tag == dt_null ) break;
  }

  out.entries = micron::move(entries);

  u64 strtab_vaddr = 0;
  bool have_strtab = false;
  u64 strsz = 0;
  micron::vector<u64> needed_offs{};
  u64 soname_off = ~0ull, rpath_off = ~0ull, runpath_off = ~0ull;

  for ( const auto &e : out.entries ) {
    if ( e.tag == dt_strtab ) {
      strtab_vaddr = e.val;
      have_strtab = true;
    } else if ( e.tag == dt_strsz )
      strsz = e.val;
    else if ( e.tag == dt_needed )
      needed_offs.push_back(e.val);
    else if ( e.tag == dt_soname )
      soname_off = e.val;
    else if ( e.tag == dt_rpath )
      rpath_off = e.val;
    else if ( e.tag == dt_runpath )
      runpath_off = e.val;
  }

  if ( !have_strtab ) return out;

  // DT_STRTAB is a VADDR; turn it into a file offset, falling back to .dynstr when no PT_LOAD
  // covers it (which happens in an unlinked or section-only file)
  u64 strtab_off = 0;
  const auto off_r = vaddr_to_offset(segs, strtab_vaddr);
  if ( off_r.is_second() ) {
    const section_row *ds = find_section(secs, ".dynstr");
    if ( ds == nullptr ) return out;
    strtab_off = ds->offset;
    if ( strsz == 0 ) strsz = ds->size;
  } else {
    strtab_off = off_r.cast<u64>();
  }
  const u64 strtab_limit = strsz != 0 ? strsz : (64ull << 10);

  const auto resolve = [&](u64 name_off) {
    const u64 lim = strtab_limit > name_off ? strtab_limit - name_off : 0;
    return read_cstr_at(*img.src, strtab_off + name_off, lim);
  };

  for ( u64 noff : needed_offs ) out.needed.push_back(resolve(noff));
  if ( soname_off != ~0ull ) out.soname = resolve(soname_off);
  if ( rpath_off != ~0ull ) out.rpath = resolve(rpath_off);
  if ( runpath_off != ~0ull ) out.runpath = resolve(runpath_off);

  return out;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
