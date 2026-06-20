//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <micron/bits/__arch.hpp>
#include <micron/exit.hpp>
#include <micron/syscall.hpp>
#include <micron/types.hpp>

#include "__auxv.hpp"

namespace micron
{

#if defined(__micron_arch_amd64)
constexpr static const int __arch_set_fs = 0x1002;      // ARCH_SET_FS
constexpr static const int __arch_get_fs = 0x1003;      // ARCH_GET_FS
#endif

#if defined(__micron_arch_arm32)
// arm tcbhead is exactly two pointers: dtv (4) + private (4)
constexpr static const usize __arm_tcbhead_sz = 8;
#endif

#if defined(__micron_arch_arm64)
// aarch64 tcbhead is two 8-byte pointers: dtv (8) + private (8)
constexpr static const usize __arm64_tcbhead_sz = 16;
#endif

#if defined(__micron_arch_x86)
// i386 GDT TLS via set_thread_area(2)
// flags = seg_32bit(b0) | limit_in_pages(b4) | useable(b6) = 0x51
constexpr static const unsigned int __i386_tls_desc_flags = 0x51u;
struct __i386_user_desc {
  unsigned int entry_number;
  unsigned int base_addr;
  unsigned int limit;
  unsigned int flags;
};
#endif

constexpr static const int __tls_prot_rw = 0x1 | 0x2;          // PROT_READ | PROT_WRITE
constexpr static const int __tls_map_flags = 0x02 | 0x20;      // MAP_PRIVATE | MAP_ANONYMOUS
constexpr static const long __tls_mmap_fd = -1;                // unused with anonymous mappings

constexpr static const usize __micron_tls_default_pagesz = 4096;
constexpr static const usize __micron_tls_min_align = 16;      // sysv-x86_64 baseline TLS alignment
constexpr static const usize __micron_tcb_sz = 64;             // self-ptr + slack for future DTV

struct __tls_frame {
  byte *base = nullptr;      // mmap base (page-aligned)
  usize size = 0;            // total bytes mmaped (page-rounded)
  byte *tp = nullptr;        // thread pointer (TCB start)
  u64 image_size = 0;        // p_memsz rounded up to p_align (block size)
};

inline __tls_frame __micron_main_tls{};

// integer round-up helper limited to powers of two (PT_TLS p_align is always a power of two by ELF spec
inline __attribute__((always_inline)) constexpr u64
__tls_round_up(u64 v, u64 a) noexcept
{
  return (v + (a - 1)) & ~(a - 1);
}

inline __attribute__((always_inline)) byte *
__tls_mmap(usize bytes) noexcept
{
#if defined(__micron_arch_arm32)
  const long ret = micron::syscall(SYS_mmap2, 0UL, bytes, __tls_prot_rw, __tls_map_flags, __tls_mmap_fd, 0L);
#else
  const long ret = micron::syscall(SYS_mmap, 0UL, bytes, __tls_prot_rw, __tls_map_flags, __tls_mmap_fd, 0L);
#endif
  if ( micron::syscall_failed(ret) ) return nullptr;
  return reinterpret_cast<byte *>(ret);
}

inline void
__tls_init([[maybe_unused]] const auxv_t *auxv) noexcept
{
#if defined(__micron_freestanding) && defined(__micron_arch_amd64)
  const tls_image img = __auxv_find_tls(auxv);

  unsigned long page_sz = __auxv_lookup(auxv, at_pagesz);
  if ( page_sz == 0 ) page_sz = __micron_tls_default_pagesz;

  const u64 p_align = (img.align != 0) ? img.align : __micron_tls_min_align;

  if ( p_align > page_sz ) micron::sys_exit(127);

  const u64 image_block_size = __tls_round_up(img.memsz, p_align);

  const u64 raw_size = image_block_size + __micron_tcb_sz;
  const u64 alloc_size = __tls_round_up(raw_size, page_sz);

  byte *base = __tls_mmap(alloc_size);
  if ( base == nullptr ) {
    micron::sys_exit(127);
  }

  byte *tp = base + image_block_size;      // TP / %fs target
  byte *image_dst = base;                  // image starts at the aligned block base

  if ( img.filesz > 0 && img.image != nullptr ) {
    for ( u64 i = 0; i < img.filesz; ++i ) image_dst[i] = img.image[i];
  }

  *reinterpret_cast<void **>(tp) = tp;

  micron::syscall(SYS_arch_prctl, __arch_set_fs, tp);

  __micron_main_tls.base = base;
  __micron_main_tls.size = alloc_size;
  __micron_main_tls.tp = tp;
  __micron_main_tls.image_size = image_block_size;
#elif defined(__micron_freestanding) && defined(__micron_arch_arm32)
  const tls_image img = __auxv_find_tls(auxv);

  unsigned long page_sz = __auxv_lookup(auxv, at_pagesz);
  if ( page_sz == 0 ) page_sz = __micron_tls_default_pagesz;

  const u64 p_align = (img.align != 0) ? img.align : __micron_tls_min_align;
  if ( p_align > page_sz ) micron::sys_exit(127);

  const u64 image_block_size = __tls_round_up(img.memsz, p_align);

  // Variant I
  const u64 head_aligned = __tls_round_up(__arm_tcbhead_sz, p_align);
  const u64 raw_size = head_aligned + image_block_size;
  const u64 alloc_size = __tls_round_up(raw_size, page_sz);

  byte *base = __tls_mmap(alloc_size);
  if ( base == nullptr ) micron::sys_exit(127);

  byte *tp = base;                              // TPIDRURO target
  byte *image_dst = base + head_aligned;        // image lives at TP + round_up(tcbhead, p_align)

  if ( img.filesz > 0 && img.image != nullptr ) {
    for ( u64 i = 0; i < img.filesz; ++i ) image_dst[i] = img.image[i];
  }

  micron::syscall(SYS_arm_set_tls, tp);

  __micron_main_tls.base = base;
  __micron_main_tls.size = alloc_size;
  __micron_main_tls.tp = tp;
  __micron_main_tls.image_size = image_block_size;
#elif defined(__micron_freestanding) && defined(__micron_arch_arm64)
  // Variant I, tcbhead = 16; TPIDR_EL0 is EL0-writable on aarch64
  const tls_image img = __auxv_find_tls(auxv);
  unsigned long page_sz = __auxv_lookup(auxv, at_pagesz);
  if ( page_sz == 0 ) page_sz = __micron_tls_default_pagesz;
  const u64 p_align = (img.align != 0) ? img.align : __micron_tls_min_align;
  if ( p_align > page_sz ) micron::sys_exit(127);
  const u64 image_block_size = __tls_round_up(img.memsz, p_align);
  const u64 head_aligned = __tls_round_up(__arm64_tcbhead_sz, p_align);
  const u64 alloc_size = __tls_round_up(head_aligned + image_block_size, page_sz);
  byte *base = __tls_mmap(alloc_size);
  if ( base == nullptr ) micron::sys_exit(127);
  byte *tp = base;
  byte *image_dst = base + head_aligned;
  if ( img.filesz > 0 && img.image != nullptr ) {
    for ( u64 i = 0; i < img.filesz; ++i ) image_dst[i] = img.image[i];
  }
  asm volatile("msr tpidr_el0, %0" ::"r"(tp) : "memory");
  __micron_main_tls.base = base;
  __micron_main_tls.size = alloc_size;
  __micron_main_tls.tp = tp;
  __micron_main_tls.image_size = image_block_size;
#elif defined(__micron_freestanding) && defined(__micron_arch_x86)
  // Variant II but %gs via set_thread_area(2)
  const tls_image img = __auxv_find_tls(auxv);
  unsigned long page_sz = __auxv_lookup(auxv, at_pagesz);
  if ( page_sz == 0 ) page_sz = __micron_tls_default_pagesz;
  const u64 p_align = (img.align != 0) ? img.align : __micron_tls_min_align;
  if ( p_align > page_sz ) micron::sys_exit(127);
  const u64 image_block_size = __tls_round_up(img.memsz, p_align);
  const u64 alloc_size = __tls_round_up(image_block_size + __micron_tcb_sz, page_sz);
  byte *base = __tls_mmap(alloc_size);
  if ( base == nullptr ) micron::sys_exit(127);
  byte *tp = base + image_block_size;
  byte *image_dst = base;
  if ( img.filesz > 0 && img.image != nullptr ) {
    for ( u64 i = 0; i < img.filesz; ++i ) image_dst[i] = img.image[i];
  }
  *reinterpret_cast<void **>(tp) = tp;      // self-ptr at %gs:0
  __i386_user_desc desc{ 0xffffffffu, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(tp)), 0xfffffu, __i386_tls_desc_flags };
  micron::syscall(SYS_set_thread_area, &desc);
  const unsigned int gs_sel = (desc.entry_number << 3) | 3u;      // kernel wrote the allocated entry_number back
  asm volatile("movw %w0, %%gs" ::"q"(gs_sel));
  __micron_main_tls.base = base;
  __micron_main_tls.size = alloc_size;
  __micron_main_tls.tp = tp;
  __micron_main_tls.image_size = image_block_size;
#elif defined(__micron_freestanding)
#error "micron: no __tls_init() branch for this freestanding architecture. Implement the main-thread TLS setup (ala src/linux/sys/micthread/tls.hpp) or build hosted; a silent empty body would crash on the first thread_local access."
#endif
}

