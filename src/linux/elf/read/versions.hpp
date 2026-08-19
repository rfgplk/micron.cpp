//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../vector.hpp"

#include "image.hpp"
#include "sections.hpp"

namespace micron
{
namespace elf
{
namespace read
{

inline constexpr usize ver_chain_max = 4096;

struct version_row {
  micron::string name{};
  micron::string file{};
  u16 index = 0;
  u16 flags = 0;
  bool is_need = false;
};

namespace __impl
{

inline const section_row *
linked_strtab(const micron::vector<section_row> &secs, const section_row &s)
{
  return s.link < secs.size() ? &secs[s.link] : nullptr;
}

};      // namespace __impl

inline micron::vector<half>
read_versym(const image &img, const section_row &sec)
{
  micron::vector<half> out{};
  if ( sec.type != sht_gnu_versym || sec.size < 2 ) return out;
  const u64 count = clamp_records(img, sec.offset, 2, sec.size / 2);
  out.reserve(static_cast<usize>(count));
  for ( u64 i = 0; i < count; i++ ) {
    const span_t raw = img.at(sec.offset + i * 2, 2);
    if ( raw.len < 2 ) break;
    out.push_back(rd<half>(raw.ptr, 0, img.hdr.data));
  }
  return out;
}

inline micron::vector<version_row>
read_verdef(const image &img, const micron::vector<section_row> &secs, const section_row &sec)
{
  micron::vector<version_row> out{};
  if ( sec.type != sht_gnu_verdef || img.src == nullptr ) return out;
  const section_row *str = __impl::linked_strtab(secs, sec);
  if ( str == nullptr ) return out;

  u64 at = sec.offset;
  const u64 end = sec.offset + sec.size;
  for ( usize guard = 0; guard < ver_chain_max && at + sizeof(verdef_t) <= end; guard++ ) {
    half flags = 0, ndx = 0, cnt = 0;
    word aux = 0, next = 0;
    {
      const span_t raw = img.at(at, sizeof(verdef_t));
      if ( raw.len < sizeof(verdef_t) ) break;
      flags = rd<half>(raw.ptr, __builtin_offsetof(verdef_t, flags), img.hdr.data);
      ndx = rd<half>(raw.ptr, __builtin_offsetof(verdef_t, ndx), img.hdr.data);
      cnt = rd<half>(raw.ptr, __builtin_offsetof(verdef_t, cnt), img.hdr.data);
      aux = rd<word>(raw.ptr, __builtin_offsetof(verdef_t, aux), img.hdr.data);
      next = rd<word>(raw.ptr, __builtin_offsetof(verdef_t, next), img.hdr.data);
    }

    if ( cnt != 0 && aux != 0 && at + aux + sizeof(verdaux_t) <= end ) {
      const span_t raw = img.at(at + aux, sizeof(verdaux_t));
      if ( raw.len >= sizeof(verdaux_t) ) {
        const word name_off = rd<word>(raw.ptr, __builtin_offsetof(verdaux_t, name), img.hdr.data);
        version_row r{};
        r.index = elf_ver_ndx(ndx);
        r.flags = flags;
        r.is_need = false;
        if ( name_off < str->size ) r.name = read_cstr_at(*img.src, str->offset + name_off, str->size - name_off);
        out.push_back(micron::move(r));
      }
    }

    if ( next == 0 ) break;
    at += next;
  }
  return out;
}

inline micron::vector<version_row>
read_verneed(const image &img, const micron::vector<section_row> &secs, const section_row &sec)
{
  micron::vector<version_row> out{};
  if ( sec.type != sht_gnu_verneed || img.src == nullptr ) return out;
  const section_row *str = __impl::linked_strtab(secs, sec);
  if ( str == nullptr ) return out;

  const auto name_at = [&](word off) {
    micron::string s{};
    if ( off < str->size ) s = read_cstr_at(*img.src, str->offset + off, str->size - off);
    return s;
  };

  u64 at = sec.offset;
  const u64 end = sec.offset + sec.size;
  for ( usize guard = 0; guard < ver_chain_max && at + sizeof(verneed_t) <= end; guard++ ) {
    half cnt = 0;
    word file = 0, aux = 0, next = 0;
    {
      const span_t raw = img.at(at, sizeof(verneed_t));
      if ( raw.len < sizeof(verneed_t) ) break;
      cnt = rd<half>(raw.ptr, __builtin_offsetof(verneed_t, cnt), img.hdr.data);
      file = rd<word>(raw.ptr, __builtin_offsetof(verneed_t, file), img.hdr.data);
      aux = rd<word>(raw.ptr, __builtin_offsetof(verneed_t, aux), img.hdr.data);
      next = rd<word>(raw.ptr, __builtin_offsetof(verneed_t, next), img.hdr.data);
    }
    const micron::string from = name_at(file);

    u64 a = at + aux;
    for ( half k = 0; k < cnt && a + sizeof(vernaux_t) <= end; k++ ) {
      const span_t raw = img.at(a, sizeof(vernaux_t));
      if ( raw.len < sizeof(vernaux_t) ) break;
      const half flags = rd<half>(raw.ptr, __builtin_offsetof(vernaux_t, flags), img.hdr.data);
      const half other = rd<half>(raw.ptr, __builtin_offsetof(vernaux_t, other), img.hdr.data);
      const word name = rd<word>(raw.ptr, __builtin_offsetof(vernaux_t, name), img.hdr.data);
      const word anext = rd<word>(raw.ptr, __builtin_offsetof(vernaux_t, next), img.hdr.data);

      version_row r{};
      r.index = elf_ver_ndx(other);
      r.flags = flags;
      r.is_need = true;
      r.name = name_at(name);
      r.file = from;
      out.push_back(micron::move(r));

      if ( anext == 0 ) break;
      a += anext;
    }

    if ( next == 0 ) break;
    at += next;
  }
  return out;
}

inline const version_row *
version_of(const micron::vector<version_row> &rows, half versym)
{
  const half idx = elf_ver_ndx(versym);
  if ( idx == ver_ndx_local || idx == ver_ndx_global ) return nullptr;
  for ( usize i = 0; i < rows.size(); i++ )
    if ( rows[i].index == idx ) return &rows[i];
  return nullptr;
}

};      // namespace read
};      // namespace elf
};      // namespace micron
