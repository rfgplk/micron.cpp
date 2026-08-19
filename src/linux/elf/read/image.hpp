//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../consts.hpp"
#include "../header.hpp"
#include "source.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// normalized ELF header

namespace micron
{
namespace elf
{
namespace read
{

struct header {
  fmt_class cls = fmt_class::invalid;
  fmt_data data = fmt_data::invalid;
  u8 osabi = 0;
  u8 abiversion = 0;
  u16 type = 0;
  u16 machine = 0;
  u32 version = 0;
  u64 entry = 0;
  u64 phoff = 0;
  u64 shoff = 0;
  u32 flags = 0;
  u16 ehsize = 0;
  u16 phentsize = 0;
  u16 phnum = 0;
  u16 shentsize = 0;
  u16 shnum = 0;
  u16 shstrndx = 0;
};

struct image {
  const source *src = nullptr;
  header hdr{};

  bool
  ok() const noexcept
  {
    return src != nullptr && (hdr.cls == fmt_class::elf32 || hdr.cls == fmt_class::elf64);
  }

  bool
  is64() const noexcept
  {
    return hdr.cls == fmt_class::elf64;
  }

  bool
  is_native() const noexcept
  {
    return hdr.cls == native_class && hdr.data == native_data && (expected_machine == 0 || hdr.machine == expected_machine);
  }

  span_t
  at(u64 off, u64 want) const noexcept
  {
    return src ? src->at(off, want) : span_t{};
  }
};

inline u64
records_available(const image &img, u64 off, u64 esz) noexcept
{
  if ( esz == 0 || img.src == nullptr ) return 0;
  const u64 len = static_cast<u64>(img.src->size());
  if ( off >= len ) return 0;
  return (len - off) / esz;
}

inline u64
clamp_records(const image &img, u64 off, u64 esz, u64 want) noexcept
{
  const u64 room = records_available(img, off, esz);
  return want < room ? want : room;
}

inline micron::option<image, const char *>
open_image(const source &src) noexcept
{
  using res = micron::option<image, const char *>;

  const span_t ident = src.at(0, ident_size);
  if ( ident.len < ident_size ) return res{ "too short to be an elf file" };

  const u8 *id = ident.ptr;
  if ( id[ei_mag0] != mag0 || id[ei_mag1] != static_cast<u8>(mag1) || id[ei_mag2] != static_cast<u8>(mag2)
       || id[ei_mag3] != static_cast<u8>(mag3) )
    return res{ "bad magic" };

  fmt_class cls = fmt_class::invalid;
  if ( id[ei_class] == elfclass32 )
    cls = fmt_class::elf32;
  else if ( id[ei_class] == elfclass64 )
    cls = fmt_class::elf64;
  else
    return res{ "unknown elf class" };

  fmt_data data = fmt_data::invalid;
  if ( id[ei_data] == elfdata2lsb )
    data = fmt_data::lsb;
  else if ( id[ei_data] == elfdata2msb )
    data = fmt_data::msb;
  else
    return res{ "unknown elf data encoding" };

  if ( id[ei_version] != static_cast<u8>(ev_current) ) return res{ "unsupported elf version" };

  header hdr{};
  hdr.cls = cls;
  hdr.data = data;
  hdr.osabi = id[ei_osabi];
  hdr.abiversion = id[ei_abiversion];

  if ( cls == fmt_class::elf64 ) {
    const span_t raw = src.at(0, sizeof(ehdr_t));
    if ( raw.len < sizeof(ehdr_t) ) return res{ "truncated elf64 header" };
    hdr.type = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, type), data);
    hdr.machine = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, machine), data);
    hdr.version = rd<u32>(raw.ptr, __builtin_offsetof(ehdr_t, version), data);
    hdr.entry = rd<u64>(raw.ptr, __builtin_offsetof(ehdr_t, entry), data);
    hdr.phoff = rd<u64>(raw.ptr, __builtin_offsetof(ehdr_t, phoff), data);
    hdr.shoff = rd<u64>(raw.ptr, __builtin_offsetof(ehdr_t, shoff), data);
    hdr.flags = rd<u32>(raw.ptr, __builtin_offsetof(ehdr_t, flags), data);
    hdr.ehsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, ehsize), data);
    hdr.phentsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, phentsize), data);
    hdr.phnum = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, phnum), data);
    hdr.shentsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, shentsize), data);
    hdr.shnum = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, shnum), data);
    hdr.shstrndx = rd<u16>(raw.ptr, __builtin_offsetof(ehdr_t, shstrndx), data);
  } else {
    const span_t raw = src.at(0, sizeof(ehdr32_t));
    if ( raw.len < sizeof(ehdr32_t) ) return res{ "truncated elf32 header" };
    hdr.type = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, type), data);
    hdr.machine = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, machine), data);
    hdr.version = rd<u32>(raw.ptr, __builtin_offsetof(ehdr32_t, version), data);
    hdr.entry = rd<u32>(raw.ptr, __builtin_offsetof(ehdr32_t, entry), data);
    hdr.phoff = rd<u32>(raw.ptr, __builtin_offsetof(ehdr32_t, phoff), data);
    hdr.shoff = rd<u32>(raw.ptr, __builtin_offsetof(ehdr32_t, shoff), data);
    hdr.flags = rd<u32>(raw.ptr, __builtin_offsetof(ehdr32_t, flags), data);
    hdr.ehsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, ehsize), data);
    hdr.phentsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, phentsize), data);
    hdr.phnum = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, phnum), data);
    hdr.shentsize = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, shentsize), data);
    hdr.shnum = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, shnum), data);
    hdr.shstrndx = rd<u16>(raw.ptr, __builtin_offsetof(ehdr32_t, shstrndx), data);
  }

  image img{};
  img.src = &src;
  img.hdr = hdr;
  return res{ micron::move(img) };
}

};      // namespace read
};      // namespace elf
};      // namespace micron
