//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../memory/cmemory.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/mmap_bits.hpp"
#include "../../string/sstring.hpp"
#include "../../syscall.hpp"
#include "../io/sys.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/stat.hpp"

#include "bits.hpp"
#include "consts.hpp"
#include "hash.hpp"
#include "header.hpp"
#include "host_modules.hpp"
#include "reloc.hpp"
#include "search.hpp"
#include "tls.hpp"

namespace micron
{
namespace elf
{

inline constexpr usize page_size = __micron_page_size_default;
inline constexpr u8 expected_machine
#if defined(__micron_arch_amd64)
    = static_cast<u8>(em_x86_64);
#elif defined(__micron_arch_arm64)
    = static_cast<u8>(em_aarch64);
#else
    = 0;
#endif

constexpr inline usize
__page_floor(usize v) noexcept
{
  return v & ~(page_size - 1);
}

constexpr inline usize
__page_ceil(usize v) noexcept
{
  return (v + page_size - 1) & ~(page_size - 1);
}

constexpr inline i32
__prot_from_phdr(word p_flags) noexcept
{
  i32 p = 0;
  if ( p_flags & pf_r ) p |= prot_read;
  if ( p_flags & pf_w ) p |= prot_write;
  if ( p_flags & pf_x ) p |= prot_exec;
  return p;
}

struct module_t {
  u8 *load_base = nullptr;
  usize load_span = 0;
  dyn_info_t dyn{};
  u64 tls_modid = 0;
  micron::sstring<512> path;      // path the .so was loaded from
  module_t *next = nullptr;

  ~module_t() { reset(); }

  module_t() = default;
  module_t(const module_t &) = delete;
  module_t &operator=(const module_t &) = delete;

  module_t(module_t &&o) noexcept
      : load_base(o.load_base), load_span(o.load_span), dyn(o.dyn), tls_modid(o.tls_modid),
        path(static_cast<micron::sstring<512> &&>(o.path)), next(o.next)
  {
    o.load_base = nullptr;
    o.load_span = 0;
    o.dyn = {};
    o.tls_modid = 0;
    o.next = nullptr;
  }