inline void
__tls_destroy() noexcept
{
#if defined(__micron_freestanding) && defined(__micron_arch_amd64)
  if ( __micron_main_tls.base == nullptr ) return;
  micron::syscall(SYS_arch_prctl, __arch_set_fs, 0UL);
  micron::syscall(SYS_munmap, __micron_main_tls.base, __micron_main_tls.size);
  __micron_main_tls = __tls_frame{};
#elif defined(__micron_freestanding) && (defined(__micron_arch_arm32) || defined(__micron_arch_arm64))
  if ( __micron_main_tls.base == nullptr ) return;
  micron::syscall(SYS_munmap, __micron_main_tls.base, __micron_main_tls.size);
  __micron_main_tls = __tls_frame{};
#elif defined(__micron_freestanding) && defined(__micron_arch_x86)
  if ( __micron_main_tls.base == nullptr ) return;
  micron::syscall(SYS_munmap, __micron_main_tls.base, __micron_main_tls.size);
  __micron_main_tls = __tls_frame{};
#elif defined(__micron_freestanding)
#error "micron: no __tls_destroy() branch for this freestanding architecture. Implement the main-thread TLS setup (ala src/linux/sys/micthread/tls.hpp) or build hosted; a silent empty body would crash on the first thread_local access."
#endif
}

};      // namespace micron
