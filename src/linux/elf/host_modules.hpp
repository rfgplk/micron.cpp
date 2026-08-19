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

using uptr = uintptr_t;

constexpr inline usize host_max_modules = 256;

struct host_module_t {
  u8 *base = nullptr;      // load base (lowest mapped address for this path)
  usize span = 0;          // highest mapped end for this path, minus base
  u64 bias = 0;            // symbol-value addend: base - first_load_vaddr
  const char *soname = nullptr;
  dyn_info_t dyn{};
  micron::sstring<384> path;      // file path as printed in /proc/self/maps
  bool exec = false;              // saw at least one executable mapping for this path
  bool valid = false;
};

inline bool
__host_in_span(const host_module_t &m, uptr p, usize n) noexcept
{
  const uptr lo = reinterpret_cast<uptr>(m.base);
  const uptr hi = lo + static_cast<uptr>(m.span);
  if ( p < lo || p > hi ) return false;
  return n <= static_cast<usize>(hi - p);
}

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

struct maps_line_t {
  u64 start = 0;
  u64 end = 0;
  bool exec = false;
  micron::sstring<384> path;
};

// "7f3c1a200000-7f3c1a228000 r-xp 00002000 08:02 1179656   /usr/lib64/libc.so.6"
//  ^ start      ^ end        ^ perms                       ^ path
inline bool
__parse_maps_line(const char *line, maps_line_t &out) noexcept
{
  const char *p = line;
  out.start = __parse_hex(p);
  if ( *p != '-' ) return false;
  ++p;
  out.end = __parse_hex(p);
  if ( out.end <= out.start ) return false;

  while ( *p == ' ' || *p == '\t' ) ++p;
  const char *perms = p;
  usize pl = 0;
  while ( perms[pl] && perms[pl] != ' ' && perms[pl] != '\t' && perms[pl] != '\n' ) ++pl;
  if ( pl < 4 ) return false;
  out.exec = (perms[2] == 'x');
  p += pl;

  for ( i32 f = 0; f < 3 && *p; ++f ) {      // offset, dev, inode
    while ( *p == ' ' || *p == '\t' ) ++p;
    while ( *p && *p != ' ' && *p != '\t' && *p != '\n' ) ++p;
  }
  while ( *p == ' ' || *p == '\t' ) ++p;
  if ( *p == 0 || *p == '\n' || *p != '/' ) return false;

  out.path.set_size(0);
  while ( *p && *p != '\n' && out.path.size() + 1 < out.path.max_size() ) out.path += *p++;
  out.path.null_term();
  return !out.path.empty();
}

