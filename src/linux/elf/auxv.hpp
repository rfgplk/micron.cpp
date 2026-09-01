//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "consts.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// process auxv prims
//
//
// these are used to inspect the running image program headers (not of arbitrary elfs)

// the ELF header of the running image, handed by the linker
// WARNING: weak on purpose
// NOTE: src/bits/eh/find_fde.hpp and src/bits/eh/unwind_ehabi.hpp declare the same symbol without the weak; they are reachable only from
// start/eh_runtime.cpp, whose include closure does not contain this header
extern "C" const byte __ehdr_start[] __attribute__((weak, visibility("hidden")));

namespace micron
{

struct auxv_t {
  unsigned long a_type;
  unsigned long a_val;
};

// generic / sysv-abi
constexpr static const unsigned long at_null = 0;           // end of vector
constexpr static const unsigned long at_ignore = 1;         // entry should be ignored
constexpr static const unsigned long at_execfd = 2;         // file descriptor of program
constexpr static const unsigned long at_phdr = 3;           // base of program header table
constexpr static const unsigned long at_phent = 4;          // size of one phdr entry
constexpr static const unsigned long at_phnum = 5;          // count of phdr entries
constexpr static const unsigned long at_pagesz = 6;         // page size in bytes
constexpr static const unsigned long at_base = 7;           // base of dynamic linker (ld.so)
constexpr static const unsigned long at_flags = 8;          // process flags (always 0 on linux)
constexpr static const unsigned long at_entry = 9;          // program entry point (e_entry)
constexpr static const unsigned long at_notelf = 10;        // program is a.out (legacy non-zero)
constexpr static const unsigned long at_uid = 11;           // real user id
constexpr static const unsigned long at_euid = 12;          // effective user id
constexpr static const unsigned long at_gid = 13;           // real group id
constexpr static const unsigned long at_egid = 14;          // effective group id
constexpr static const unsigned long at_platform = 15;      // pointer to cpu-id string
constexpr static const unsigned long at_hwcap = 16;         // architecture cpu feature mask
constexpr static const unsigned long at_clktck = 17;        // ticks per second for times(2)

// 18..22 reserved by linux

constexpr static const unsigned long at_secure = 23;                 // AT_SECURE: setuid-ish exec
constexpr static const unsigned long at_base_platform = 24;          // pointer to real-cpu string
constexpr static const unsigned long at_random = 25;                 // pointer to 16 random bytes
constexpr static const unsigned long at_hwcap2 = 26;                 // extension of at_hwcap
constexpr static const unsigned long at_rseq_feature_size = 27;      // rseq supported feature size
constexpr static const unsigned long at_rseq_align = 28;             // rseq allocation alignment
constexpr static const unsigned long at_hwcap3 = 29;                 // extension #2 of at_hwcap
constexpr static const unsigned long at_hwcap4 = 30;                 // extension #3 of at_hwcap

constexpr static const unsigned long at_execfn = 31;      // pointer to argv[0] full path

// arch-specific (x86 / x86-64)
constexpr static const unsigned long at_sysinfo = 32;           // x86: vsyscall fast call entry
constexpr static const unsigned long at_sysinfo_ehdr = 33;      // base of vDSO ELF image

// cache geometry hints (powerpc / aarch64 publish these; x86 generally does not)
constexpr static const unsigned long at_l1i_cachesize = 40;
constexpr static const unsigned long at_l1i_cachegeometry = 41;
constexpr static const unsigned long at_l1d_cachesize = 42;
constexpr static const unsigned long at_l1d_cachegeometry = 43;
constexpr static const unsigned long at_l2_cachesize = 44;
constexpr static const unsigned long at_l2_cachegeometry = 45;
constexpr static const unsigned long at_l3_cachesize = 46;
constexpr static const unsigned long at_l3_cachegeometry = 47;

constexpr static const unsigned long at_minsigstksz = 51;      // minimum stack size for signal delivery

#if defined(__micron_arch_amd64) || (__SIZEOF_POINTER__ == 8)
struct phdr_t {
  u32 p_type;
  u32 p_flags;
  u64 p_offset;
  u64 p_vaddr;
  u64 p_paddr;
  u64 p_filesz;
  u64 p_memsz;
  u64 p_align;
};
#else
struct phdr_t {
  u32 p_type;
  u32 p_offset;
  u32 p_vaddr;
  u32 p_paddr;
  u32 p_filesz;
  u32 p_memsz;
  u32 p_flags;
  u32 p_align;
};
#endif

inline __attribute__((always_inline)) unsigned long
__auxv_lookup(const auxv_t *av, unsigned long type) noexcept
{
  for ( ; av->a_type != at_null; ++av ) {
    if ( av->a_type == type ) return av->a_val;
  }
  return 0;
}

template<typename T = void>
inline __attribute__((always_inline)) T *
__auxv_lookup_ptr(const auxv_t *av, unsigned long type) noexcept
{
  return reinterpret_cast<T *>(__auxv_lookup(av, type));
}

inline __attribute__((always_inline)) bool
__auxv_has(const auxv_t *av, unsigned long type) noexcept
{
  for ( ; av->a_type != at_null; ++av ) {
    if ( av->a_type == type ) return true;
  }
  return false;
}

struct tls_image {
  const byte *image;      // base of the .tdata template (ph->p_vaddr + bias)
  u64 filesz;             // bytes of initialised tdata
  u64 memsz;              // total tls block size (filesz + tbss)
  u64 align;              // p_align from PT_TLS (0 if no PT_TLS)
};

// computes the PIE / static-pie load bias from PT_PHDR (0 for ET_EXEC) so the returned image pointer is the runtime address of the .tdata
// template under -pie/-static-pie the template would be read from the link-time vaddr
//
// WARNING: this cannot compute a bias when the image has no PT_PHDR, and the bias is not recoverable from these three arguments alone
// ld emits PT_PHDR for a dynamic PIE and for anything with a PT_INTERP, but not for static ET_EXECs (bias is genuinely 0)
// but _not_ for --static-pie (NOT HARMLESS: bias stays 0 yet out.image is the link-time vaddr)
inline __attribute__((always_inline)) tls_image
find_tls_in_phdrs(unsigned long phdr_addr, unsigned long phent, unsigned long phnum) noexcept
{
  tls_image out{ nullptr, 0, 0, 0 };
  if ( phdr_addr == 0 || phent == 0 || phnum == 0 ) return out;

  const byte *p = reinterpret_cast<const byte *>(phdr_addr);

  u64 bias = 0;
  for ( unsigned long i = 0; i < phnum; ++i ) {
    const phdr_t *ph = micron::ptr_cast<const phdr_t *>(p + i * phent);
    if ( ph->p_type == elf::pt_phdr ) {
      bias = static_cast<u64>(phdr_addr) - static_cast<u64>(ph->p_vaddr);
      break;
    }
  }

  for ( unsigned long i = 0; i < phnum; ++i ) {
    const phdr_t *ph = micron::ptr_cast<const phdr_t *>(p + i * phent);
    if ( ph->p_type == elf::pt_tls ) {
      out.image = reinterpret_cast<const byte *>(static_cast<u64>(ph->p_vaddr) + bias);
      out.filesz = ph->p_filesz;
      out.memsz = ph->p_memsz;
      out.align = ph->p_align;
      return out;
    }
  }
  return out;
}

inline __attribute__((always_inline)) tls_image
__auxv_find_tls(const auxv_t *av) noexcept
{
  return find_tls_in_phdrs(__auxv_lookup(av, at_phdr), __auxv_lookup(av, at_phent), __auxv_lookup(av, at_phnum));
}

#if defined(__micron_arch_amd64) || (__SIZEOF_POINTER__ == 8)
struct ehdr_t {
  byte e_ident[16];
  u16 e_type;
  u16 e_machine;
  u32 e_version;
  u64 e_entry;
  u64 e_phoff;
  u64 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};
#else
struct ehdr_t {
  byte e_ident[16];
  u16 e_type;
  u16 e_machine;
  u32 e_version;
  u32 e_entry;
  u32 e_phoff;
  u32 e_shoff;
  u32 e_flags;
  u16 e_ehsize;
  u16 e_phentsize;
  u16 e_phnum;
  u16 e_shentsize;
  u16 e_shnum;
  u16 e_shstrndx;
};
#endif

// true when the linker actually gave us a usable ELF header for the running image
inline bool
__ehdr_usable(void) noexcept
{
  if ( __ehdr_start == nullptr ) return false;
  const ehdr_t *eh = micron::ptr_cast<const ehdr_t *>(__ehdr_start);
  if ( eh->e_ident[0] != 0x7f or eh->e_ident[1] != 'E' or eh->e_ident[2] != 'L' or eh->e_ident[3] != 'F' ) return false;
  return eh->e_phoff != 0 and eh->e_phentsize != 0 and eh->e_phnum != 0;
}

// WARNING: the load bias is not derived from PT_PHDR alone
inline tls_image
find_tls_from_ehdr(void) noexcept
{
  tls_image out{ nullptr, 0, 0, 0 };
  if ( !__ehdr_usable() ) return out;

  const ehdr_t *eh = micron::ptr_cast<const ehdr_t *>(__ehdr_start);
  const byte *p = __ehdr_start + eh->e_phoff;

  u64 bias = 0;
  bool have_bias = false;
  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const phdr_t *ph = micron::ptr_cast<const phdr_t *>(p + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == elf::pt_phdr ) {
      bias = reinterpret_cast<u64>(p) - static_cast<u64>(ph->p_vaddr);
      have_bias = true;
      break;
    }
  }
  if ( !have_bias ) {
    for ( u16 i = 0; i < eh->e_phnum; ++i ) {
      const phdr_t *ph = micron::ptr_cast<const phdr_t *>(p + static_cast<usize>(i) * eh->e_phentsize);
      if ( ph->p_type == elf::pt_load and ph->p_offset == 0 ) {
        bias = reinterpret_cast<u64>(__ehdr_start) - static_cast<u64>(ph->p_vaddr);
        break;
      }
    }
  }

  for ( u16 i = 0; i < eh->e_phnum; ++i ) {
    const phdr_t *ph = micron::ptr_cast<const phdr_t *>(p + static_cast<usize>(i) * eh->e_phentsize);
    if ( ph->p_type == elf::pt_tls ) {
      out.image = reinterpret_cast<const byte *>(static_cast<u64>(ph->p_vaddr) + bias);
      out.filesz = ph->p_filesz;
      out.memsz = ph->p_memsz;
      out.align = ph->p_align;
      return out;
    }
  }
  return out;
}

};      // namespace micron
