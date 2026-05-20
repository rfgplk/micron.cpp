//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../memory/cstring.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../../string/format.hpp"
#include "../../string/sstring.hpp"
#include "../../syscall.hpp"

#include "../io/sys.hpp"
#include "../sys/fcntl.hpp"

#include "bits.hpp"
#include "consts.hpp"
#include "hash.hpp"
#include "header.hpp"

// TODO: eventually push this out in a full /proc/self/ parser

namespace micron
{
namespace elf
{

constexpr inline usize host_max_modules = 256;

struct host_module_t {
  u8 *base = nullptr;      // load base (lowest mapped address for this path)
  const char *soname = nullptr;
  dyn_info_t dyn{};
  micron::sstring<384> path;      // file path as printed in /proc/self/maps
  bool valid = false;
};

inline host_module_t __host_modules[host_max_modules] = {};
inline usize __host_module_count = 0;
inline bool __host_initialized = false;

constexpr inline usize host_scratch_size = 65536;      // /proc/self/maps cap

inline const char *
__path_basename(const char *p) noexcept
{
  const usize n = micron::strlen(p);
  for ( usize i = n; i-- > 0; ) {
    if ( p[i] == '/' ) return p + i + 1;
  }
  return p;
}

inline u64
__parse_hex(const char *&p) noexcept
{
  u64 v = 0;
  while ( *p ) {
    char c = *p;
    u32 d;
    if ( c >= '0' && c <= '9' )
      d = static_cast<u32>(c - '0');
    else if ( c >= 'a' && c <= 'f' )
      d = static_cast<u32>(c - 'a' + 10);
    else if ( c >= 'A' && c <= 'F' )
      d = static_cast<u32>(c - 'A' + 10);
    else
      break;
    v = (v << 4) | d;
    ++p;
  }
  return v;
}

inline bool
__line_path(const char *line, micron::sstring<384> &out) noexcept
{
  out.set_size(0);
  const char *p = line;
  for ( i32 f = 0; f < 5 && *p; ++f ) {
    while ( *p == ' ' || *p == '\t' ) ++p;
    while ( *p && *p != ' ' && *p != '\t' && *p != '\n' ) ++p;
  }
  while ( *p == ' ' || *p == '\t' ) ++p;
  if ( *p == 0 || *p == '\n' || *p != '/' ) return false;

  while ( *p && *p != '\n' && out.size() + 1 < out.max_size() ) out += *p++;
  out.null_term();
  return !out.empty();
}

inline void
__build_host_dyn(host_module_t &m)
{
  volatile const u8 *probe = m.base;
  const u8 b0 = probe[0];
  if ( b0 != mag0 ) return;
  const u8 b1 = probe[1];
  if ( b1 != mag1 ) return;
  const u8 b2 = probe[2];
  if ( b2 != mag2 ) return;
  const u8 b3 = probe[3];
  if ( b3 != mag3 ) return;

  const ehdr_t *eh = reinterpret_cast<const ehdr_t *>(m.base);
  if ( eh->ident[ei_class] != elfclass64 ) return;

  const phdr_t *phdrs = reinterpret_cast<const phdr_t *>(m.base + eh->phoff);
  const phdr_t *dyn_ph = nullptr;
  u64 first_load_vaddr = ~u64(0);
  for ( half i = 0; i < eh->phnum; ++i ) {
    if ( phdrs[i].type == pt_load && phdrs[i].vaddr < first_load_vaddr ) {
      first_load_vaddr = phdrs[i].vaddr;
    }
    if ( phdrs[i].type == pt_dynamic ) dyn_ph = &phdrs[i];
  }
  if ( !dyn_ph || first_load_vaddr == ~u64(0) ) return;
  const u64 bias = reinterpret_cast<u64>(m.base) - first_load_vaddr;

  const dyn_t *dyn = reinterpret_cast<const dyn_t *>(bias + dyn_ph->vaddr);

  const u64 base_u = reinterpret_cast<u64>(m.base);
  auto resolve = [&](u64 v) -> u64 { return (v >= base_u && v < base_u + (u64(1) << 32)) ? v : (bias + v); };

  for ( const dyn_t *d = dyn; d->tag != dt_null; ++d ) {
    if ( d->tag == dt_strtab )
      m.dyn.strtab = reinterpret_cast<const char *>(resolve(d->un.ptr));
    else if ( d->tag == dt_strsz )
      m.dyn.strsz = d->un.val;
  }

  if ( !m.dyn.strtab ) return;

  for ( const dyn_t *d = dyn; d->tag != dt_null; ++d ) {
    switch ( d->tag ) {
    case dt_symtab:
      m.dyn.symtab = reinterpret_cast<const sym_t *>(resolve(d->un.ptr));
      break;
    case dt_hash:
      m.dyn.hash = reinterpret_cast<const word *>(resolve(d->un.ptr));
      break;
    case dt_gnu_hash:
      m.dyn.gnu_hash = reinterpret_cast<const word *>(resolve(d->un.ptr));
      break;
    case dt_soname:
      m.dyn.soname = m.dyn.strtab + d->un.val;
      m.soname = m.dyn.soname;
      break;
    default:
      break;
    }
  }
  m.dyn.symcount = count_dynsyms(m.dyn);
  m.valid = m.dyn.symtab && (m.dyn.hash || m.dyn.gnu_hash);
}

inline void
init_host_modules()
{
  if ( __host_initialized ) return;
  __host_initialized = true;

  // avoiding new/malloc
  u8 *buf = reinterpret_cast<u8 *>(micron::mmap(nullptr, host_scratch_size, prot_read | prot_write, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(buf) ) return;

  i32 fd = posix::open("/proc/self/maps", posix::o_rdonly);
  if ( fd < 0 ) {
    micron::munmap(reinterpret_cast<addr_t *>(buf), host_scratch_size);
    return;
  }
  usize total = 0;
  while ( total + 1 < host_scratch_size ) {
    max_t n = posix::read(fd, buf + total, host_scratch_size - 1 - total);
    if ( n <= 0 ) break;
    total += static_cast<usize>(n);
  }
  buf[total] = 0;
  posix::close(fd);

  usize i = 0;
  while ( i < total ) {
    usize line_start = i;
    while ( i < total && buf[i] != '\n' ) ++i;
    if ( i >= total ) break;
    buf[i] = 0;
    const char *line = reinterpret_cast<const char *>(buf + line_start);
    ++i;

    micron::sstring<384> path;
    if ( !__line_path(line, path) ) continue;

    const char *cur = line;
    u64 start = __parse_hex(cur);

    host_module_t *existing = nullptr;
    for ( usize k = 0; k < __host_module_count; ++k ) {
      if ( __host_modules[k].path == path ) {
        existing = &__host_modules[k];
        break;
      }
    }
    if ( existing ) {
      if ( start < reinterpret_cast<u64>(existing->base) ) {
        existing->base = reinterpret_cast<u8 *>(start);
      }
      continue;
    }
    if ( __host_module_count >= host_max_modules ) continue;

    host_module_t &m = __host_modules[__host_module_count++];
    m.base = reinterpret_cast<u8 *>(start);
    m.path = path;
  }

  micron::munmap(reinterpret_cast<addr_t *>(buf), host_scratch_size);

  for ( usize k = 0; k < __host_module_count; ++k ) {
    __build_host_dyn(__host_modules[k]);
  }
}

inline const host_module_t *
host_find(const char *name)
{
  if ( !__host_initialized ) init_host_modules();
  if ( !name || !*name ) return nullptr;
  for ( usize k = 0; k < __host_module_count; ++k ) {
    const host_module_t &m = __host_modules[k];
    if ( !m.valid ) continue;
    if ( m.soname && micron::strcmp(m.soname, name) == 0 ) return &m;
    if ( micron::strcmp(__path_basename(m.path.c_str()), name) == 0 ) return &m;
  }
  return nullptr;
}

inline void *
host_resolve_sym(const char *name)
{
  if ( !__host_initialized ) init_host_modules();
  for ( usize k = 0; k < __host_module_count; ++k ) {
    const host_module_t &m = __host_modules[k];
    if ( !m.valid ) continue;
    const sym_t *s = lookup_sym(m.dyn, name);
    if ( s && s->shndx != shn_undef ) {
      return reinterpret_cast<void *>(m.base + s->value);
    }
  }
  return nullptr;
}

inline usize
host_count() noexcept
{
  if ( !__host_initialized ) init_host_modules();
  return __host_module_count;
}

};      // namespace elf
};      // namespace micron
