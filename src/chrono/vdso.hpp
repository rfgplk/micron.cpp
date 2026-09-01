//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../linux/sys/time.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vDSO clock
//
// WARNING: OFF BY DEFAULT; define MICRON_CHRONO_VDSO to enable

#include "../linux/elf/auxval.hpp"
#include "../linux/elf/consts.hpp"
#include "../linux/elf/hash.hpp"
#include "../linux/elf/header.hpp"

namespace micron
{
namespace chrono
{
namespace vdso
{

using clock_gettime_fn = int (*)(clockid_t, timespec_t *);
using getcpu_fn = int (*)(unsigned *, unsigned *, void *);

inline constexpr bool supported = true;

struct resolved {
  clock_gettime_fn clock_gettime = nullptr;
  getcpu_fn getcpu = nullptr;
  bool tried = false;
};

namespace __impl
{

inline resolved __slot{};
inline u32 __state = 0;

// WARNING: entry point has to match micron's timespec_t, for 32b targets { time64_t tv_sec; i64 tv_nsec; }
inline constexpr const char *__cgt_names[] = {
#if defined(__micron_arch_width_32)
  "__vdso_clock_gettime64",
  "__kernel_clock_gettime64",
#elif defined(__micron_arch_arm64)
  "__kernel_clock_gettime",
  "__vdso_clock_gettime",
#else
  "__vdso_clock_gettime",
  "__kernel_clock_gettime",
#endif
};

// getcpu amd64/arm64 exclusive
inline constexpr const char *__getcpu_names[] = {
#if defined(__micron_arch_arm64)
  "__kernel_getcpu",
  "__vdso_getcpu",
#elif defined(__micron_arch_amd64)
  "__vdso_getcpu",
  "__kernel_getcpu",
#else
  "",
#endif
};

struct scan_out {
  elf::dyn_info_t dyn{};
  const u8 *bias = nullptr;
  bool ok = false;
};

[[gnu::cold, gnu::noinline]] inline scan_out
__scan(unsigned long base_addr) noexcept
{
  scan_out o{};
  if ( base_addr == 0 ) return o;
  const u8 *base = reinterpret_cast<const u8 *>(base_addr);

  const elf::nehdr_t *eh = micron::ptr_cast<const elf::nehdr_t *>(base);
  if ( eh->ident[0] != 0x7f || eh->ident[1] != 'E' || eh->ident[2] != 'L' || eh->ident[3] != 'F' ) return o;
  if ( eh->phoff == 0 || eh->phentsize == 0 || eh->phnum == 0 ) return o;

  const u8 *ph = base + eh->phoff;
  const elf::ndyn_t *dyn = nullptr;

  for ( elf::half i = 0; i < eh->phnum; ++i ) {
    const elf::nphdr_t *p = micron::ptr_cast<const elf::nphdr_t *>(ph + static_cast<usize>(i) * eh->phentsize);
    if ( p->type == elf::pt_load && o.bias == nullptr ) {
      o.bias = reinterpret_cast<const u8 *>(base_addr + static_cast<unsigned long>(p->offset) - static_cast<unsigned long>(p->vaddr));
    } else if ( p->type == elf::pt_dynamic ) {
      dyn = micron::ptr_cast<const elf::ndyn_t *>(base + p->offset);
    }
  }
  if ( !dyn || !o.bias ) return o;

  for ( const elf::ndyn_t *d = dyn; d->tag != elf::dt_null; ++d ) {
    switch ( d->tag ) {
    case elf::dt_strtab:
      o.dyn.strtab = reinterpret_cast<const char *>(o.bias + d->un.ptr);
      break;
    case elf::dt_symtab:
      o.dyn.symtab = micron::ptr_cast<const elf::nsym_t *>(o.bias + d->un.ptr);
      break;
    case elf::dt_hash:
      o.dyn.hash = micron::ptr_cast<const elf::word *>(o.bias + d->un.ptr);
      break;
    case elf::dt_gnu_hash:
      o.dyn.gnu_hash = micron::ptr_cast<const elf::word *>(o.bias + d->un.ptr);
      break;
    default:
      break;
    }
  }
  if ( !o.dyn.symtab || !o.dyn.strtab ) return o;
  if ( !o.dyn.hash && !o.dyn.gnu_hash ) return o;
  o.dyn.symcount = elf::count_dynsyms(o.dyn);
  o.ok = true;
  return o;
}

[[gnu::cold, gnu::noinline]] inline void
__resolve(void) noexcept
{
  resolved r{};
  r.tried = true;

  const scan_out o = __scan(micron::getauxval(micron::at_sysinfo_ehdr));
  if ( o.ok ) {
    for ( const char *n : __cgt_names ) {
      const elf::nsym_t *s = elf::lookup_sym(o.dyn, n);
      if ( s && s->value != 0 ) {
        r.clock_gettime = reinterpret_cast<clock_gettime_fn>(const_cast<u8 *>(o.bias) + s->value);
        break;
      }
    }
    for ( const char *n : __getcpu_names ) {
      if ( n[0] == 0 ) break;      // this arch's vDSO exports no getcpu
      const elf::nsym_t *s = elf::lookup_sym(o.dyn, n);
      if ( s && s->value != 0 ) {
        r.getcpu = reinterpret_cast<getcpu_fn>(const_cast<u8 *>(o.bias) + s->value);
        break;
      }
    }
  }

  __slot = r;
  __atomic_store_n(&__state, 1u, __ATOMIC_RELEASE);
}

};      // namespace __impl

inline const resolved &
info(void) noexcept
{
  if ( __atomic_load_n(&__impl::__state, __ATOMIC_ACQUIRE) == 0u ) [[unlikely]]
    __impl::__resolve();
  return __impl::__slot;
}

inline bool
available(void) noexcept
{
  return info().clock_gettime != nullptr;
}

[[gnu::always_inline]] inline ssize_t
clock_gettime(clockid_t clk, timespec_t &ts) noexcept
{
  const clock_gettime_fn f = info().clock_gettime;
  if ( f ) [[likely]]
    return static_cast<ssize_t>(f(clk, &ts));
  return micron::clock_gettime(clk, ts);
}

};      // namespace vdso
};      // namespace chrono
};      // namespace micron
