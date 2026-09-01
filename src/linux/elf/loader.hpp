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
#include "../../mutex/locks.hpp"
#include "../../mutex/mutex.hpp"
#include "../../string/sstring.hpp"
#include "../../syscall.hpp"
#include "../io/sys.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/stat.hpp"
#include "../sys/sysinfo.hpp"

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

inline constexpr usize page_size = __micron_page_size_default;      // compile-time fallback

inline usize
__runtime_page_size() noexcept
{
  return micron::getpagesizelive();
}

inline constexpr usize max_phdrs = 96;

inline usize
__page_floor(usize v) noexcept
{
  const usize ps = __runtime_page_size();
  return v & ~(ps - 1);
}

inline usize
__page_ceil(usize v) noexcept
{
  const usize ps = __runtime_page_size();
  return (v + ps - 1) & ~(ps - 1);
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
  usize relro_start = 0;
  usize relro_len = 0;
  const nphdr_t *phdrs = nullptr;
  half phnum = 0;
  micron::sstring<512> path;      // path the .so was loaded from
  module_t *next = nullptr;

  ~module_t() { reset(); }

  module_t() = default;
  module_t(const module_t &) = delete;
  module_t &operator=(const module_t &) = delete;

  module_t(module_t &&o) noexcept
      : load_base(o.load_base), load_span(o.load_span), dyn(o.dyn), tls_modid(o.tls_modid), relro_start(o.relro_start),
        relro_len(o.relro_len), phdrs(o.phdrs), phnum(o.phnum), path(static_cast<micron::sstring<512> &&>(o.path)), next(o.next)
  {
    o.load_base = nullptr;
    o.load_span = 0;
    o.dyn = {};
    o.tls_modid = 0;
    o.relro_start = 0;
    o.relro_len = 0;
    o.phdrs = nullptr;
    o.phnum = 0;
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
      relro_start = o.relro_start;
      relro_len = o.relro_len;
      phdrs = o.phdrs;
      phnum = o.phnum;
      path = static_cast<micron::sstring<512> &&>(o.path);
      next = o.next;
      o.load_base = nullptr;
      o.load_span = 0;
      o.dyn = {};
      o.tls_modid = 0;
      o.relro_start = 0;
      o.relro_len = 0;
      o.phdrs = nullptr;
      o.phnum = 0;
      o.next = nullptr;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( tls_modid ) tls_unregister(tls_modid);
    if ( load_base && load_span ) {
      micron::munmap(micron::ptr_cast<addr_t *>(load_base), load_span);
      invalidate_host_modules();
    }
    load_base = nullptr;
    load_span = 0;
    dyn = {};
    tls_modid = 0;
    relro_start = 0;
    relro_len = 0;
    phdrs = nullptr;
    phnum = 0;
  }
};

inline module_t *__loaded_modules = nullptr;
inline micron::mutex __loaded_modules_mtx;

inline void *
__resolve_across_loaded(void *user, const char *name, u32 sym_index) noexcept
{
  (void)user;
  (void)sym_index;
  for ( module_t *m = __loaded_modules; m; m = m->next ) {
    const nsym_t *s = lookup_sym(m->dyn, name);
    if ( s && s->shndx != shn_undef ) {
      void *a = reinterpret_cast<void *>(m->load_base + s->value);
      if ( elf_st_type(s->info) == stt_gnu_ifunc ) {
        using ifn = void *(*)();
        a = reinterpret_cast<ifn>(a)();
      }
      return a;
    }
  }
  return host_resolve_sym(name);
}

