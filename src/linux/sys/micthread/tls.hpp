//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../atomic/atomic.hpp"
#include "../../../bits/__arch.hpp"
#include "../../../memory/mman.hpp"
#include "../../../types.hpp"

#include "../../elf/auxv.hpp"

#include "../../io/sys.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// thread local storage

namespace micron
{

// main thread TLS block
// (tp -> clone_args.tls) installed via CLONE_SETTLS before thread execs
struct __tls_frame {
  byte *base = nullptr;      // page-aligned
  usize size = 0;            // page-rounded in bytes
  byte *tp = nullptr;        // thread pointer (TCB start) [%fs/%gs/TPIDR]
  u64 image_size = 0;        // PT_TLS memsz rounded to p_align
};

struct __tls_template_t {
  const byte *image = nullptr;      // .tdata source
  u64 filesz = 0;                   // initialised bytes to copy
  u64 memsz = 0;                    // total tls size (filesz + tbss)
  u64 align = 0;                    // PT_TLS p_align
  usize pagesz = 4096;
  bool valid = false;
};

inline __tls_template_t __micron_tls_template{};

inline atomic_token<u32> __micron_tls_template_state{ 0 };
inline constexpr u32 __tls_tmpl_uninit = 0;
inline constexpr u32 __tls_tmpl_capturing = 1;
inline constexpr u32 __tls_tmpl_ready = 2;

inline constexpr usize __micron_tls_min_align = 16;      // sysv-amd64 baseline
inline constexpr usize __micron_tcb_sz = 64;             // variant II TCB: self-ptr@0x00, canary@0x28, pguard@0x30
inline constexpr usize __arm_tcbhead_sz = 8;             // arm32 tcbhead (dtv + priv)
inline constexpr usize __arm64_tcbhead_sz = 16;          // arm64 tcbhead

#if defined(__micron_arch_amd64)
inline constexpr int __micron_arch_set_fs = 0x1002;
#endif

inline __attribute__((always_inline)) constexpr u64
__tls_round_up(u64 v, u64 a) noexcept
{
  return (v + (a - 1)) & ~(a - 1);
}

inline byte *
__tls_raw_mmap(usize bytes) noexcept
{
  byte *p = micron::bytemap(bytes);
  return micron::mmap_failed(p) ? nullptr : p;
}

inline bool
__tls_capture_from_proc_auxv() noexcept
{
  // WARNING: auxv is usually small, but this _might_ overflow, be careful!
  alignas(16) unsigned long buf[512];
  i32 fd = micron::posix::open("/proc/self/auxv", micron::posix::o_rdonly);
  if ( fd < 0 ) return false;
  usize total = 0;
  for ( ;; ) {
    long n = micron::posix::read(fd, reinterpret_cast<char *>(buf) + total, sizeof(buf) - total);
    if ( n <= 0 ) break;
    total += static_cast<usize>(n);
    if ( total >= sizeof(buf) ) break;
  }
  micron::posix::close(fd);

  const usize pairs = total / (2 * sizeof(unsigned long));
  unsigned long phdr_addr = 0, phent = 0, phnum = 0, pagesz = 0;
  for ( usize i = 0; i < pairs; ++i ) {
    unsigned long t = buf[2 * i], v = buf[2 * i + 1];
    if ( t == at_null ) break;
    if ( t == at_phdr )
      phdr_addr = v;
    else if ( t == at_phent )
      phent = v;
    else if ( t == at_phnum )
      phnum = v;
    else if ( t == at_pagesz )
      pagesz = v;
  }
  if ( phdr_addr == 0 || phent == 0 || phnum == 0 ) return false;

  const tls_image img = find_tls_in_phdrs(phdr_addr, phent, phnum);
  __micron_tls_template
      = __tls_template_t{ img.image, img.filesz, img.memsz, img.align ? img.align : __micron_tls_min_align, pagesz ? pagesz : 4096, true };
  return true;
}

// called by start/__tls in freestanding mode)
inline void
__tls_capture_template(const byte *image, u64 filesz, u64 memsz, u64 align, usize pagesz) noexcept
{
  __micron_tls_template = __tls_template_t{ image, filesz, memsz, align ? align : __micron_tls_min_align, pagesz ? pagesz : 4096, true };
  atom::store(__micron_tls_template_state.ptr(), __tls_tmpl_ready, static_cast<int>(memory_order_release));
}

inline bool
__tls_ensure_template() noexcept
{
  if ( atom::load(__micron_tls_template_state.ptr(), static_cast<int>(memory_order_acquire)) == __tls_tmpl_ready ) return true;
  u32 expected = __tls_tmpl_uninit;
  if ( __micron_tls_template_state.compare_exchange_strong(expected, __tls_tmpl_capturing, memory_order_acq_rel, memory_order_acquire) ) {
    const bool ok = __micron_tls_template.valid ? true : __tls_capture_from_proc_auxv();
    atom::store(__micron_tls_template_state.ptr(), ok ? __tls_tmpl_ready : __tls_tmpl_uninit, static_cast<int>(memory_order_release));
    return ok;
  }
  // capture is a tiny /proc read; a brief spin is fine (contention only on the very first spawns)
  while ( atom::load(__micron_tls_template_state.ptr(), static_cast<int>(memory_order_acquire)) == __tls_tmpl_capturing ) {
  }
  return atom::load(__micron_tls_template_state.ptr(), static_cast<int>(memory_order_acquire)) == __tls_tmpl_ready;
}

// this doesnt install the TP (CLONE_SETTLS in clone3 will)
inline __tls_frame
__tls_make_frame(const byte *image, u64 filesz, u64 memsz, u64 align, usize page_sz) noexcept
{
  __tls_frame f{};
  if ( page_sz == 0 ) page_sz = 4096;
  const u64 p_align = align ? align : __micron_tls_min_align;
  if ( p_align > page_sz ) return f;      // bad align
  const u64 block = __tls_round_up(memsz, p_align);

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  // Variant II: [ image_block | TCB ]
  // tp = base + block
  const u64 alloc = __tls_round_up(block + __micron_tcb_sz, page_sz);
  byte *base = __tls_raw_mmap(static_cast<usize>(alloc));
  if ( !base ) return f;
  byte *tp = base + block;
  for ( u64 i = 0; i < filesz; ++i ) base[i] = image[i];
  *reinterpret_cast<void **>(tp) = tp;      // TCB self-pointer (%fs:0 == tp)
  f = __tls_frame{ base, static_cast<usize>(alloc), tp, block };
#elif defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
  // Variant I: [ TCB | pad | image_block ]
  // tp = base; no self-pointer
  // WARNING: armv7/armv8+ exec model resolves a thread_local at PT_TLS offset to read_tp + round_up(tcbhead, p_align) + o,
  // the image __MUST__ start at p_align past the TCB, not at tcbhead
  const u64 head = (sizeof(void *) == 8) ? __arm64_tcbhead_sz : __arm_tcbhead_sz;
  const u64 head_aligned = __tls_round_up(head, p_align);
  const u64 alloc = __tls_round_up(head_aligned + block, page_sz);
  byte *base = __tls_raw_mmap(static_cast<usize>(alloc));
  if ( !base ) return f;
  byte *image_dst = base + head_aligned;
  for ( u64 i = 0; i < filesz; ++i ) image_dst[i] = image[i];
  f = __tls_frame{ base, static_cast<usize>(alloc), base, block };
#endif
  return f;
}

inline __tls_frame
__tls_make_frame_cached() noexcept
{
  if ( !__tls_ensure_template() ) return __tls_frame{};
  const __tls_template_t &t = __micron_tls_template;
  return __tls_make_frame(t.image, t.filesz, t.memsz, t.align, t.pagesz);
}

// NOTE: if under -fstack-protector-strong the child needs a valid stack canary at %fs:0x28
// (and pointer guard at 0x30) or it faults on the first guarded epilogue
inline void
__tls_seed_tcb_from_current([[maybe_unused]] const __tls_frame &f) noexcept
{
#if defined(__micron_arch_amd64)
  if ( !f.tp ) return;
  unsigned long guard = 0, pguard = 0;
  asm volatile("movq %%fs:0x28, %0" : "=r"(guard));
  asm volatile("movq %%fs:0x30, %0" : "=r"(pguard));
  *reinterpret_cast<unsigned long *>(f.tp + 0x28) = guard;
  *reinterpret_cast<unsigned long *>(f.tp + 0x30) = pguard;
#elif defined(__micron_arch_x86)
  if ( !f.tp ) return;
  unsigned long guard = 0, pguard = 0;
  asm volatile("movl %%gs:0x14, %0" : "=r"(guard));      // i386 glibc stack guard
  asm volatile("movl %%gs:0x18, %0" : "=r"(pguard));
  *reinterpret_cast<unsigned long *>(f.tp + 0x14) = guard;
  *reinterpret_cast<unsigned long *>(f.tp + 0x18) = pguard;
#endif
}

inline __tls_frame
__tls_make_child_frame() noexcept
{
  __tls_frame f = __tls_make_frame_cached();
  if ( f.base ) __tls_seed_tcb_from_current(f);
  return f;
}

inline void
__tls_free_frame(const __tls_frame &f) noexcept
{
  if ( f.base ) micron::munmap(reinterpret_cast<addr_t *>(f.base), f.size);
}

};      // namespace micron