inline void
__build_host_dyn(host_module_t &m)
{
  // a loaded module always has an executable mapping
  if ( !m.exec ) return;
  if ( m.span < sizeof(nehdr_t) ) return;

  volatile const u8 *probe = m.base;
  const u8 b0 = probe[0];
  if ( b0 != mag0 ) return;
  const u8 b1 = probe[1];
  if ( b1 != mag1 ) return;
  const u8 b2 = probe[2];
  if ( b2 != mag2 ) return;
  const u8 b3 = probe[3];
  if ( b3 != mag3 ) return;

  const nehdr_t *eh = reinterpret_cast<const nehdr_t *>(m.base);
  if ( eh->ident[ei_class] != native_traits::ident_class ) return;
  // WARNING: a class check alone is not enough; amd64 and an aarch64 object are both ELFCLASS64 with the same header layout
  if ( expected_machine != 0 && eh->machine != expected_machine ) return;
  if ( eh->ident[ei_data] != (native_data == fmt_data::msb ? elfdata2msb : elfdata2lsb) ) return;
  if ( eh->type != et_dyn && eh->type != et_exec ) return;      // not a loadable image
  if ( eh->phnum == 0 || eh->phnum > 256 ) return;              // sanity bound on the phdr table

  const uptr phdr_at = reinterpret_cast<uptr>(m.base) + static_cast<uptr>(eh->phoff);
  if ( !__host_in_span(m, phdr_at, static_cast<usize>(eh->phnum) * sizeof(nphdr_t)) ) return;

  const nphdr_t *phdrs = reinterpret_cast<const nphdr_t *>(phdr_at);
  const nphdr_t *dyn_ph = nullptr;
  uptr first_load_vaddr = ~uptr(0);
  for ( half i = 0; i < eh->phnum; ++i ) {
    if ( phdrs[i].type == pt_load && static_cast<uptr>(phdrs[i].vaddr) < first_load_vaddr ) {
      first_load_vaddr = static_cast<uptr>(phdrs[i].vaddr);
    }
    if ( phdrs[i].type == pt_dynamic ) dyn_ph = &phdrs[i];
  }
  if ( !dyn_ph || first_load_vaddr == ~uptr(0) ) return;

  const uptr base_u = reinterpret_cast<uptr>(m.base);
  const uptr bias = base_u - first_load_vaddr;
  m.bias = static_cast<u64>(bias);      // store for host_resolve_sym (ET_EXEC / nonzero-vaddr modules need it)

  const uptr dyn_at = bias + static_cast<uptr>(dyn_ph->vaddr);
  if ( !__host_in_span(m, dyn_at, sizeof(ndyn_t)) ) return;

  const ndyn_t *dyn = reinterpret_cast<const ndyn_t *>(dyn_at);
  const uptr span_end = reinterpret_cast<uptr>(m.base) + static_cast<uptr>(m.span);
  auto in_dyn = [&](const ndyn_t *d) { return reinterpret_cast<uptr>(d) + sizeof(ndyn_t) <= span_end; };

  auto resolve = [&](u64 v) -> uptr {
    const uptr uv = static_cast<uptr>(v);
    return uv >= base_u ? uv : (bias + uv);
  };

  for ( const ndyn_t *d = dyn; in_dyn(d) && d->tag != dt_null; ++d ) {
    if ( d->tag == dt_strtab ) {
      const uptr at = resolve(d->un.ptr);
      if ( __host_in_span(m, at, 1) ) m.dyn.strtab = reinterpret_cast<const char *>(at);
    } else if ( d->tag == dt_strsz )
      m.dyn.strsz = d->un.val;
  }

  if ( !m.dyn.strtab ) return;
  // clamp DT_STRSZ to what is actually mapped so a name offset can never leave the image
  {
    const uptr st = reinterpret_cast<uptr>(m.dyn.strtab);
    const u64 avail = static_cast<u64>(span_end - st);
    if ( m.dyn.strsz > avail ) m.dyn.strsz = avail;
  }

  for ( const ndyn_t *d = dyn; in_dyn(d) && d->tag != dt_null; ++d ) {
    const uptr at = resolve(d->un.ptr);
    switch ( d->tag ) {
    case dt_symtab:
      if ( __host_in_span(m, at, sizeof(nsym_t)) ) m.dyn.symtab = reinterpret_cast<const nsym_t *>(at);
      break;
    case dt_hash:
      if ( __host_in_span(m, at, 2 * sizeof(word)) ) m.dyn.hash = reinterpret_cast<const word *>(at);
      break;
    case dt_gnu_hash:
      if ( __host_in_span(m, at, 4 * sizeof(word)) ) m.dyn.gnu_hash = reinterpret_cast<const word *>(at);
      break;
    case dt_versym:
      if ( __host_in_span(m, at, sizeof(half)) ) m.dyn.versym = reinterpret_cast<const half *>(at);
      break;
    case dt_syment:
      m.dyn.syment = d->un.val;
      break;
    case dt_soname:
      if ( d->un.val < m.dyn.strsz ) {
        m.dyn.soname = m.dyn.strtab + d->un.val;
        m.soname = m.dyn.soname;
      }
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

    maps_line_t ln;
    if ( !__parse_maps_line(line, ln) ) continue;

    host_module_t *existing = nullptr;
    for ( usize k = 0; k < __host_module_count; ++k ) {
      if ( __host_modules[k].path == ln.path ) {
        existing = &__host_modules[k];
        break;
      }
    }
    // a module spans several lines (r--p / r-xp / rw-p); union them so span covers the whole image
    if ( existing ) {
      const uptr lo = reinterpret_cast<uptr>(existing->base);
      const uptr hi = lo + static_cast<uptr>(existing->span);
      const uptr nlo = static_cast<uptr>(ln.start) < lo ? static_cast<uptr>(ln.start) : lo;
      const uptr nhi = static_cast<uptr>(ln.end) > hi ? static_cast<uptr>(ln.end) : hi;
      existing->base = reinterpret_cast<u8 *>(nlo);
      existing->span = static_cast<usize>(nhi - nlo);
      existing->exec = existing->exec || ln.exec;
      continue;
    }
    if ( __host_module_count >= host_max_modules ) continue;

    host_module_t &m = __host_modules[__host_module_count++];
    // NOTE: via uptr, never straight off the u64 -- a bare cast truncates/sign-extends on i386/arm32
    m.base = reinterpret_cast<u8 *>(static_cast<uptr>(ln.start));
    m.span = static_cast<usize>(ln.end - ln.start);
    m.exec = ln.exec;
    m.path = ln.path;
  }

  micron::munmap(reinterpret_cast<addr_t *>(buf), host_scratch_size);

  for ( usize k = 0; k < __host_module_count; ++k ) {
    __build_host_dyn(__host_modules[k]);
  }
  __host_initialized = true;      // publish only after the table is fully built (was set too early -> empty-table race)
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
    const nsym_t *s = lookup_sym(m.dyn, name);
    if ( s && s->shndx != shn_undef ) {
      void *a = reinterpret_cast<void *>(static_cast<uptr>(m.bias) + static_cast<uptr>(s->value));
      if ( elf_st_type(s->info) == stt_gnu_ifunc ) {
        using ifn = void *(*)();
        a = reinterpret_cast<ifn>(a)();
      }
      return a;
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

inline void
invalidate_host_modules() noexcept
{
  for ( usize k = 0; k < __host_module_count; ++k ) __host_modules[k] = host_module_t{};
  __host_module_count = 0;
  __host_initialized = false;
}

};      // namespace elf
};      // namespace micron