inline void
__build_dyn_info(dyn_info_t &out, u8 *base, const ndyn_t *dyn) noexcept
{
  for ( const ndyn_t *d = dyn; d->tag != dt_null; ++d ) {
    if ( d->tag == dt_strtab )
      out.strtab = reinterpret_cast<const char *>(base + d->un.ptr);
    else if ( d->tag == dt_strsz )
      out.strsz = d->un.val;
  }

  for ( const ndyn_t *d = dyn; d->tag != dt_null; ++d ) {
    switch ( d->tag ) {
    case dt_symtab:
      out.symtab = micron::ptr_cast<const nsym_t *>(base + d->un.ptr);
      break;
    case dt_syment:
      out.syment = d->un.val;
      break;
    case dt_needed:
      // kept as .dynstr offsets
      if ( out.needed_count < dyn_info_t::max_needed )
        out.needed[out.needed_count++] = static_cast<word>(d->un.val);
      else
        out.needed_truncated = true;
      break;
    case dt_hash:
      out.hash = micron::ptr_cast<const word *>(base + d->un.ptr);
      break;
    case dt_gnu_hash:
      out.gnu_hash = micron::ptr_cast<const word *>(base + d->un.ptr);
      break;
    case dt_rela:
      out.rela = micron::ptr_cast<const nrela_t *>(base + d->un.ptr);
      break;
    case dt_rel:
      out.rel = micron::ptr_cast<const nrel_t *>(base + d->un.ptr);
      break;
    case dt_relsz:
      out.relsz = d->un.val;
      break;
    case dt_relent:
      out.relent = d->un.val;
      break;
    case dt_relcount:
      out.rel_count = d->un.val;
      break;
    case dt_pltgot:
      out.pltgot = micron::ptr_cast<naddr_t *>(base + d->un.ptr);
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
      out.relr = micron::ptr_cast<const nword_t *>(base + d->un.ptr);
      break;
    case dt_relrsz:
      out.relrsz = d->un.val;
      break;
    case dt_jmprel:
      out.jmprel = reinterpret_cast<const void *>(base + d->un.ptr);
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
      out.init_array = micron::ptr_cast<void (*const *)()>(base + d->un.ptr);
      break;
    case dt_init_arraysz:
      out.init_arraysz = d->un.val;
      break;
    case dt_fini_array:
      out.fini_array = micron::ptr_cast<void (*const *)()>(base + d->un.ptr);
      break;
    case dt_fini_arraysz:
      out.fini_arraysz = d->un.val;
      break;
    case dt_preinit_array:
      out.preinit_array = micron::ptr_cast<void (*const *)()>(base + d->un.ptr);
      break;
    case dt_preinit_arraysz:
      out.preinit_arraysz = d->un.val;
      break;
    case dt_versym:
      out.versym = micron::ptr_cast<const half *>(base + d->un.ptr);
      break;
    case dt_verdef:
      out.verdef = micron::ptr_cast<const verdef_t *>(base + d->un.ptr);
      break;
    case dt_verdefnum:
      out.verdefnum = static_cast<word>(d->un.val);
      break;
    case dt_verneed:
      out.verneed = micron::ptr_cast<const verneed_t *>(base + d->un.ptr);
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
  // map only
  defer,
  // RELATIVE/IRELATIVE only
  relative_only,
  // try each and every relocation
  best_effort,
  // as best_effort, but an unresolved non weak symbol fails
  strict,
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// relocation application
// ..DT_RELA / DT_RELASZ      (amd64, arm64)
// ..DT_REL  / DT_RELSZ       (i386, armv7)
// ..DT_JMPREL / DT_PLTRELSZ  either, per DT_PLTREL

inline bool
__reloc_is_relative(u32 t) noexcept
{
#if defined(__micron_arch_amd64)
  return t == r_x86_64_relative || t == r_x86_64_irelative;
#elif defined(__micron_arch_arm64)
  return t == r_aarch64_relative || t == r_aarch64_irelative;
#elif defined(__micron_arch_x86)
  return t == r_386_relative || t == r_386_irelative;
#elif defined(__micron_arch_arm32)
  return t == r_arm_relative || t == r_arm_irelative;
#else
  return false;
#endif
}

inline reloc_ctx_t
__reloc_ctx_for(module_t &m, resolve_fn resolve = nullptr, void *user = nullptr) noexcept
{
  reloc_ctx_t ctx;
  ctx.load_base = m.load_base;
  ctx.d = &m.dyn;
  ctx.resolve = resolve ? resolve : &__resolve_across_loaded;
  ctx.user = user;
  ctx.tls_modid = m.tls_modid;
  ctx.tls_offset = 0;
  return ctx;
}

template<class Rec>
inline bool
__apply_reloc_table(module_t &m, const Rec *tbl, usize count_bytes, reloc_mode_t mode, resolve_fn resolve = nullptr, void *user = nullptr)
{
  if ( !tbl || count_bytes == 0 ) return true;
  const usize n = count_bytes / sizeof(Rec);
  const reloc_ctx_t ctx = __reloc_ctx_for(m, resolve, user);
  for ( usize i = 0; i < n; ++i ) {
    const reloc_view v = reloc_view_of<native_class>(tbl[i]);
    if ( mode == reloc_mode_t::relative_only && !__reloc_is_relative(v.type) ) continue;
    const reloc_result r = apply_reloc(ctx, v);
    if ( r == reloc_result::unsupported ) return false;
    if ( r == reloc_result::unresolved && mode == reloc_mode_t::strict ) return false;
  }
  return true;
}

inline bool
__apply_plt_table(module_t &m, reloc_mode_t mode, resolve_fn resolve = nullptr, void *user = nullptr)
{
  if ( !m.dyn.jmprel || m.dyn.pltrelsz == 0 ) return true;
  if ( m.dyn.pltrel == dt_rela )
    return __apply_reloc_table(m, reinterpret_cast<const nrela_t *>(m.dyn.jmprel), static_cast<usize>(m.dyn.pltrelsz), mode, resolve, user);
  if ( m.dyn.pltrel == dt_rel )
    return __apply_reloc_table(m, reinterpret_cast<const nrel_t *>(m.dyn.jmprel), static_cast<usize>(m.dyn.pltrelsz), mode, resolve, user);
  if constexpr ( arch_uses_rel )
    return __apply_reloc_table(m, reinterpret_cast<const nrel_t *>(m.dyn.jmprel), static_cast<usize>(m.dyn.pltrelsz), mode, resolve, user);
  else
    return __apply_reloc_table(m, reinterpret_cast<const nrela_t *>(m.dyn.jmprel), m.dyn.pltrelsz, mode, resolve, user);
}

inline bool
__apply_all_relocs(module_t &m, reloc_mode_t mode, resolve_fn resolve = nullptr, void *user = nullptr)
{
  if ( mode == reloc_mode_t::defer ) return true;
  if ( !__apply_reloc_table(m, m.dyn.rela, static_cast<usize>(m.dyn.relasz), mode, resolve, user) ) return false;
  if ( !__apply_reloc_table(m, m.dyn.rel, static_cast<usize>(m.dyn.relsz), mode, resolve, user) ) return false;
  return __apply_plt_table(m, mode, resolve, user);
}

inline void
__apply_relr(module_t &m) noexcept
{
  using tr = native_traits;
  using rw = tr::uword;

  if ( !m.dyn.relr || m.dyn.relrsz == 0 ) return;
  const uintptr_t l_addr = reinterpret_cast<uintptr_t>(m.load_base);
  const usize n = static_cast<usize>(m.dyn.relrsz / sizeof(rw));
  uintptr_t *where = nullptr;
  for ( usize k = 0; k < n; ++k ) {
    rw entry = m.dyn.relr[k];
    if ( (entry & 1) == 0 ) {
      where = reinterpret_cast<uintptr_t *>(l_addr + entry);
      *where += l_addr;
      ++where;
    } else {
      for ( usize i = 0; (entry >>= 1) != 0; ++i )
        if ( (entry & 1) != 0 ) where[i] += l_addr;
      where += tr::relr_bits;
    }
  }
}

inline void
__run_initializers(module_t &m) noexcept
{
  // DT_PREINIT_ARRAY is legal exclusively in actual executables
  if ( m.dyn.preinit_array && m.dyn.preinit_arraysz ) {
    const usize n = static_cast<usize>(m.dyn.preinit_arraysz / sizeof(void *));
    for ( usize i = 0; i < n; ++i ) {
      if ( m.dyn.preinit_array[i] ) m.dyn.preinit_array[i]();
    }
  }
  if ( m.dyn.init ) m.dyn.init();
  if ( m.dyn.init_array && m.dyn.init_arraysz ) {
    const usize n = static_cast<usize>(m.dyn.init_arraysz / sizeof(void *));
    for ( usize i = 0; i < n; ++i ) {
      if ( m.dyn.init_array[i] ) m.dyn.init_array[i]();
    }
  }
}

inline void
__run_finalizers(module_t &m) noexcept
{
  if ( m.dyn.fini_array && m.dyn.fini_arraysz ) {
    usize n = static_cast<usize>(m.dyn.fini_arraysz / sizeof(void *));
    while ( n-- ) {
      if ( m.dyn.fini_array[n] ) m.dyn.fini_array[n]();
    }
  }
  if ( m.dyn.fini ) m.dyn.fini();
}

inline void
__record_relro(module_t &m, const nphdr_t *phdrs, half phnum) noexcept
{
  for ( half i = 0; i < phnum; ++i ) {
    if ( phdrs[i].type != pt_gnu_relro ) continue;
    const usize start = __page_floor(phdrs[i].vaddr);
    const usize end = __page_floor(phdrs[i].vaddr + phdrs[i].memsz);
    if ( end <= start ) continue;
    m.relro_start = start;
    m.relro_len = end - start;
    return;
  }
}

inline void
__apply_relro_now(module_t &m) noexcept
{
  if ( !m.relro_len || !m.load_base ) return;
  micron::mprotect(micron::ptr_cast<addr_t *>(m.load_base + m.relro_start), m.relro_len, prot_read);
}

inline void
__apply_relro(const u8 *base, const nphdr_t *phdrs, half phnum) noexcept
{
  for ( half i = 0; i < phnum; ++i ) {
    if ( phdrs[i].type != pt_gnu_relro ) continue;
    const usize start = __page_floor(phdrs[i].vaddr);
    const usize end = __page_ceil(phdrs[i].vaddr + phdrs[i].memsz);
    micron::mprotect(const_cast<addr_t *>(micron::ptr_cast<const addr_t *>(base + start)), end - start, prot_read);
  }
}

struct load_opts_t {
  reloc_mode_t reloc = reloc_mode_t::relative_only;
  bool run_init = false;
  bool apply_relro = true;
  resolve_fn resolve = nullptr;
  void *resolve_user = nullptr;
};

// load a single .so by absolute path
// DOES NOT recursively load DT_NEEDED: host libc/libpthread/etc. are expected to be already mapped by the runtime
// if running in freestanding mode nothing will be loaded, so you need to load it manually (if required)
// TODO: should extend via proper /proc/self/maps walking and loading all depend separately
inline module_t
__load_module_from_path(const char *path, const load_opts_t &opts = {})
{
  i32 fd = posix::open(path, posix::o_rdonly);
  if ( fd < 0 ) exc<except::library_error>("elf: open failed");

  nehdr_t eh;
  if ( posix::pread(fd, &eh, sizeof(eh), 0) != static_cast<max_t>(sizeof(eh)) ) {
    posix::close(fd);
    exc<except::library_error>("elf: short ehdr read");
  }
  if ( eh.ident[ei_mag0] != mag0 || eh.ident[ei_mag1] != mag1 || eh.ident[ei_mag2] != mag2 || eh.ident[ei_mag3] != mag3 ) {
    posix::close(fd);
    exc<except::library_error>("elf: bad magic");
  }
  if ( eh.ident[ei_class] != native_traits::ident_class ) {
    posix::close(fd);
    exc<except::library_error>("elf: wrong ELF class for this target (32/64-bit mismatch)");
  }
  if ( eh.ident[ei_data] != (native_data == fmt_data::msb ? elfdata2msb : elfdata2lsb) ) {
    posix::close(fd);
    exc<except::library_error>("elf: wrong byte order");
  }
  if ( eh.machine != expected_machine ) {
    posix::close(fd);
    exc<except::library_error>("elf: wrong machine");
  }
  if ( eh.type != et_dyn ) {
    posix::close(fd);
    exc<except::library_error>("elf: not a shared object");
  }

  if ( eh.phentsize != sizeof(nphdr_t) || eh.phnum == 0 ) {
    posix::close(fd);
    exc<except::library_error>("elf: bad phdr table");
  }

  nphdr_t phdrs[max_phdrs];
  if ( eh.phnum > max_phdrs ) {
    posix::close(fd);
    exc<except::library_error>("elf: too many phdrs");
  }
  if ( posix::pread(fd, phdrs, sizeof(nphdr_t) * eh.phnum, eh.phoff) != static_cast<max_t>(sizeof(nphdr_t) * eh.phnum) ) {
    posix::close(fd);
    exc<except::library_error>("elf: short phdr read");
  }

  usize vaddr_min = ~usize(0);
  usize vaddr_max = 0;
  const nphdr_t *dyn_ph = nullptr;
  const nphdr_t *tls_ph = nullptr;
  for ( half i = 0; i < eh.phnum; ++i ) {
    if ( phdrs[i].type == pt_load ) {
      if ( phdrs[i].vaddr < vaddr_min ) vaddr_min = phdrs[i].vaddr;
      const usize end = phdrs[i].vaddr + phdrs[i].memsz;
      if ( end < phdrs[i].vaddr ) {      // vaddr + memsz overflow on a crafted/corrupt phdr
        posix::close(fd);
        exc<except::library_error>("elf: PT_LOAD vaddr+memsz overflow");
      }
      if ( end > vaddr_max ) vaddr_max = end;
    } else if ( phdrs[i].type == pt_dynamic ) {
      dyn_ph = &phdrs[i];
    } else if ( phdrs[i].type == pt_tls ) {
      tls_ph = &phdrs[i];
    }
  }
  if ( vaddr_min == ~usize(0) || !dyn_ph ) {
    posix::close(fd);
    exc<except::library_error>("elf: no PT_LOAD / PT_DYNAMIC");
  }
  vaddr_min = __page_floor(vaddr_min);
  vaddr_max = __page_ceil(vaddr_max);
  const usize span = vaddr_max - vaddr_min;

  u8 *base = reinterpret_cast<u8 *>(micron::mmap(nullptr, span, prot_none, map_private | map_anonymous, -1, 0));
  if ( mmap_failed(base) ) {
    posix::close(fd);
    exc<except::library_error>("elf: reserve mmap failed");
  }

  for ( half i = 0; i < eh.phnum; ++i ) {
    if ( phdrs[i].type != pt_load ) continue;
    const usize vstart = __page_floor(phdrs[i].vaddr);
    const usize vend = __page_ceil(phdrs[i].vaddr + phdrs[i].memsz);
    const usize fstart = __page_floor(phdrs[i].offset);

    if constexpr ( native_class == fmt_class::elf32 ) {
      if ( (fstart & 0xfffu) != 0 ) {
        micron::munmap(micron::ptr_cast<addr_t *>(base), span);
        posix::close(fd);
        exc<except::library_error>("elf: PT_LOAD file offset is not a multiple of 4096 (mmap2 cannot express it)");
      }
    }
    const usize segoff = phdrs[i].vaddr - vstart;
    u8 *target = base + (vstart - vaddr_min);

    const i32 prot = __prot_from_phdr(phdrs[i].flags);

    if ( ((phdrs[i].vaddr - phdrs[i].offset) & (__runtime_page_size() - 1)) != 0 ) {
      micron::munmap(micron::ptr_cast<addr_t *>(base), span);
      posix::close(fd);
      exc<except::library_error>("elf: PT_LOAD vaddr/offset not page-congruent (rebuild with max-page-size)");
    }

    if ( phdrs[i].filesz ) {
      const usize file_end = __page_ceil(phdrs[i].filesz + segoff);
      const usize mem_end = __page_ceil(phdrs[i].memsz + segoff);
      u8 *got = reinterpret_cast<u8 *>(
          micron::mmap(micron::ptr_cast<addr_t *>(target), file_end, prot_read | prot_write, map_private | map_fixed, fd, fstart));
      if ( mmap_failed(got) ) {
        micron::munmap(micron::ptr_cast<addr_t *>(base), span);
        posix::close(fd);
        exc<except::library_error>("elf: segment mmap failed");
      }
      if ( mem_end > file_end ) {
        u8 *bgot = reinterpret_cast<u8 *>(micron::mmap(micron::ptr_cast<addr_t *>(target + file_end), mem_end - file_end,
                                                       prot_read | prot_write, map_private | map_anonymous | map_fixed, -1, 0));
        if ( mmap_failed(bgot) ) {
          micron::munmap(micron::ptr_cast<addr_t *>(base), span);
          posix::close(fd);
          exc<except::library_error>("elf: bss tail mmap failed");
        }
      }
      const usize bss_start = segoff + phdrs[i].filesz;
      const usize bss_end = segoff + phdrs[i].memsz;
      if ( bss_end > bss_start ) {
        micron::memset(target + bss_start, byte{ 0 }, bss_end - bss_start);
      }
      micron::mprotect(micron::ptr_cast<addr_t *>(target), mem_end, prot);
    } else {
      u8 *got = reinterpret_cast<u8 *>(
          micron::mmap(micron::ptr_cast<addr_t *>(target), vend - vstart, prot, map_private | map_anonymous | map_fixed, -1, 0));
      if ( mmap_failed(got) ) {
        micron::munmap(micron::ptr_cast<addr_t *>(base), span);
        posix::close(fd);
        exc<except::library_error>("elf: bss mmap failed");
      }
    }
  }

  posix::close(fd);

  module_t m;
  m.load_base = base - vaddr_min;      // bias so load_base + vaddr == actual VA
  m.load_span = span;
  for ( usize i = 0; path[i] && m.path.size() + 1 < m.path.max_size(); ++i ) m.path += path[i];
  m.path.null_term();

  const ndyn_t *dyn = micron::ptr_cast<const ndyn_t *>(m.load_base + dyn_ph->vaddr);
  __build_dyn_info(m.dyn, m.load_base, dyn);

  if ( tls_ph ) {
    m.tls_modid = tls_register(m.load_base + tls_ph->vaddr, tls_ph->filesz, tls_ph->memsz, tls_ph->align);
  }

  __record_relro(m, phdrs, eh.phnum);

  // point at the phdr table inside the mapping itself
  {
    const usize ph_end = static_cast<usize>(eh.phoff) + static_cast<usize>(eh.phnum) * sizeof(nphdr_t);
    for ( half i = 0; i < eh.phnum; ++i ) {
      if ( phdrs[i].type != pt_load ) continue;
      if ( eh.phoff >= phdrs[i].offset && ph_end <= phdrs[i].offset + phdrs[i].filesz ) {
        m.phdrs = micron::ptr_cast<const nphdr_t *>(m.load_base + phdrs[i].vaddr + (eh.phoff - phdrs[i].offset));
        m.phnum = eh.phnum;
        break;
      }
    }
  }

  if ( opts.reloc == reloc_mode_t::defer ) return m;

  micron::lock_guard __ll(__loaded_modules_mtx);      // held through reloc + unlink (line below) until return
  m.next = __loaded_modules;
  __loaded_modules = &m;

  if ( !__apply_all_relocs(m, opts.reloc, opts.resolve, opts.resolve_user) ) {
    __loaded_modules = m.next;      // unlink the partially-relocated module before unwinding
    micron::munmap(micron::ptr_cast<addr_t *>(base), span);
    exc<except::library_error>("elf: unsupported relocation (static TLS / TLSDESC / COPY) — refusing to load module with corrupt TLS");
  }

  __apply_relr(m);

  if ( opts.apply_relro ) __apply_relro_now(m);
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
    exc<except::library_error>("elf: soname not found in search paths");
  }
  return __load_module_from_path(resolved.c_str(), opts);
}

};      // namespace elf
};      // namespace micron