  module_t &
  operator=(module_t &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      load_base = o.load_base;
      load_span = o.load_span;
      dyn = o.dyn;
      tls_modid = o.tls_modid;
      path = static_cast<micron::sstring<512> &&>(o.path);
      next = o.next;
      o.load_base = nullptr;
      o.load_span = 0;
      o.dyn = {};
      o.tls_modid = 0;
      o.next = nullptr;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( tls_modid ) tls_unregister(tls_modid);
    if ( load_base && load_span ) micron::munmap(reinterpret_cast<addr_t *>(load_base), load_span);
    load_base = nullptr;
    load_span = 0;
    dyn = {};
    tls_modid = 0;
  }
};

inline module_t *__loaded_modules = nullptr;

inline void *
__resolve_across_loaded(void *user, const char *name) noexcept
{
  (void)user;
  for ( module_t *m = __loaded_modules; m; m = m->next ) {
    const sym_t *s = lookup_sym(m->dyn, name);
    if ( s && s->shndx != shn_undef ) {
      return reinterpret_cast<void *>(m->load_base + s->value);
    }
  }
  return host_resolve_sym(name);
}

inline void
__build_dyn_info(dyn_info_t &out, u8 *base, const dyn_t *dyn) noexcept
{
  for ( const dyn_t *d = dyn; d->tag != dt_null; ++d ) {
    if ( d->tag == dt_strtab )
      out.strtab = reinterpret_cast<const char *>(base + d->un.ptr);
    else if ( d->tag == dt_strsz )
      out.strsz = d->un.val;
  }

  for ( const dyn_t *d = dyn; d->tag != dt_null; ++d ) {
    switch ( d->tag ) {
    case dt_symtab:
      out.symtab = reinterpret_cast<const sym_t *>(base + d->un.ptr);
      break;
    case dt_hash:
      out.hash = reinterpret_cast<const word *>(base + d->un.ptr);
      break;
    case dt_gnu_hash:
      out.gnu_hash = reinterpret_cast<const word *>(base + d->un.ptr);
      break;
    case dt_rela:
      out.rela = reinterpret_cast<const rela_t *>(base + d->un.ptr);
      break;
    case dt_relasz:
      out.relasz = d->un.val;
      break;
    case dt_relaent:
      out.relaent = d->un.val;
      break;
    case dt_relacount:
      out.rela_count = d->un.val;
      break;
    case dt_relr:
      out.relr = reinterpret_cast<const xword *>(base + d->un.ptr);
      break;
    case dt_relrsz:
      out.relrsz = d->un.val;
      break;
    case dt_jmprel:
      out.jmprel = reinterpret_cast<const rela_t *>(base + d->un.ptr);
      break;
    case dt_pltrelsz:
      out.pltrelsz = d->un.val;
      break;
    case dt_pltrel:
      out.pltrel = static_cast<sxword>(d->un.val);
      break;
    case dt_init:
      out.init = reinterpret_cast<void (*)()>(base + d->un.ptr);
      break;
    case dt_fini:
      out.fini = reinterpret_cast<void (*)()>(base + d->un.ptr);
      break;
    case dt_init_array:
      out.init_array = reinterpret_cast<void (*const *)()>(base + d->un.ptr);
      break;
    case dt_init_arraysz:
      out.init_arraysz = d->un.val;
      break;
    case dt_fini_array:
      out.fini_array = reinterpret_cast<void (*const *)()>(base + d->un.ptr);
      break;
    case dt_fini_arraysz:
      out.fini_arraysz = d->un.val;
      break;
    case dt_versym:
      out.versym = reinterpret_cast<const half *>(base + d->un.ptr);
      break;
    case dt_verdef:
      out.verdef = reinterpret_cast<const void *>(base + d->un.ptr);
      break;
    case dt_verdefnum:
      out.verdefnum = static_cast<word>(d->un.val);
      break;
    case dt_verneed:
      out.verneed = reinterpret_cast<const void *>(base + d->un.ptr);
      break;
    case dt_verneednum:
      out.verneednum = static_cast<word>(d->un.val);
      break;
    case dt_flags:
      out.flags = d->un.val;
      break;
    case dt_flags_1:
      out.flags1 = d->un.val;
      break;
    case dt_soname:
      if ( out.strtab ) out.soname = out.strtab + d->un.val;
      break;
    case dt_rpath:
      if ( out.strtab ) out.rpath = out.strtab + d->un.val;
      break;
    case dt_runpath:
      if ( out.strtab ) out.runpath = out.strtab + d->un.val;
      break;
    default:
      break;
    }
  }

  out.symcount = count_dynsyms(out);
}

enum class reloc_mode_t : u8 {
  // RELATIVE/IRELATIVE only; leave symbol-bearing relocs alone
  relative_only,
  // try each and every relocation
  best_effort,
};

inline bool
__reloc_is_relative(const rela_t &r) noexcept
{
  const u32 t = elf_r_type(r.info);
#if defined(__micron_arch_amd64)
  return t == r_x86_64_relative || t == r_x86_64_irelative;
#elif defined(__micron_arch_arm64)
  return t == r_aarch64_relative || t == r_aarch64_irelative;
#else
  return false;
#endif
}

inline bool
__apply_rela_table(module_t &m, const rela_t *tbl, usize count_bytes, reloc_mode_t mode)
{
  if ( !tbl || count_bytes == 0 ) return true;
  const usize n = count_bytes / sizeof(rela_t);
  reloc_ctx_t ctx;
  ctx.load_base = m.load_base;
  ctx.d = &m.dyn;
  ctx.resolve = &__resolve_across_loaded;
  ctx.user = nullptr;
  ctx.tls_modid = m.tls_modid;
  ctx.tls_offset = 0;
  for ( usize i = 0; i < n; ++i ) {
    if ( mode == reloc_mode_t::relative_only && !__reloc_is_relative(tbl[i]) ) continue;
    if ( apply_reloc(ctx, tbl[i]) == reloc_result::unsupported ) return false;
  }
  return true;
}

inline void
__apply_relr(module_t &m) noexcept
{
  if ( !m.dyn.relr || m.dyn.relrsz == 0 ) return;
  const uintptr_t l_addr = reinterpret_cast<uintptr_t>(m.load_base);
  const usize n = m.dyn.relrsz / sizeof(xword);
  uintptr_t *where = nullptr;
  for ( usize k = 0; k < n; ++k ) {
    xword entry = m.dyn.relr[k];
    if ( (entry & 1) == 0 ) {
      where = reinterpret_cast<uintptr_t *>(l_addr + entry);
      *where += l_addr;
      ++where;
    } else {
      for ( int i = 0; (entry >>= 1) != 0; ++i )
        if ( (entry & 1) != 0 ) where[i] += l_addr;
      where += (8 * sizeof(xword)) - 1;      // 63 words covered per bitmap entry
    }
  }
}

inline void
__run_initializers(module_t &m) noexcept
{
  if ( m.dyn.init ) m.dyn.init();
  if ( m.dyn.init_array && m.dyn.init_arraysz ) {
    const usize n = m.dyn.init_arraysz / sizeof(void *);
    for ( usize i = 0; i < n; ++i ) {
      if ( m.dyn.init_array[i] ) m.dyn.init_array[i]();
    }
  }
}

inline void
__apply_relro(const u8 *base, const phdr_t *phdrs, half phnum) noexcept
{
  for ( half i = 0; i < phnum; ++i ) {
    if ( phdrs[i].type != pt_gnu_relro ) continue;
    const usize start = __page_floor(phdrs[i].vaddr);
    const usize end = __page_ceil(phdrs[i].vaddr + phdrs[i].memsz);
    micron::mprotect(const_cast<addr_t *>(reinterpret_cast<const addr_t *>(base + start)), end - start, prot_read);
  }
}

struct load_opts_t {
  reloc_mode_t reloc = reloc_mode_t::relative_only;
  bool run_init = false;
};

// load a single .so by absolute path
// DOES NOT recursively load DT_NEEDED: host libc/libpthread/etc. are expected to be already mapped by the runtime
// if running in freestanding mode nothing will be loaded, so you need to load it manually (if required)
// TODO: should extend via proper /proc/self/maps walking and loading all depend separately
inline module_t
__load_module_from_path(const char *path, const load_opts_t &opts = {})
{
  i32 fd = posix::open(path, posix::o_rdonly);
  if ( fd < 0 ) throw except::library_error("elf: open failed");

  ehdr_t eh;
  if ( posix::pread(fd, &eh, sizeof(eh), 0) != static_cast<max_t>(sizeof(eh)) ) {
    posix::close(fd);
    throw except::library_error("elf: short ehdr read");
  }
  if ( eh.ident[ei_mag0] != mag0 || eh.ident[ei_mag1] != mag1 || eh.ident[ei_mag2] != mag2 || eh.ident[ei_mag3] != mag3 ) {
    posix::close(fd);
    throw except::library_error("elf: bad magic");
  }
  if ( eh.ident[ei_class] != elfclass64 || eh.ident[ei_data] != elfdata2lsb || eh.machine != expected_machine ) {
    posix::close(fd);
    throw except::library_error("elf: wrong class/data/machine");
  }
  if ( eh.type != et_dyn ) {
    posix::close(fd);
    throw except::library_error("elf: not a shared object");
  }

  if ( eh.phentsize != sizeof(phdr_t) || eh.phnum == 0 ) {
    posix::close(fd);
    throw except::library_error("elf: bad phdr table");
  }

  phdr_t phdrs[64];
  if ( eh.phnum > 64 ) {
    posix::close(fd);
    throw except::library_error("elf: too many phdrs");
  }
  if ( posix::pread(fd, phdrs, sizeof(phdr_t) * eh.phnum, eh.phoff) != static_cast<max_t>(sizeof(phdr_t) * eh.phnum) ) {
    posix::close(fd);
    throw except::library_error("elf: short phdr read");
  }

  usize vaddr_min = ~usize(0);
  usize vaddr_max = 0;
  const phdr_t *dyn_ph = nullptr;
  const phdr_t *tls_ph = nullptr;
  for ( half i = 0; i < eh.phnum; ++i ) {
    if ( phdrs[i].type == pt_load ) {
      if ( phdrs[i].vaddr < vaddr_min ) vaddr_min = phdrs[i].vaddr;
      const usize end = phdrs[i].vaddr + phdrs[i].memsz;
      if ( end > vaddr_max ) vaddr_max = end;
    } else if ( phdrs[i].type == pt_dynamic ) {
      dyn_ph = &phdrs[i];
    } else if ( phdrs[i].type == pt_tls ) {
      tls_ph = &phdrs[i];
    }
  }
  if ( vaddr_min == ~usize(0) || !dyn_ph ) {
    posix::close(fd);
    throw except::library_error("elf: no PT_LOAD / PT_DYNAMIC");
  }
  vaddr_min = __page_floor(vaddr_min);
  vaddr_max = __page_ceil(vaddr_max);
  const usize span = vaddr_max - vaddr_min;

  u8 *base = reinterpret_cast<u8 *>(micron::mmap(nullptr, span, prot_none, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(base) ) {
    posix::close(fd);
    throw except::library_error("elf: reserve mmap failed");
  }

  for ( half i = 0; i < eh.phnum; ++i ) {
    if ( phdrs[i].type != pt_load ) continue;
    const usize vstart = __page_floor(phdrs[i].vaddr);
    const usize vend = __page_ceil(phdrs[i].vaddr + phdrs[i].memsz);
    const usize fstart = __page_floor(phdrs[i].offset);
    const usize segoff = phdrs[i].vaddr - vstart;
    u8 *target = base + (vstart - vaddr_min);

    const i32 prot = __prot_from_phdr(phdrs[i].flags);

    // ELF requires vaddr ≡ offset (mod p_align), and p_align >= page_size for loadable segments,
    // so the sub-page bias of vaddr and offset must match. Reject a mis-aligned ELF up front
    // rather than silently mapping file data at the wrong address.
    if ( ((phdrs[i].vaddr - phdrs[i].offset) & (page_size - 1)) != 0 ) {
      micron::munmap(reinterpret_cast<addr_t *>(base), span);
      posix::close(fd);
      throw except::library_error("elf: PT_LOAD vaddr/offset not page-congruent (rebuild with max-page-size)");
    }

    if ( phdrs[i].filesz ) {
      const usize file_end = __page_ceil(phdrs[i].filesz + segoff);
      const usize mem_end = __page_ceil(phdrs[i].memsz + segoff);
      u8 *got = reinterpret_cast<u8 *>(
          micron::mmap(reinterpret_cast<addr_t *>(target), file_end, prot_read | prot_write, map_private | map_fixed, fd, fstart));
      if ( mmap_failed(got) ) {
        micron::munmap(reinterpret_cast<addr_t *>(base), span);
        posix::close(fd);
        throw except::library_error("elf: segment mmap failed");
      }
      if ( mem_end > file_end ) {
        u8 *bgot = reinterpret_cast<u8 *>(micron::mmap(reinterpret_cast<addr_t *>(target + file_end), mem_end - file_end,
                                                       prot_read | prot_write, map_private | map_anonymous | map_fixed, -1, 0));
        if ( mmap_failed(bgot) ) {
          micron::munmap(reinterpret_cast<addr_t *>(base), span);
          posix::close(fd);
          throw except::library_error("elf: bss tail mmap failed");
        }
      }
      const usize bss_start = segoff + phdrs[i].filesz;
      const usize bss_end = segoff + phdrs[i].memsz;
      if ( bss_end > bss_start ) {
        micron::memset(target + bss_start, byte{ 0 }, bss_end - bss_start);
      }
      micron::mprotect(reinterpret_cast<addr_t *>(target), mem_end, prot);
    } else {
      u8 *got = reinterpret_cast<u8 *>(
          micron::mmap(reinterpret_cast<addr_t *>(target), vend - vstart, prot, map_private | map_anonymous | map_fixed, -1, 0));
      if ( mmap_failed(got) ) {
        micron::munmap(reinterpret_cast<addr_t *>(base), span);
        posix::close(fd);
        throw except::library_error("elf: bss mmap failed");
      }
    }
  }

  posix::close(fd);

  module_t m;
  m.load_base = base - vaddr_min;      // bias so load_base + vaddr == actual VA
  m.load_span = span;
  for ( usize i = 0; path[i] && m.path.size() + 1 < m.path.max_size(); ++i ) m.path += path[i];
  m.path.null_term();

  const dyn_t *dyn = reinterpret_cast<const dyn_t *>(m.load_base + dyn_ph->vaddr);
  __build_dyn_info(m.dyn, m.load_base, dyn);

  if ( tls_ph ) {
    m.tls_modid = tls_register(m.load_base + tls_ph->vaddr, tls_ph->filesz, tls_ph->memsz, tls_ph->align);
  }

  m.next = __loaded_modules;
  __loaded_modules = &m;

  if ( !__apply_rela_table(m, m.dyn.rela, m.dyn.relasz, opts.reloc) || !__apply_rela_table(m, m.dyn.jmprel, m.dyn.pltrelsz, opts.reloc) ) {
    __loaded_modules = m.next;      // unlink the partially-relocated module before unwinding
    micron::munmap(reinterpret_cast<addr_t *>(base), span);
    throw except::library_error("elf: unsupported relocation (static TLS / TLSDESC / COPY) — refusing to load module with corrupt TLS");
  }

  __apply_relr(m);

  __apply_relro(m.load_base, phdrs, eh.phnum);
  if ( opts.run_init ) __run_initializers(m);

  __loaded_modules = m.next;
  m.next = nullptr;
  return m;
}

inline module_t
load(const char *soname, const char *runpath = nullptr, const load_opts_t &opts = {})
{
  path_str_t resolved = resolve_soname(soname, runpath);
  if ( resolved.empty() ) {
    throw except::library_error("elf: soname not found in search paths");
  }
  return __load_module_from_path(resolved.c_str(), opts);
}

};      // namespace elf
};      // namespace micron
