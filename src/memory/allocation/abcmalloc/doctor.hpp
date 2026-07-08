// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#if defined(ABCMALLOC_DOCTOR_HELP)

#include "../../../atomic/atomic.hpp"
#include "../../../atomic/flag.hpp"
#include "../../../bits/__pause.hpp"
#include "../../../linux/sys/signal.hpp"
#include "../../../syscall.hpp"
#include "../../../types.hpp"

#include "../kmemory.hpp"

#include "config.hpp"
#include "metadata.hpp"
#include "printing.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// abcmalloc doctor mone
//
// activate this to get a fully stateful forensic/debug layer

namespace abc
{
namespace doctor
{

// helpers
inline void
__d(const char *s)
{
  abc::__write(s, micron::strlen(s));
}

inline void
__d_nl(void)
{
  abc::__write("\n", 1);
}

inline void
__d_line(const char *s)
{
  __d(s);
  __d_nl();
}

inline void
__d_u(u64 n)
{
  abc::__print_unsigned(n);
}

inline void
__d_i(i64 n)
{
  abc::__print_signed(n);
}

inline void
__d_ptr(const void *p)
{
  abc::__print_ptr(p);
}

inline void
__d_hex_byte(byte b)
{
  char buf[2];
  const auto nib = [](unsigned v) -> char { return static_cast<char>(v < 10 ? ('0' + v) : ('a' + v - 10)); };
  buf[0] = nib((b >> 4) & 0xF);
  buf[1] = nib(b & 0xF);
  abc::__write(buf, 2);
}

inline void
__d_hex64(u64 v)
{
  char buf[18];
  const auto nib = [](unsigned x) -> char { return static_cast<char>(x < 10 ? ('0' + x) : ('a' + x - 10)); };
  buf[0] = '0';
  buf[1] = 'x';
  for ( int i = 0; i < 16; ++i ) buf[2 + i] = nib(static_cast<unsigned>((v >> (60 - 4 * i)) & 0xF));
  abc::__write(buf, 18);
}

inline void
__dump_row(const void *label, const byte *src, usize n) noexcept
{
  __d("  ");
  __d_ptr(label);
  __d(":  ");
  for ( usize j = 0; j < n; ++j ) {
    __d_hex_byte(src[j]);
    __d(" ");
  }
  __d_nl();
}

// gdb x/Nxb-style raw memory dump, pure hex
inline void
__dump_bytes(const void *addr, usize len)
{
  const byte *p = reinterpret_cast<const byte *>(addr);
  usize i = 0;
  while ( i < len ) {
    const usize row = (len - i < 16) ? (len - i) : 16;
    __dump_row(p + i, p + i, row);
    i += row;
  }
}

inline int __doctor_color_cache = -1;      // -1 unknown, 0 no, 1 yes

inline bool
__doctor_want_color(void) noexcept
{
  if constexpr ( __default_doctor_color == 0 )
    return false;
  else if constexpr ( __default_doctor_color == 1 )
    return true;
  else {
    if ( __doctor_color_cache < 0 ) {

      byte tio[64];
      long r = static_cast<long>(micron::syscall(SYS_ioctl, 2, 0x5401UL, static_cast<void *>(tio)));
      __doctor_color_cache = (r == 0) ? 1 : 0;
    }
    return __doctor_color_cache == 1;
  }
}

inline void
__banner(const char *what)
{
  if ( __doctor_want_color() )
    __d("\033[31mabcmalloc[doctor]\033[0m ");
  else
    __d("abcmalloc[doctor] ");
  __d(what);
}

inline void
__d_bad_begin(void) noexcept
{
  if ( __doctor_want_color() ) __d("\033[31m");
}

inline void
__d_bad_end(void) noexcept
{
  if ( __doctor_want_color() ) __d("\033[0m");
}

// NOTE: must be mmaped, don't reuse abcmalloc
inline void *
__mmap_bytes(usize n) noexcept
{
  addr_t *p = micron::map_normal(nullptr, n);
  if ( reinterpret_cast<uintptr_t>(p) >= ~static_cast<uintptr_t>(0xfff) ) return nullptr;
  return reinterpret_cast<void *>(p);
}

inline void
__munmap_bytes(void *p, usize n) noexcept
{
  if ( p ) micron::munmap(reinterpret_cast<addr_t *>(p), n);
}

inline i32
__gettid(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

constexpr static const usize __doctor_bt_depth = 4;      // frames captured per alloc

[[gnu::noinline]] inline usize
__capture_backtrace(void **out, usize maxn) noexcept
{
  usize n = 0;
  if constexpr ( __default_doctor_backtrace ) {
    void **fp = reinterpret_cast<void **>(__builtin_frame_address(0));
    const uintptr_t lo = reinterpret_cast<uintptr_t>(fp);
    const uintptr_t hi = lo + (16u << 20);      // 16 MB window above the current frame
    usize guard = 0;
    while ( fp && n < maxn && guard++ < 128 ) {
      const uintptr_t a = reinterpret_cast<uintptr_t>(fp);
      if ( a < lo || a >= hi || (a & (sizeof(void *) - 1)) ) break;      // not a plausible frame pointer
      void *ret = fp[1];
      void **next = reinterpret_cast<void **>(fp[0]);
      if ( !ret ) break;
      out[n++] = ret;
      if ( reinterpret_cast<uintptr_t>(next) <= a ) break;      // fp chain must strictly ascend
      fp = next;
    }
  }
  for ( usize i = n; i < maxn; ++i ) out[i] = nullptr;
  return n;
}

inline void
__thread_name(i32 tid, char *out, usize cap) noexcept
{
  if ( cap ) out[0] = 0;
  if ( cap < 2 || tid <= 0 ) return;
  char path[48];
  usize n = 0;
  for ( const char *p = "/proc/self/task/"; *p; ++p ) path[n++] = *p;
  char tmp[12];
  usize tn = 0;
  u32 v = static_cast<u32>(tid);
  do {
    tmp[tn++] = static_cast<char>('0' + v % 10);
    v /= 10;
  } while ( v );
  while ( tn ) path[n++] = tmp[--tn];
  for ( const char *p = "/comm"; *p; ++p ) path[n++] = *p;
  path[n] = 0;
  const int fd = static_cast<int>(micron::syscall(SYS_open, path, 0 /*O_RDONLY*/, 0));
  if ( fd < 0 ) return;
  const long r = static_cast<long>(micron::syscall(SYS_read, fd, out, cap - 1));
  micron::syscall(SYS_close, fd);
  if ( r <= 0 ) {
    out[0] = 0;
    return;
  }
  usize len = static_cast<usize>(r);
  while ( len && (out[len - 1] == '\n' || out[len - 1] == '\r') ) --len;      // strip trailing newline
  out[len] = 0;
}

inline void
__d_tid(i32 tid) noexcept
{
  __d_i(tid);
  char nm[24];
  __thread_name(tid, nm, sizeof(nm));
  if ( nm[0] ) {
    __d(" (");
    __d(nm);
    __d(")");
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// crash-safe inspection

// amd64 only for now
inline void __report_hw_fault(int sig, const void *addr, const void *uctx) noexcept;

#if defined(__micron_arch_amd64)
constexpr static const bool __crash_safe_on = __default_doctor_crash_safe;

// buf layout: [0]rbx [1]rbp [2]r12 [3]r13 [4]r14 [5]r15 [6]rsp [7]rip
[[gnu::naked]] __micron_no_ssp inline int
__dr_setjmp(void **) noexcept      // arg in %rdi
{
  asm volatile("movq %%rbx, 0(%%rdi)\n\t"
               "movq %%rbp, 8(%%rdi)\n\t"
               "movq %%r12, 16(%%rdi)\n\t"
               "movq %%r13, 24(%%rdi)\n\t"
               "movq %%r14, 32(%%rdi)\n\t"
               "movq %%r15, 40(%%rdi)\n\t"
               "leaq 8(%%rsp), %%rax\n\t"      // caller's rsp (after the pending ret)
               "movq %%rax, 48(%%rdi)\n\t"
               "movq (%%rsp), %%rax\n\t"      // return address
               "movq %%rax, 56(%%rdi)\n\t"
               "xorl %%eax, %%eax\n\t"      // return 0 on the direct call
               "ret\n\t" ::
                   : "memory");
}

[[gnu::naked]] __micron_no_ssp inline void
__dr_longjmp(void **, int) noexcept      // buf in %rdi, val in %esi
{
  asm volatile("movq 0(%%rdi), %%rbx\n\t"
               "movq 8(%%rdi), %%rbp\n\t"
               "movq 16(%%rdi), %%r12\n\t"
               "movq 24(%%rdi), %%r13\n\t"
               "movq 32(%%rdi), %%r14\n\t"
               "movq 40(%%rdi), %%r15\n\t"
               "movq 48(%%rdi), %%rsp\n\t"
               "movl %%esi, %%eax\n\t"      // return val (forced to 1 if 0)
               "testl %%eax, %%eax\n\t"
               "jnz 1f\n\t"
               "movl $1, %%eax\n\t"
               "1:\n\t"
               "jmp *56(%%rdi)\n\t" ::
                   : "memory");
}

struct __dr_sigcontext {
  u64 r8, r9, r10, r11, r12, r13, r14, r15;
  u64 rdi, rsi, rbp, rbx, rdx, rax, rcx, rsp;
  u64 rip, eflags;
  u16 cs, gs, fs, ss;      // ss was __pad0 before Linux 4.1; real since (uc_flags bit 1)
  u64 err, trapno, oldmask, cr2;
  const void *fpstate;      // points into the sigframe; null if no FPU context saved
  u64 __reserved[8];
};

struct __dr_ucontext {
  u64 uc_flags;
  void *uc_link;
  u64 __uc_stack[3];      // stack_t, opaque: never read
  __dr_sigcontext mc;
  u64 sigmask;
};

static_assert(sizeof(__dr_sigcontext) == 256);
static_assert(__builtin_offsetof(__dr_sigcontext, rip) == 128);
static_assert(__builtin_offsetof(__dr_sigcontext, cs) == 144);
static_assert(__builtin_offsetof(__dr_sigcontext, err) == 152);
static_assert(__builtin_offsetof(__dr_sigcontext, fpstate) == 184);
static_assert(__builtin_offsetof(__dr_ucontext, mc) == 40);
static_assert(__builtin_offsetof(__dr_ucontext, sigmask) == 296);
#else
constexpr static const bool __crash_safe_on = false;      // crash-safe sweep is x86-64 only for now
#endif

inline thread_local void *__fault_jmp[8];      // setjmp buffer: rbx,rbp,r12-r15,rsp,rip
inline thread_local volatile bool __fault_armed = false;
inline thread_local const void *volatile __fault_addr = nullptr;

inline micron::posix::sigaction_t __prev_segv{};
inline micron::posix::sigaction_t __prev_bus{};
inline micron::atomic_flag __sig_install_once{};

inline void
__doctor_fault_handler(int sig, micron::posix::siginfo_t *info, void *ucontext) noexcept
{
  const void *addr = info ? info->_sifields._sigfault.si_addr : nullptr;
  if ( __fault_armed ) {
    __fault_armed = false;
    __fault_addr = addr;
#if defined(__micron_arch_amd64)
    __dr_longjmp(__fault_jmp, 1);      // noreturn -> back to the armed __dr_setjmp
#endif
  }

  __report_hw_fault(sig, addr, ucontext);
  micron::posix::sigaction(sig, (sig == micron::posix::sig_bus) ? __prev_bus : __prev_segv, nullptr);
}

inline void
__install_fault_handler(void) noexcept
{
  if constexpr ( __crash_safe_on ) {
    if ( __sig_install_once.test_and_set(micron::memory_order_acquire) ) return;      // exactly once
    micron::posix::sigaction_t sa{};
    sa.sigaction_handler.sa_sigaction = &__doctor_fault_handler;
    micron::posix::sigemptyset(sa.sa_mask);
    // WARNING: SA_NODEFER is required we longjmp out instead of returning via sigreturn
    // the kernel never unblocks the signal and without NODEFER the second fault arrives blocked
    sa.sa_flags = micron::posix::sa_siginfo | micron::posix::sa_nodefer;
    micron::posix::sigaction(micron::posix::sig_segv, sa, &__prev_segv);
    micron::posix::sigaction(micron::posix::sig_bus, sa, &__prev_bus);
  }
}

template<class F>
[[gnu::noinline]] inline bool
__guard_read(F &&f) noexcept
{
  if constexpr ( !__crash_safe_on ) {
    f();
    return true;
  } else {
    if ( __dr_setjmp(__fault_jmp) != 0 ) {
      __fault_armed = false;
      return false;      // faulted
    }
    __fault_armed = true;
    f();
    __fault_armed = false;
    return true;
  }
}

struct __fault_installer {
  __fault_installer(void) noexcept { __install_fault_handler(); }
};

inline __fault_installer __fault_installer_instance{};

// dump as pure hex rows
inline void
__splat_window(const void *base, usize len) noexcept
{
  const uintptr_t a = reinterpret_cast<uintptr_t>(base);
  byte buf[16];
  const auto emit = [&](uintptr_t at, usize n) {
    const bool ok = __guard_read([&] {
      const byte *s = reinterpret_cast<const byte *>(at);
      for ( usize i = 0; i < n; ++i ) buf[i] = s[i];      // copy under guard; print outside
    });
    if ( ok ) {
      __dump_row(reinterpret_cast<const void *>(at), buf, n);
    } else {
      __d("  ");
      __d_ptr(reinterpret_cast<const void *>(at));
      __d(":  (unreadable)\n");
    }
  };
  emit(a - 8, 8);
  for ( usize off = 0; off < len; off += 16 ) emit(a + off, (len - off < 16) ? (len - off) : 16);
}

enum class __rec_state : u8 {
  empty = 0,            // slot unused
  live = 1,             // currently allocated
  freed = 2,            // freed (retained for double-free forensics)
  tombstoned = 3,       // tombstone-freed (large/huge tiers)
  quarantined = 4,      // rescue quarantined (leaked deliberately)
};

struct __rec {
  uintptr_t key;          // user pointer; 0 == empty slot
  usize req_size;         // requested size at alloc time
  u64 alloc_op;           // op# of the allocation
  u64 free_op;            // op# of the (first) free, 0 if live
  const void *owner;      // __arena* owning the granule at alloc time
  i32 alloc_tid;          // kernel tid that allocated
  i32 free_tid;           // kernel tid that (first) freed
  __rec_state state;
  u32 hdr_shadow;
  usize free_len;
  void *alloc_bt[__doctor_bt_depth];
};

struct __event {
  u64 op;             // op# stamped on this event
  uintptr_t key;      // user pointer
  usize size;         // req_size for alloc, free len for free
  i32 tid;            // kernel tid
  u8 kind;            // 0 alloc, 1 free, 2 tombstone, 3 realloc (in-place)
};

constexpr static const usize __doctor_event_ring_cap = (1u << 16);      // 64K events (~2 MB), power of two

// NOTE: this is highly inefficient but irrelevant, only to be used in hard debug scenarios
struct __doctor_state {
  micron::atomic_token<u64> op_counter{ 0 };              // monotonic op# stamped on every alloc/free
  micron::atomic_token<u64> corruption_events{ 0 };       // faults classed as corruption
  micron::atomic_token<u64> remote_free_events{ 0 };      // cross-thread (wrong-thread) frees observed
  micron::atomic_flag lock{};                             // the doctor spinlock

  // ledger table (guarded by lock)
  __rec *slots{ nullptr };
  usize cap{ 0 };           // slot count, power of two
  usize mask{ 0 };          // cap - 1
  usize occupied{ 0 };      // non-empty slots (live + freed + tombstoned + quarantined)
  usize n_live{ 0 };
  usize n_freed{ 0 };

  __event *ev{ nullptr };
  u64 ev_total{ 0 };      // total events pushed; head index == ev_total & (cap-1)

  [[gnu::always_inline]] u64
  next_op(void) noexcept
  {
    return op_counter.fetch_add(1, micron::memory_order_relaxed);
  }

  void
  __push_event(u8 kind, uintptr_t key, usize size, u64 op, i32 tid) noexcept
  {
    if ( !ev ) {
      ev = reinterpret_cast<__event *>(__mmap_bytes(__doctor_event_ring_cap * sizeof(__event)));
      if ( !ev ) return;      // best-effort: no timeline if the mmap fails
    }
    ev[ev_total & (__doctor_event_ring_cap - 1)] = __event{ op, key, size, tid, kind };
    ++ev_total;
  }

  void
  acquire(void) noexcept
  {
    while ( lock.test_and_set(micron::memory_order_acquire) ) __cpu_pause();
  }

  void
  release(void) noexcept
  {
    lock.clear(micron::memory_order_release);
  }

  // splitmix64, simple
  static u64
  __hash(uintptr_t k) noexcept
  {
    u64 x = static_cast<u64>(k >> 4);
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
  }

  void
  __rehash(usize newcap) noexcept
  {
    if ( newcap == 0 ) return;
    __rec *old = slots;
    usize oldcap = cap;
    usize bytes = newcap * sizeof(__rec);
    __rec *fresh = reinterpret_cast<__rec *>(__mmap_bytes(bytes));
    if ( !fresh ) {
      // cannot grow: leave the old table in place; doctor becomes best-effort
      __banner("ledger: mmap failed to grow ledger; tracking is now best-effort\n");
      return;
    }
    slots = fresh;
    cap = newcap;
    mask = newcap - 1;
    occupied = 0;
    if ( old ) {
      for ( usize i = 0; i < oldcap; ++i ) {
        if ( old[i].state != __rec_state::empty && old[i].key != 0 ) {
          __rec *dst = __slot_for(old[i].key);
          if ( dst ) {
            *dst = old[i];
            ++occupied;
          }
        }
      }
      __munmap_bytes(old, oldcap * sizeof(__rec));
    }
  }

  void
  __ensure(void) noexcept
  {
    if ( !slots ) __rehash(1u << 12);      // 4096 slots initial
  }

  void
  __grow_if_needed(void) noexcept
  {
    if ( cap == 0 ) return;      // ledger not initialized (mmap failed); nothing to grow
    // grow at 70% load
    if ( occupied * 10 < cap * 7 ) return;
    usize newcap = cap << 1;
    if ( newcap > __default_doctor_max_records ) {
      if ( __drop_freed() && occupied * 10 < cap * 7 ) return;
    }
    __rehash(newcap);
  }

  __rec *
  __slot_for(uintptr_t key) noexcept
  {
    if ( !slots ) return nullptr;
    usize i = static_cast<usize>(__hash(key)) & mask;
    for ( usize probes = 0; probes <= cap; ++probes ) {
      __rec &s = slots[i];
      if ( s.state == __rec_state::empty || s.key == key ) return &s;
      i = (i + 1) & mask;
    }
    return nullptr;
  }

  __rec *
  __find(uintptr_t key) noexcept
  {
    if ( !slots ) return nullptr;
    usize i = static_cast<usize>(__hash(key)) & mask;
    for ( usize probes = 0; probes <= cap; ++probes ) {
      __rec &s = slots[i];
      if ( s.state == __rec_state::empty ) return nullptr;
      if ( s.key == key ) return &s;
      i = (i + 1) & mask;
    }
    return nullptr;
  }

  bool
  __drop_freed(void) noexcept
  {
    if ( !slots || occupied <= n_live ) return false;      // nothing non-live to reclaim
    __rec *old = slots;
    usize oldcap = cap;
    const usize before = occupied;
    __rec *fresh = reinterpret_cast<__rec *>(__mmap_bytes(oldcap * sizeof(__rec)));
    if ( !fresh ) return false;
    slots = fresh;
    mask = oldcap - 1;      // cap unchanged
    occupied = 0;
    for ( usize i = 0; i < oldcap; ++i ) {
      if ( old[i].state == __rec_state::live ) {
        __rec *dst = __slot_for(old[i].key);
        if ( dst ) {
          *dst = old[i];
          ++occupied;
        }
      }
    }
    n_freed = 0;
    __munmap_bytes(old, oldcap * sizeof(__rec));
    __banner("ledger: evicted ");
    __d_u(before - occupied);
    __d(" non-live records (freed/tombstoned/quarantined) to stay under the record cap\n");
    return true;
  }
};

inline __doctor_state __dr{};

// raii lock
inline thread_local u32 __dr_lock_depth = 0;

struct __scoped_lock {
  __scoped_lock(void) noexcept
  {
    if ( __dr_lock_depth != 0 ) {
      __banner("FATAL: re-entrant doctor lock acquisition on one thread (would deadlock)\n");
      abc::abort_state();
    }
    ++__dr_lock_depth;
    __dr.acquire();
  }

  ~__scoped_lock(void) noexcept
  {
    __dr.release();
    --__dr_lock_depth;
  }

  __scoped_lock(const __scoped_lock &) = delete;
  __scoped_lock &operator=(const __scoped_lock &) = delete;
};

// per-thread re-entry guard
inline thread_local u32 __in_doctor = 0;

struct __reentry {
  bool entered;

  __reentry(void) noexcept : entered(__in_doctor == 0) { ++__in_doctor; }

  ~__reentry(void) noexcept { --__in_doctor; }

  explicit
  operator bool(void) const noexcept
  {
    return entered;
  }

  __reentry(const __reentry &) = delete;
  __reentry &operator=(const __reentry &) = delete;
};

// index of an arena within the primary pool, or -1 (overflow / not found)
inline i32
__arena_index(const void *a) noexcept
{
  if ( !a ) return -1;
  const u32 n = __arena_pool_next.get(micron::memory_order_acquire);
  const u32 lim = n > __max_arenas ? __max_arenas : n;
  for ( u32 i = 0; i < lim; ++i ) {
    if ( static_cast<const void *>(__arena_pool[i]) == a ) return static_cast<i32>(i);
  }
  return -1;
}

inline const char *
__state_name(__rec_state s) noexcept
{
  switch ( s ) {
  case __rec_state::empty:
    return "empty";
  case __rec_state::live:
    return "live";
  case __rec_state::freed:
    return "freed";
  case __rec_state::tombstoned:
    return "tombstoned";
  case __rec_state::quarantined:
    return "quarantined";
  }
  return "?";
}

// buddy block orders start at 0, which collides with hdr_shadow's "0 == not captured" sentinel
inline constexpr u32 __hdr_shadow_buddy_captured = 0x80000000u;

inline void
__doctor_arm_canaries(byte *ptr, usize req, const void *owner, __rec &s) noexcept
{
  s.hdr_shadow = 0;
  if constexpr ( __default_doctor_canary ) {
    __arena *o = static_cast<__arena *>(const_cast<void *>(owner));
    if ( !o || o != __tls_arena ) return;
    const int kind = o->__doctor_tier_kind(reinterpret_cast<addr_t *>(ptr));
    const usize real = o->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
    if ( kind == 1 ) {      // TLSF: bsize (u32) at user - __hdr_offset
      u32 bs = 0;
      __builtin_memcpy(&bs, ptr - __hdr_offset, sizeof(bs));
      s.hdr_shadow = bs;
    } else if ( kind == 2 ) {      // buddy: order (i32) at the tail = user + real
      i32 ord = 0;
      __builtin_memcpy(&ord, ptr + real, sizeof(ord));
      // OR in the captured marker so order 0 is distinct from the 0 "not captured" sentinel
      s.hdr_shadow = __hdr_shadow_buddy_captured | static_cast<u32>(ord);
    }
    if constexpr ( !ABC_EFF_REDZONE ) {      // the allocator's own redzone owns that region if enabled
      if ( real > req ) __builtin_memset(ptr + req, __default_doctor_canary_byte, real - req);
    }
  }
}

inline void
record_alloc(byte *ptr, usize req_size) noexcept
{
  if ( !ptr ) return;
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __dr.__ensure();
  if ( !__dr.slots ) return;      // ledger mmap failed: degrade to no-op, never deref a null table
  __dr.__grow_if_needed();
  __rec *s = __dr.__slot_for(reinterpret_cast<uintptr_t>(ptr));
  if ( !s ) return;
  if ( s->state == __rec_state::empty )
    ++__dr.occupied;
  else if ( s->state == __rec_state::freed || s->state == __rec_state::tombstoned ) {
    if ( __dr.n_freed ) --__dr.n_freed;
  }
  if ( s->state != __rec_state::live ) ++__dr.n_live;
  s->key = reinterpret_cast<uintptr_t>(ptr);
  s->req_size = req_size;
  s->alloc_op = __dr.next_op();
  s->free_op = 0;
  s->alloc_tid = __gettid();
  s->free_tid = 0;
  s->free_len = 0;
  s->owner = static_cast<const void *>(__owner_of(ptr));
  s->state = __rec_state::live;
  __doctor_arm_canaries(ptr, req_size, s->owner, *s);
  __capture_backtrace(s->alloc_bt, __doctor_bt_depth);
  __dr.__push_event(0, s->key, req_size, s->alloc_op, s->alloc_tid);
}

inline void
record_realloc(byte *ptr, usize new_req_size) noexcept
{
  if ( !ptr ) return;
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( !s || s->state != __rec_state::live ) return;      // only refresh a live, tracked block
  s->req_size = new_req_size;
  __doctor_arm_canaries(ptr, new_req_size, s->owner, *s);
  __dr.__push_event(3, s->key, new_req_size, __dr.next_op(), __gettid());
}

// mark a tracked pointer freed
inline void
__mark_free_locked(byte *ptr, __rec_state to, usize len) noexcept
{
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( !s ) return;      // untracked (foreign)
  if ( s->state == __rec_state::freed || s->state == __rec_state::tombstoned || s->state == __rec_state::quarantined ) {
    // already freed/tombstoned (double free), or quarantined by a rescue handler
    return;
  }
  if ( s->state == __rec_state::live && __dr.n_live ) --__dr.n_live;
  ++__dr.n_freed;
  s->state = to;
  s->free_op = __dr.next_op();
  s->free_tid = __gettid();
  s->free_len = len;
  __dr.__push_event(to == __rec_state::tombstoned ? 2 : 1, s->key, len, s->free_op, s->free_tid);
}

inline void
record_free(byte *ptr, usize len) noexcept
{
  if ( !ptr ) return;
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __mark_free_locked(ptr, __rec_state::freed, len);
}

inline void
record_tombstone(byte *ptr, usize len) noexcept
{
  if ( !ptr ) return;
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __mark_free_locked(ptr, __rec_state::tombstoned, len);
}

inline void
record_remote_free(byte *ptr, usize len) noexcept
{
  if ( !ptr ) return;
  __reentry re;
  if ( !re ) return;
  __dr.remote_free_events.fetch_add(1, micron::memory_order_relaxed);
  __scoped_lock g;
  __mark_free_locked(ptr, __rec_state::freed, len);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// query / reporting API

inline micron::atomic_flag __maps_dumped{};

inline void
__dump_maps(void) noexcept
{
  if ( __maps_dumped.test_and_set(micron::memory_order_acquire) ) return;
  const int fd = static_cast<int>(micron::syscall(SYS_open, "/proc/self/maps", 0 /*O_RDONLY*/, 0));
  if ( fd < 0 ) return;
  __d("  --- /proc/self/maps (resolve backtrace addrs: addr2line -e <binary> <addr - module base>) ---\n");
  char buf[1024];
  for ( ;; ) {
    const long r = static_cast<long>(micron::syscall(SYS_read, fd, buf, sizeof(buf)));
    if ( r <= 0 ) break;
    abc::__write(buf, static_cast<usize>(r));
  }
  micron::syscall(SYS_close, fd);
}

inline void
__print_backtrace(const char *label, void *const *bt) noexcept
{
  if constexpr ( __default_doctor_backtrace ) {
    if ( !bt[0] ) return;      // nothing captured (backtrace off, or no frame pointers)
    __d("  ");
    __d(label);
    for ( usize i = 0; i < __doctor_bt_depth && bt[i]; ++i ) {
      __d(" ");
      __d_ptr(bt[i]);
    }
    __d_nl();
    __dump_maps();
  }
}

inline void
__print_record(const __rec *s) noexcept
{
  __d("  ptr        ");
  __d_ptr(reinterpret_cast<const void *>(s->key));
  __d_nl();
  __d("  state      ");
  __d(__state_name(s->state));
  __d_nl();
  __d("  req_size   ");
  __d_u(s->req_size);
  __d_nl();
  __d("  alloc      op#");
  __d_u(s->alloc_op);
  __d("  tid ");
  __d_tid(s->alloc_tid);
  __d_nl();
  __print_backtrace("allocated at:", s->alloc_bt);
  if ( s->state != __rec_state::live ) {
    __d("  freed      op#");
    __d_u(s->free_op);
    __d("  tid ");
    __d_tid(s->free_tid);
    __d("  size ");
    if ( s->free_len )
      __d_u(s->free_len);
    else
      __d("(size-less)");
    __d_nl();
  }
  i32 aidx = __arena_index(s->owner);
  __d("  owner      arena ");
  if ( aidx >= 0 ) {
    __d("#");
    __d_i(aidx);
  } else {
    __d("(overflow/recycled)");
  }
  __d_nl();
}

// forensic record dump for a single pointer, with live provenance basics
inline void
report(const void *ptr)
{
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __banner("report ");
  __d_ptr(ptr);
  __d_nl();
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( !s ) {
    __d("  not tracked by doctor ledger");
    __d_nl();
    __d("  in-VA      ");
    __d(__va_contains(ptr) ? "yes (abcmalloc region, but no ledger record)" : "no (foreign / stack / static)");
    __d_nl();
    return;
  }
  __print_record(s);
}

inline void
history(const void *ptr)
{
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __banner("history ");
  __d_ptr(ptr);
  __d_nl();
  const uintptr_t k = reinterpret_cast<uintptr_t>(ptr);
  if ( !__dr.ev || __dr.ev_total == 0 ) {
    __d("  (no event ring)\n");
    __rec *s = __dr.__find(k);
    if ( s ) __print_record(s);
    return;
  }
  static const char *const kn[4] = { "ALLOC      ", "FREE       ", "TOMBSTONE  ", "REALLOC    " };
  const usize cap = __doctor_event_ring_cap;
  const u64 total = __dr.ev_total;
  const u64 start = total > cap ? total - cap : 0;      // oldest retained event index
  usize shown = 0;
  for ( u64 i = start; i < total; ++i ) {
    const __event &e = __dr.ev[i & (cap - 1)];
    if ( e.key != k ) continue;
    __d("  op#");
    __d_u(e.op);
    __d("  ");
    __d(e.kind < 4 ? kn[e.kind] : "?          ");
    __d_u(e.size);
    __d(" B  tid ");
    __d_i(e.tid);
    __d_nl();
    ++shown;
  }
  if ( !shown )
    __d("  (no events for this pointer in the retained window)\n");
  else {
    __d("  events: ");
    __d_u(shown);
    if ( total > cap ) __d(" (older events evicted from the ring)");
    __d_nl();
  }
}

// is this pointer currently tracked as live?
inline bool
is_tracked(const void *ptr)
{
  __reentry re;
  if ( !re ) return false;
  __scoped_lock g;
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  return s && s->state == __rec_state::live;
}

inline void
leaks(void)
{
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __install_fault_handler();
  __banner("leaks: live tracked pointers (classified by tier)\n");
  __arena *const self = __tls_arena;
  usize shown = 0, total_bytes = 0;
  // classes: 0 unresolved/internal, 1 TLSF, 2 buddy, 3 cross-thread (owned by another arena)
  usize cls_cnt[4] = { 0, 0, 0, 0 };
  usize cls_bytes[4] = { 0, 0, 0, 0 };
  usize cls_max[4] = { 0, 0, 0, 0 };
  if ( __dr.slots ) {
    for ( usize i = 0; i < __dr.cap; ++i ) {
      const __rec &s = __dr.slots[i];
      if ( s.state != __rec_state::live ) continue;
      ++shown;
      total_bytes += s.req_size;
      byte *u = reinterpret_cast<byte *>(s.key);
      int cls = 0;
      __arena *o = __va_contains(u) ? __owner_of(u) : nullptr;
      if ( o && o != self )
        cls = 3;
      else if ( o == self ) {
        int kind = 0;
        __guard_read([&] { kind = o->__doctor_tier_kind(reinterpret_cast<addr_t *>(u)); });
        cls = (kind == 1) ? 1 : (kind == 2) ? 2 : 0;
      }
      ++cls_cnt[cls];
      cls_bytes[cls] += s.req_size;
      if ( s.req_size > cls_max[cls] ) cls_max[cls] = s.req_size;
      __d("  ");
      __d_ptr(u);
      __d("  ");
      __d_u(s.req_size);
      __d(" B  alloc op#");
      __d_u(s.alloc_op);
      __d("  tid ");
      __d_i(s.alloc_tid);
      __d_nl();
    }
  }
  __d("  total live: ");
  __d_u(shown);
  __d(" (");
  __d_u(total_bytes);
  __d(" B)\n");
  static const char *const names[4] = { "unresolved/internal", "TLSF (user)", "buddy (user)", "cross-thread" };
  for ( int c = 0; c < 4; ++c )
    if ( cls_cnt[c] ) {
      __d("    ");
      __d(names[c]);
      __d(": ");
      __d_u(cls_cnt[c]);
      __d(" blocks, ");
      __d_u(cls_bytes[c]);
      __d(" B (largest ");
      __d_u(cls_max[c]);
      __d(" B)\n");
    }
}

// dump process-wide doctor counters
inline void
stats(void)
{
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __banner("stats\n");
  __d("  op_counter         ");
  __d_u(__dr.op_counter.get(micron::memory_order_relaxed));
  __d_nl();
  __d("  live_records       ");
  __d_u(__dr.n_live);
  __d_nl();
  __d("  freed_records      ");
  __d_u(__dr.n_freed);
  __d_nl();
  __d("  ledger_occupied    ");
  __d_u(__dr.occupied);
  __d(" / ");
  __d_u(__dr.cap);
  __d_nl();
  __d("  corruption_events  ");
  __d_u(__dr.corruption_events.get(micron::memory_order_relaxed));
  __d_nl();
  __d("  remote_free_events ");
  __d_u(__dr.remote_free_events.get(micron::memory_order_relaxed));
  __d_nl();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gdb-like forensics

// kind: 1 = TLSF, 2 = buddy, 0 = unresolved
inline const char *
__kind_name(int kind) noexcept
{
  return kind == 1 ? "TLSF" : kind == 2 ? "buddy" : "unresolved";
}

inline void
__d_flag_names(i32 f) noexcept
{
  if ( f == __block_free ) {
    __d("__block_free");
  } else if ( f & __block_alloc ) {
    __d("__block_alloc");
    if ( f & __block_temporal ) __d("|__block_temporal");
    if ( f & __block_tombstone ) __d("|__block_tombstone");
    if ( f & ~(__block_alloc | __block_temporal | __block_tombstone) ) __d("|INVALID");
  } else if ( f == __block_tombstone ) {
    __d("__block_tombstone");
  } else {
    __d("INVALID");
  }
}

inline void
__d_flags(i32 f) noexcept
{
  __d_i(f);
  __d(" (");
  __d_flag_names(f);
  __d(")");
}

inline void
__d_ok(bool ok) noexcept
{
  __d(ok ? "  OK" : "  <-- BAD");
}

inline bool
__alloc_flags_ok(i32 f) noexcept
{
  return f == __block_alloc || f == (__block_alloc | __block_temporal) || f == __block_tombstone;
}

inline bool
__flags_known(i32 f) noexcept
{
  return f == __block_free || __alloc_flags_ok(f);
}

// decode && splat the block metadata header for a live/known user pointer
inline void
__decode_header(byte *user, usize user_size, int kind, bool expect_live, const __rec *rec) noexcept
{
  if ( kind != 1 && kind != 2 ) {
    __d("  header       (allocator kind unresolved; block not in any tier)\n");
    return;
  }
  byte *h = (kind == 1) ? user - __hdr_offset : user + user_size;      // buddy header lives at the block tail
  const usize hdr_n = (kind == 1) ? __hdr_offset : 2 * sizeof(i32);
  byte c[__hdr_offset] = {};
  const bool readable = __guard_read([&] {
    for ( usize i = 0; i < hdr_n; ++i ) c[i] = h[i];      // copy under guard; decode outside
  });

  const u32 shadow = (rec && rec->state == __rec_state::live) ? rec->hdr_shadow : 0;
  const bool tlsf_shadow = shadow && kind == 1 && !(shadow & __hdr_shadow_buddy_captured);
  const bool buddy_shadow = shadow && kind == 2 && (shadow & __hdr_shadow_buddy_captured);

  const auto field_flags = [&](i32 f) {
    const bool ok = __flags_known(f) && (!expect_live || (f & __block_alloc));
    __d("    i32       flags      = ");
    __d_i(f);
    __d(";   // ");
    __d_flag_names(f);
    if ( !ok ) __d("  <-- BAD");
    __d_nl();
    if ( !ok ) {
      __d_bad_begin();
      if ( expect_live )
        __d("    i32       flags      = 1;   // SHOULD READ: __block_alloc (temporal bit unknowable)");
      else
        __d("    i32       flags      = 0;   // SHOULD READ: __block_free (1/2/5 also valid)");
      __d_bad_end();
      __d_nl();
    }
  };

  if ( !readable ) {
    __d("  header       (");
    __d(kind == 1 ? "tlsf_hdr" : "block_header");
    __d(" slot unreadable @");
    __d_ptr(h);
    __d("; raw window below)\n");
  } else if ( kind == 1 ) {
    u32 bsize = 0;
    i32 flags = 0;
    u64 prev_phys = 0, next_free = 0, prev_free = 0;
    __builtin_memcpy(&bsize, c + 0, sizeof(bsize));
    __builtin_memcpy(&flags, c + 4, sizeof(flags));
    __builtin_memcpy(&prev_phys, c + 8, sizeof(prev_phys));
    __builtin_memcpy(&next_free, c + 16, sizeof(next_free));
    __builtin_memcpy(&prev_free, c + 24, sizeof(prev_free));
    __d("  header       struct tlsf_hdr {   // @");
    __d_ptr(h);
    __d(" = user-32  cache_list.hpp:67\n");
    const u64 bsize_floor = rec ? rec->req_size + __hdr_offset : 2 * __hdr_offset;
    const bool bsize_ok = tlsf_shadow ? (bsize == shadow) : ((bsize & (__hdr_offset - 1)) == 0 && bsize >= bsize_floor);
    __d("    u32       bsize      = ");
    __d_u(bsize);
    __d(";");
    if ( !bsize_ok )
      __d("  <-- BAD");
    else
      __d(tlsf_shadow ? "   // matches alloc-time snapshot" : "   // plausible (no alloc-time snapshot)");
    __d_nl();
    if ( !bsize_ok ) {
      __d_bad_begin();
      if ( tlsf_shadow ) {
        __d("    u32       bsize      = ");
        __d_u(shadow);
        __d(";   // SHOULD READ (alloc-time snapshot)");
      } else {
        __d("    u32       bsize      = ?;   // SHOULD READ: >= ");
        __d_u(bsize_floor);
        __d(", 32-byte aligned (exact unknown: no alloc-time snapshot)");
      }
      __d_bad_end();
      __d_nl();
    }
    field_flags(flags);
    const auto field_link = [&](const char *name_padded, u64 v, const char *note) {
      __d("    tlsf_hdr *");
      __d(name_padded);
      __d("= (tlsf_hdr *)");
      __d_hex64(v);
      __d(";   // ");
      __d(note);
      __d_nl();
    };
    field_link("prev_phys  ", prev_phys, "unvalidated (physical link)");
    field_link("next_free  ", next_free, "unvalidated (free link; stale while allocated)");
    field_link("prev_free  ", prev_free, "unvalidated (free link; stale while allocated)");
    __d("  };\n");
  } else {
    i32 order = 0;
    i32 flags = 0;
    __builtin_memcpy(&order, c + 0, sizeof(order));
    __builtin_memcpy(&flags, c + 4, sizeof(flags));
    __d("  header       struct block_header {   // @");
    __d_ptr(h);
    __d(" = user+");
    __d_u(user_size);
    __d(" (buddy tail)  metadata.hpp:35\n");
    const i32 want_order = buddy_shadow ? static_cast<i32>(shadow & ~__hdr_shadow_buddy_captured) : 0;
    const bool order_ok = buddy_shadow ? (order == want_order) : (order >= 0 && order <= 25);
    __d("    i32       order      = ");
    __d_i(order);
    __d(";");
    if ( !order_ok )
      __d("  <-- BAD");
    else
      __d(buddy_shadow ? "   // matches alloc-time snapshot" : "   // plausible (no alloc-time snapshot)");
    __d_nl();
    if ( !order_ok ) {
      __d_bad_begin();
      if ( buddy_shadow ) {
        __d("    i32       order      = ");
        __d_i(want_order);
        __d(";   // SHOULD READ (alloc-time snapshot)");
      } else {
        __d("    i32       order      = ?;   // SHOULD READ: 0..25 (exact unknown: no alloc-time snapshot)");
      }
      __d_bad_end();
      __d_nl();
    }
    field_flags(flags);
    __d("  };   // 8 of 32 tail-slot bytes used (free_list.hpp:41)\n");
  }
  __d("    raw [hdr-8, hdr+32):\n");
  __splat_window(h, __hdr_offset);
}

// dump redzone canaries with expected vs actual (only meaningful when redzone on)
inline void
__dump_redzones(byte *user, usize user_size) noexcept
{
  __d("  redzone      expected byte 0x");
  __d_hex_byte(__default_redzone_byte);
  __d(" x ");
  __d_u(__default_redzone_size);
  __d(" each side\n");
  __d("    leading  @");
  __d_ptr(user - __default_redzone_size);
  __d(":\n");
  __dump_bytes(user - __default_redzone_size, __default_redzone_size);
  if ( user_size ) {
    __d("    trailing @");
    __d_ptr(user + user_size);
    __d(":\n");
    __dump_bytes(user + user_size, __default_redzone_size);
  }
}

inline void
__forensics(const void *p, bool bt) noexcept
{
  __install_fault_handler();
  byte *ptr = reinterpret_cast<byte *>(const_cast<void *>(p));
  const bool inva = __va_contains(p);
  __d("  ptr          ");
  __d_ptr(p);
  __d_nl();
  __d("  in-VA        ");
  __d(inva ? "yes (abcmalloc reserved region)" : "no (foreign / stack / static / libc)");
  __d_nl();

  const __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( s ) {
    __d("  ledger       state=");
    __d(__state_name(s->state));
    __d("  req_size=");
    __d_u(s->req_size);
    __d("  alloc op#");
    __d_u(s->alloc_op);
    __d(" tid ");
    __d_tid(s->alloc_tid);
    if ( s->state != __rec_state::live ) {
      __d("  |  FIRST FREED op#");
      __d_u(s->free_op);
      __d(" tid ");
      __d_tid(s->free_tid);
      __d(" size ");
      if ( s->free_len )
        __d_u(s->free_len);
      else
        __d("(size-less)");
    }
    __d_nl();
    if ( bt ) __print_backtrace("allocated at:", s->alloc_bt);
  } else {
    __d("  ledger       no record (never allocated by abcmalloc, or record evicted)\n");
  }

  if ( p ) {
    __d("  memory       [ptr-8, ptr+64) raw window:\n");
    __splat_window(ptr, 64);
  }

  if ( !inva ) return;      // do NOT dereference arena/memory state for foreign pointers

  // NOTE: use the raw TLS arena / owner table directly, never __current_arena()
  // here, which would drain the remote-free ring -> pop() -> record_free() ->
  // re-enter the (non-recursive) doctor lock we already hold -> deadlock
  __arena *owner = __owner_of(ptr);
  __arena *cur = __tls_arena;
  i32 aidx = __arena_index(owner);
  __d("  owner        arena ");
  if ( aidx >= 0 ) {
    __d("#");
    __d_i(aidx);
  } else {
    __d("(overflow / unregistered granule)");
  }
  if ( owner && cur && owner != cur ) __d("   [CROSS-THREAD free: freeing thread is NOT the owner]");
  __d_nl();

  __arena *q = owner ? owner : cur;
  if ( !q ) {
    __d("  block        (no resolvable arena for this granule)\n");
    return;
  }
  if ( q != cur ) {
    __d("  block        (cross-thread: owned by another arena; not inspected to avoid racing the owner)\n");
    return;
  }
  usize sz = 0;
  int kind = 0;
  bool live = false, prov = false, present = false, cached = false;
  const bool got = __guard_read([&] {
    sz = q->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
    kind = q->__doctor_tier_kind(reinterpret_cast<addr_t *>(ptr));
    live = q->is_valid_block(reinterpret_cast<addr_t *>(ptr));
    prov = q->has_provenance(reinterpret_cast<addr_t *>(ptr));
    present = q->present(reinterpret_cast<addr_t *>(ptr));
    cached = q->__is_cached(ptr);
  });
  if ( !got ) {
    const void *fa = __fault_addr;
    __d("  block        (FAULTED reading arena metadata @");
    __d_ptr(fa);
    __d("; skipped to keep the report alive)\n");
    return;
  }
  __d("  block        size=");
  __d_u(sz);
  __d("  allocator=");
  __d(__kind_name(kind));
  __d("  live=");
  __d(live ? "yes" : "no");
  __d("  cached=");
  __d(cached ? "yes(freed-but-parked)" : "no");
  __d("  present=");
  __d(present ? "yes" : "no");
  __d("  provenance=");
  __d(prov ? "yes" : "no");
  __d_nl();

  if ( prov && sz > 0 && kind != 0 )
    __decode_header(ptr, sz, kind, live, s);      // guards internally; we are unarmed here
  else
    __d("  header       (not decoded: pointer outside any live sheet / interior / reclaimed)\n");
}

// scheduler / signal-mask contexts
inline const char *
__sig_name(unsigned sig) noexcept
{
  static const char *const names[32] = { nullptr, "HUP",  "INT",  "QUIT", "ILL",    "TRAP",   "ABRT",  "BUS",  "FPE",  "KILL", "USR1",
                                         "SEGV",  "USR2", "PIPE", "ALRM", "TERM",   "STKFLT", "CHLD",  "CONT", "STOP", "TSTP", "TTIN",
                                         "TTOU",  "URG",  "XCPU", "XFSZ", "VTALRM", "PROF",   "WINCH", "IO",   "PWR",  "SYS" };
  return sig < 32 ? names[sig] : nullptr;
}

inline void
__d_sigset64(u64 mask) noexcept
{
  __d_hex64(mask);
  if ( !mask ) {
    __d("  (none)");
    return;
  }
  __d("  [");
  bool first = true;
  for ( unsigned sig = 1; sig <= 64; ++sig ) {
    if ( !(mask & (1ULL << (sig - 1))) ) continue;
    if ( !first ) __d(" ");
    first = false;
    const char *nm = __sig_name(sig);
    if ( nm )
      __d(nm);
    else {
      __d("rt");
      __d_u(sig);
    }
  }
  __d("]");
}

inline void
__dump_sched_sig_context(const u64 *at_fault) noexcept
{
  __d("  thread       tid ");
  __d_tid(__gettid());
  __d_nl();

  __d("  cpu now      ");
  u32 cpu = ~0u, node = ~0u;
  if ( micron::syscall(SYS_getcpu, &cpu, &node, nullptr) == 0 ) {
    __d("#");
    __d_u(cpu);
    __d(" (node ");
    __d_u(node);
    __d(")");
  } else
    __d("(unavailable)");

  __d("   affinity ");
  u64 aff[16] = {};
  const long ar = static_cast<long>(micron::syscall(SYS_sched_getaffinity, 0, sizeof(aff), aff));
  if ( ar > 0 && static_cast<usize>(ar) <= sizeof(aff) && (ar % 8) == 0 ) {
    const usize words = static_cast<usize>(ar) / 8;
    usize top = words;
    while ( top > 1 && aff[top - 1] == 0 ) --top;
    const auto nib = [](unsigned x) -> char { return static_cast<char>(x < 10 ? ('0' + x) : ('a' + x - 10)); };
    __d("0x");
    for ( usize w = top; w-- > 0; ) {
      char buf[16];
      for ( int i = 0; i < 16; ++i ) buf[i] = nib(static_cast<unsigned>((aff[w] >> (60 - 4 * i)) & 0xF));
      abc::__write(buf, 16);
    }
    u64 ncpu = 0;
    for ( usize w = 0; w < words; ++w )
      for ( u64 x = aff[w]; x; x &= x - 1 ) ++ncpu;      // hand-rolled: no libgcc popcount in freestanding
    __d(" (");
    __d_u(ncpu);
    __d(" cpus, kernel window ");
    __d_u(static_cast<u64>(ar));
    __d(" B)");
  } else
    __d("(unavailable)");
  __d_nl();

  u64 blocked = 0, pending = 0;
  micron::syscall(SYS_rt_sigprocmask, micron::posix::sig_block, nullptr, &blocked, micron::posix::__sig_syscall_size);
  micron::syscall(SYS_rt_sigpending, &pending, micron::posix::__sig_syscall_size);
  if ( at_fault ) {
    __d("  sigmask      at-fault ");
    __d_sigset64(*at_fault);
    __d_nl();
    __d("               blocked  ");
  } else
    __d("  sigmask      blocked  ");
  __d_sigset64(blocked);
  __d_nl();
  __d("               pending  ");
  __d_sigset64(pending);
  __d_nl();
}

inline void
__fault_head(const char *what, const char *file, int line) noexcept
{
  __banner("FAULT: ");
  __d(what);
  __d("\n  detected at  ");
  __d(file);
  __d(":");
  __d_i(line);
  __d_nl();
  __dump_sched_sig_context(nullptr);
}

// non-printing header validity check
inline bool
__check_header_ok(byte *user, usize user_size, int kind) noexcept
{
  if ( kind == 1 ) {
    u32 bsize = 0;
    i32 flags = 0;
    __builtin_memcpy(&bsize, user - __hdr_offset, sizeof(bsize));
    __builtin_memcpy(&flags, user - __hdr_offset + 4, sizeof(flags));
    return bsize == static_cast<u32>(user_size + __hdr_offset) && __alloc_flags_ok(flags);
  }
  if ( kind == 2 ) {
    i32 order = 0;
    i32 flags = 0;
    __builtin_memcpy(&order, user + user_size, sizeof(order));
    __builtin_memcpy(&flags, user + user_size + 4, sizeof(flags));
    return order >= 0 && order <= 25 && __alloc_flags_ok(flags);
  }
  return true;
}

inline __rec *
__find_preceding_live(uintptr_t p, usize max_gap) noexcept
{
  if ( !__dr.slots ) return nullptr;
  __rec *best = nullptr;
  uintptr_t best_end = 0;
  for ( usize i = 0; i < __dr.cap; ++i ) {
    __rec &r = __dr.slots[i];
    if ( r.state != __rec_state::live ) continue;
    const uintptr_t end = r.key + r.req_size;
    if ( end <= p && (p - end) <= max_gap && end >= best_end ) {
      best = &r;
      best_end = end;
    }
  }
  return best;
}

inline void
__note_overflow_source(byte *u) noexcept
{
  __rec *src = __find_preceding_live(reinterpret_cast<uintptr_t>(u), 4096);
  if ( !src ) return;
  const usize gap = reinterpret_cast<uintptr_t>(u) - (src->key + src->req_size);
  __d("            probable source: live block @");
  __d_ptr(reinterpret_cast<const void *>(src->key));
  __d(" (");
  __d_u(src->req_size);
  __d(" B, alloc op#");
  __d_u(src->alloc_op);
  __d(") ends ");
  __d_u(gap);
  __d(" B before -- check its write bounds\n");
  __print_backtrace("          source allocated at:", src->alloc_bt);
}

// sweep accumulator
struct __sweep_ctx {
  usize arenas = 0;
  usize sheets = 0;
  usize live_checked = 0;
  usize blocks = 0;              // physical blocks walked (deep sweep)
  usize freelist_nodes = 0;      // free-list nodes walked (deep sweep)
  usize anomalies = 0;
  usize repairs = 0;
  usize shown = 0;
  usize skipped_arenas = 0;         // other-thread arenas NOT deep-walked (reading them would race the owner)
  usize cross_thread_live = 0;      // live ledger records owned by another thread's arena (metadata not probed)
  bool repair = false;              // set per-arena: rescue && !conservative && current thread's arena
  static constexpr usize __max_show = 48;

  void
  note(const char *what, const void *addr) noexcept
  {
    ++anomalies;
    if ( shown < __max_show ) {
      ++shown;
      __d("  ANOMALY   ");
      __d(what);
      __d("  @");
      __d_ptr(addr);
      __d_nl();
    }
  }

  void
  did_repair(const char *what, const void *addr) noexcept
  {
    ++repairs;
    if ( shown < __max_show ) {
      ++shown;
      __d("  REPAIR    ");
      __d(what);
      __d("  @");
      __d_ptr(addr);
      __d_nl();
    }
  }
};

// the full all-arenas metadata + arena-health sweep
inline void
__sweep_all_locked(const char *cause, bool verbose) noexcept
{
  __sweep_ctx ctx;
  __install_fault_handler();
  __banner("SWEEP (");
  __d(cause);
  __d(")\n");

  __arena *const self = __tls_arena;
  __for_each_live_arena([&](__arena &a) {
    if ( &a != self ) {
      ++ctx.arenas;
      ++ctx.skipped_arenas;
      return;
    }
    ctx.repair = __default_doctor_rescue && !__default_doctor_rescue_conservative;
    if ( !__guard_read([&] {
           a.__doctor_check_struct(ctx);
           a.__doctor_walk_blocks(ctx);
         }) )
      ctx.note("deep walk FAULTED (corrupt sheet metadata); aborted this arena's walk", self);
  });
  ctx.repair = false;

  if ( __dr.slots ) {
    for ( usize i = 0; i < __dr.cap; ++i ) {
      __rec &r = __dr.slots[i];
      if ( r.state != __rec_state::live ) continue;
      byte *u = reinterpret_cast<byte *>(r.key);
      ++ctx.live_checked;
      if ( !__va_contains(u) ) {
        ctx.note("live record: pointer not in abcmalloc VA region", u);
        continue;
      }
      __arena *o = __owner_of(u);
      if ( !o ) {
        ctx.note("live record: granule has no owner (sheet released underneath?)", u);
        continue;
      }
      if ( o != self ) {
        ++ctx.cross_thread_live;
        continue;
      }
      const bool ok = __guard_read([&] {
        if ( !o->has_provenance(reinterpret_cast<addr_t *>(u)) ) {
          ctx.note("live record: arena lost provenance", u);
          return;
        }
        const usize sz = o->__size_of_alloc(reinterpret_cast<addr_t *>(u));
        const int kind = o->__doctor_tier_kind(reinterpret_cast<addr_t *>(u));
        if ( !o->is_valid_block(reinterpret_cast<addr_t *>(u)) && !o->__is_cached(u) )
          ctx.note("live record: allocator reports block NOT live (use-after-free / silent free)", u);
        const bool hdr_struct_ok = !(kind && sz) || __check_header_ok(u, sz, kind);
        if ( kind && sz && !hdr_struct_ok ) ctx.note("live record: block metadata header INVALID (corruption)", u);
        if ( r.owner && o != r.owner )
          ctx.note("live record: granule owner changed since alloc (sheet released + re-registered under a live block)", u);
        if ( sz && r.req_size > sz )
          ctx.note("live record: recorded request exceeds the real block size (ledger/allocator disagree -- corruption)", u);
        if constexpr ( __default_doctor_canary ) {
          bool hdr_shadow_ok = true;
          if ( kind && r.hdr_shadow ) {
            u32 cur = 0;
            if ( kind == 1 )
              __builtin_memcpy(&cur, u - __hdr_offset, sizeof(cur));
            else {
              __builtin_memcpy(&cur, u + sz, sizeof(cur));
              cur = __hdr_shadow_buddy_captured | cur;      // match the captured marker armed for buddy blocks
            }
            if ( cur != r.hdr_shadow ) {
              hdr_shadow_ok = false;
              ctx.note("live record: header structural field changed since alloc (metadata overwrite)", u);
              __note_overflow_source(u);
            }
          }
          if constexpr ( !ABC_EFF_REDZONE ) {
            if ( hdr_struct_ok && hdr_shadow_ok && sz > r.req_size ) {
              for ( usize k = r.req_size; k < sz; ++k )
                if ( u[k] != __default_doctor_canary_byte ) {
                  ctx.note("live record: slack canary clobbered (right buffer overflow past requested size)", u);
                  __note_overflow_source(u);
                  break;
                }
            }
          }
        }
      });
      if ( !ok ) ctx.note("live record: FAULTED probing block metadata (unmapped page / wild pointer)", u);
    }
  }

  __d("  swept: arenas=");
  __d_u(ctx.arenas);
  __d(" sheets=");
  __d_u(ctx.sheets);
  __d(" blocks=");
  __d_u(ctx.blocks);
  __d(" freelist_nodes=");
  __d_u(ctx.freelist_nodes);
  __d(" live_recs=");
  __d_u(ctx.live_checked);
  if ( ctx.skipped_arenas ) {
    __d(" other_arenas=");
    __d_u(ctx.skipped_arenas);
    __d("(not walked: cross-thread)");
  }
  if ( ctx.cross_thread_live ) {
    __d(" cross_thread_live=");
    __d_u(ctx.cross_thread_live);
  }
  __d(" anomalies=");
  __d_u(ctx.anomalies);
  __d(" repairs=");
  __d_u(ctx.repairs);
  if ( ctx.anomalies + ctx.repairs > ctx.shown ) {
    __d(" (");
    __d_u(ctx.anomalies + ctx.repairs - ctx.shown);
    __d(" more not shown)");
  }
  __d_nl();
  (void)verbose;
}

inline void
__run_sweep_locked(const char *cause) noexcept
{
  __sweep_all_locked(cause, false);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rescue primitives

// mark a tracked pointer deliberately leaked (excluded from the live/leak set)
inline void
__quarantine_locked(byte *ptr) noexcept
{
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( !s ) return;
  if ( s->state == __rec_state::live && __dr.n_live ) --__dr.n_live;
  s->state = __rec_state::quarantined;
}

// aggressive in-place header repair
// rewrite the block metadata header to its expected live form
inline bool
__repair_header_locked(byte *user, usize user_size, int kind) noexcept
{
  if ( kind == 1 ) {
    i32 old = 0;
    __builtin_memcpy(&old, user - __hdr_offset + 4, sizeof(old));
    u32 bsize = static_cast<u32>(user_size + __hdr_offset);
    i32 flags = __block_alloc | (old & __block_temporal);      // preserve temporal (rings still reference it)
    __builtin_memcpy(user - __hdr_offset, &bsize, sizeof(bsize));
    __builtin_memcpy(user - __hdr_offset + 4, &flags, sizeof(flags));
    __d("  RESCUE       rewrote tlsf_hdr {bsize,flags} to live form (temporal bit preserved)\n");
    return __check_header_ok(user, user_size, kind);
  }
  if ( kind == 2 ) {
    i32 old = 0;
    __builtin_memcpy(&old, user + user_size + 4, sizeof(old));
    i32 flags = __block_alloc | (old & __block_temporal);      // preserve temporal (rings still reference it)
    __builtin_memcpy(user + user_size + 4, &flags, sizeof(flags));
    __d("  RESCUE       rewrote buddy block_header.flags to allocated form (temporal bit preserved)\n");
    return __check_header_ok(user, user_size, kind);
  }
  return false;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// "most likely fix" suggestions

inline void
__help(const char *s) noexcept
{
  __d("  help:  ");
  __d(s);
  __d_nl();
}

inline void
__note(const char *s) noexcept
{
  __d("       = note: ");
  __d(s);
  __d_nl();
}

// scan the ledger for a LIVE record whose [key, key+req_size) strictly contains p
inline __rec *
__find_containing_live(uintptr_t p) noexcept
{
  if ( !__dr.slots ) return nullptr;
  for ( usize i = 0; i < __dr.cap; ++i ) {
    __rec &r = __dr.slots[i];
    if ( r.state != __rec_state::live ) continue;
    if ( r.key == p ) continue;      // an exact base is not "interior"
    if ( p > r.key && p < r.key + r.req_size ) return &r;
  }
  return nullptr;
}

inline __rec *
__find_containing_any(uintptr_t p) noexcept
{
  if ( !__dr.slots ) return nullptr;
  for ( usize i = 0; i < __dr.cap; ++i ) {
    __rec &r = __dr.slots[i];
    if ( r.state == __rec_state::empty || r.key == 0 ) continue;
    if ( p >= r.key && p < r.key + r.req_size ) return &r;
  }
  return nullptr;
}

#if defined(__micron_arch_amd64)
inline void
__d_hex16(u16 v) noexcept
{
  char buf[6];
  const auto nib = [](unsigned x) -> char { return static_cast<char>(x < 10 ? ('0' + x) : ('a' + x - 10)); };
  buf[0] = '0';
  buf[1] = 'x';
  for ( int i = 0; i < 4; ++i ) buf[2 + i] = nib(static_cast<unsigned>((v >> (12 - 4 * i)) & 0xF));
  abc::__write(buf, 6);
}

inline void
__d_reg(const char *name, u64 v) noexcept
{
  __d(name);
  __d_hex64(v);
  __d("  ");
}

inline void
__d_eflags(u64 f) noexcept
{
  __d("    eflags ");
  __d_hex64(f);
  __d("  [");

  static const struct {
    u64 bit;
    const char *nm;
  } fl[] = { { 1u << 0, "CF" }, { 1u << 2, "PF" },  { 1u << 4, "AF" },  { 1u << 6, "ZF" },  { 1u << 7, "SF" }, { 1u << 8, "TF" },
             { 1u << 9, "IF" }, { 1u << 10, "DF" }, { 1u << 11, "OF" }, { 1u << 16, "RF" }, { 1u << 18, "AC" } };

  bool first = true;
  for ( const auto &x : fl )
    if ( f & x.bit ) {
      if ( !first ) __d(" ");
      first = false;
      __d(x.nm);
    }
  __d("]");
  __d_nl();
}

inline void
__d_fault_regs(const __dr_ucontext &u) noexcept
{
  const __dr_sigcontext &m = u.mc;
  __d("  registers    (ucontext at fault)\n");
  __d("    ");
  __d_reg("rip ", m.rip);
  __d_reg("rsp ", m.rsp);
  __d_reg("rbp ", m.rbp);
  __d_nl();
  __d("    ");
  __d_reg("rax ", m.rax);
  __d_reg("rbx ", m.rbx);
  __d_reg("rcx ", m.rcx);
  __d_nl();
  __d("    ");
  __d_reg("rdx ", m.rdx);
  __d_reg("rsi ", m.rsi);
  __d_reg("rdi ", m.rdi);
  __d_nl();
  __d("    ");
  __d_reg("r8  ", m.r8);
  __d_reg("r9  ", m.r9);
  __d_reg("r10 ", m.r10);
  __d_nl();
  __d("    ");
  __d_reg("r11 ", m.r11);
  __d_reg("r12 ", m.r12);
  __d_reg("r13 ", m.r13);
  __d_nl();
  __d("    ");
  __d_reg("r14 ", m.r14);
  __d_reg("r15 ", m.r15);
  __d_nl();
  __d_eflags(m.eflags);
  __d("    cs ");
  __d_hex16(m.cs);
  __d("  ss ");
  __d_hex16(m.ss);
  __d("  fs ");
  __d_hex16(m.fs);
  __d("  gs ");
  __d_hex16(m.gs);
  __d_nl();
  __d("    trapno ");
  __d_u(m.trapno);
  if ( m.trapno == 13 )
    __d(" (#GP)");
  else if ( m.trapno == 14 )
    __d(" (#PF)");
  else if ( m.trapno == 17 )
    __d(" (#AC)");
  __d("  err ");
  __d_hex64(m.err);
  if ( m.trapno == 14 ) {      // page-fault error code
    __d(" (");
    __d(m.err & 4 ? "user" : "kernel");
    __d((m.err & 16) ? " ifetch" : ((m.err & 2) ? " write" : " read"));
    __d((m.err & 1) ? ", protection violation" : ", page not present");
    if ( m.err & 8 ) __d(", rsvd-bit");
    if ( m.err & 32 ) __d(", pkey");
    __d(")");
  }
  __d("  cr2 ");
  __d_hex64(m.cr2);
  __d_nl();
  __d("    oldmask ");
  __d_hex64(m.oldmask);
  __d("  fpstate ");
  __d(m.fpstate ? "present" : "(null)");
  __d_nl();
}
#endif

inline void
__splat_fault_memory(const void *addr) noexcept
{
  if ( !addr ) {
    __d("  memory       (fault address null; window skipped)\n");
    return;
  }
  __d("  memory       [fault-8, fault+64) raw window:\n");
  __splat_window(addr, 64);
}

inline void
__report_hw_fault(int sig, const void *addr, const void *uctx) noexcept
{
  if constexpr ( __crash_safe_on ) {
    __banner("FAULT (hardware): ");
    __d(sig == micron::posix::sig_bus ? "SIGBUS" : "SIGSEGV");
    __d(" at ");
    __d_ptr(addr);
    __d_nl();
    __d("  in-VA        ");
    __d(__va_contains(addr) ? "yes (abcmalloc reserved region)" : "no (foreign / stack / static / libc)");
    __d_nl();
#if defined(__micron_arch_amd64)
    const __dr_ucontext *uc = static_cast<const __dr_ucontext *>(uctx);
    const u64 fmask = uc ? uc->sigmask : 0;
    __dump_sched_sig_context(uc ? &fmask : nullptr);
    if ( uc ) __d_fault_regs(*uc);
#else
    (void)uctx;
    __dump_sched_sig_context(nullptr);
#endif
    __splat_fault_memory(addr);
    if ( __va_contains(addr) ) {
      // non-blocking: if the ledger lock is held (by us mid-op, or another thread) just skip it
      if ( !__dr.lock.test_and_set(micron::memory_order_acquire) ) {
        __rec *f = __find_containing_any(reinterpret_cast<uintptr_t>(addr));
        if ( f ) {
          __d("  ledger       address is inside a ");
          __d(__state_name(f->state));
          __d(" block [base ");
          __d_ptr(reinterpret_cast<const void *>(f->key));
          __d(", ");
          __d_u(f->req_size);
          __d(" B]  alloc op#");
          __d_u(f->alloc_op);
          __d(" tid ");
          __d_i(f->alloc_tid);
          if ( f->state != __rec_state::live ) {
            __d("  |  USE-AFTER-FREE: freed op#");
            __d_u(f->free_op);
            __d(" tid ");
            __d_i(f->free_tid);
          }
          __d_nl();
          const uintptr_t hbase = f->key;
          int hkind = 0;
          usize husz = 0;
          if ( f->hdr_shadow && !(f->hdr_shadow & __hdr_shadow_buddy_captured) )
            hkind = 1;
          else {
            __arena *o = __owner_of(reinterpret_cast<const void *>(hbase));
            if ( o && o == __tls_arena ) {
              int k = 0;
              usize s2 = 0;
              if ( __guard_read([&] {
                     k = o->__doctor_tier_kind(reinterpret_cast<addr_t *>(hbase));
                     s2 = o->__size_of_alloc(reinterpret_cast<addr_t *>(hbase));
                   }) ) {
                hkind = k;
                husz = s2;
              }
            }
          }
          if ( hkind == 1 ) {
            __d("  header       tlsf_hdr @");
            __d_ptr(reinterpret_cast<const void *>(hbase - __hdr_offset));
            __d(" (base-32); raw [hdr-8, hdr+32):\n");
            __splat_window(reinterpret_cast<const void *>(hbase - __hdr_offset), __hdr_offset);
          } else if ( hkind == 2 && husz ) {
            __d("  header       block_header @");
            __d_ptr(reinterpret_cast<const void *>(hbase + husz));
            __d(" (base+");
            __d_u(husz);
            __d(", buddy tail); raw [hdr-8, hdr+32):\n");
            __splat_window(reinterpret_cast<const void *>(hbase + husz), __hdr_offset);
          } else
            __d("  header       (tier/tail unresolved for this block; header not splatted)\n");
        } else
          __d("  ledger       no tracked block contains this address (wild pointer / metadata)\n");
        __dr.lock.clear(micron::memory_order_release);
      }
    }
    __d("  action       doctor note emitted; chaining to the previous handler (normal crash proceeds)\n\n");
  } else {
    (void)sig;
    (void)addr;
    (void)uctx;
  }
}

// suggest a fix for a pointer that failed as a free target
inline void
__suggest_free_target(byte *ptr) noexcept
{
  if ( !__va_contains(ptr) ) {
    __help("this pointer was never returned by abcmalloc (in-VA=no); it looks like a stack / global / foreign-heap address");
    __note("do not free it here; verify the pointer actually came from abc::alloc / abc::malloc");
    return;
  }
  __rec *b = __find_containing_live(reinterpret_cast<uintptr_t>(ptr));
  if ( b ) {
    __d("  help:  free the BASE pointer ");
    __d_ptr(reinterpret_cast<void *>(b->key));
    __d(" instead -- this is +");
    __d_u(reinterpret_cast<uintptr_t>(ptr) - b->key);
    __d(" bytes into a live ");
    __d_u(b->req_size);
    __d("-byte allocation (op#");
    __d_u(b->alloc_op);
    __d(")\n");
    return;
  }
  if ( !__owner_of(ptr) )
    __help("pointer is inside the abcmalloc region but its sheet was released -- use-after-free of reclaimed memory");
  else
    __help("pointer is in a live sheet but is not a block start -- interior/misaligned pointer, or an already-freed block");
}

bool
on_double_free(byte *ptr, const char *file, int line) noexcept
{
  __reentry re;
  if ( !re ) return false;      // nested fault: don't recurse; let the existing handler proceed
  __scoped_lock g;
  __fault_head("double free / free of non-live block", file, line);
  __forensics(ptr, false);
  __run_sweep_locked("double-free");
  {
    const __rec *sd = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
    if ( sd && sd->state != __rec_state::live ) {
      __d("  help:  remove the duplicate free -- already freed at op#");
      __d_u(sd->free_op);
      __d(" by tid ");
      __d_i(sd->free_tid);
      __d("; null the pointer after the first free (or check for a double ownership transfer)\n");
    } else
      // no exact freed record: could be an interior pointer or a foreign address
      __suggest_free_target(ptr);
  }
  if constexpr ( __default_doctor_rescue ) {
    // the block is provably not live (that is why we are here)
    // dropping the duplicate free mutates no allocator state and is always safe
    __quarantine_locked(ptr);
    __d("  RESCUE       duplicate free DROPPED (no-op); abort suppressed\n\n");
    return true;
  }
  __d("  action       report emitted; existing handle_double_free proceeds (abort/return per config)\n\n");
  return false;
}

void
on_free_result(byte *ptr, bool ok, const char *file, int line) noexcept
{
  if ( ok ) return;      // normal free
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  // a double free that went through the tier path was already fully reported by on_double_free
  const __rec *sd = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  if ( sd && sd->state != __rec_state::live ) return;
  __fault_head("unrecognised free (address not found in any tier)", file, line);
  __forensics(ptr, false);
  __run_sweep_locked("unknown-free");
  __suggest_free_target(ptr);
  __d("  action       report emitted; free returns false (existing, non-fatal)\n\n");
}

bool
on_corruption(byte *ptr, const char *kind, const char *file, int line) noexcept
{
  __reentry re;
  if ( !re ) return false;
  __dr.corruption_events.fetch_add(1, micron::memory_order_relaxed);
  __scoped_lock g;
  __banner("FAULT: corruption (");
  __d(kind);
  __d(")\n  detected at  ");
  __d(file);
  __d(":");
  __d_i(line);
  __d_nl();
  __dump_sched_sig_context(nullptr);
  __forensics(ptr, true);      // corruption: the alloc-site backtrace is warranted
  const __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  usize usz = s ? s->req_size : 0;
  if ( __va_contains(ptr) )
    if ( !__guard_read([&] { __dump_redzones(ptr, usz); }) ) __d("  redzone      (FAULTED reading canary region; skipped)\n");
  __run_sweep_locked("corruption");
  __help("a neighboring write likely overran into this block's metadata header; check the preceding allocation's bounds (buffer overflow)");
  if constexpr ( __default_doctor_rescue ) {
    if constexpr ( !__default_doctor_rescue_conservative ) {
      // aggressive: attempt an in-place structural repair of the block header
      // WARNING: only touch our own arena; repairing another thread's live arena races it
      __arena *o = __va_contains(ptr) ? __owner_of(ptr) : nullptr;
      if ( o && o == __tls_arena && usz ) {
        const int kind2 = o->__doctor_tier_kind(reinterpret_cast<addr_t *>(ptr));
        const usize sz2 = o->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
        if ( kind2 && sz2 && __repair_header_locked(ptr, sz2, kind2) ) {
          __d("  RESCUE       header repaired + validated; error suppressed\n\n");
          return true;
        }
      }
    }
    // conservative, or repair could not converge: quarantine (leak) the block
    __quarantine_locked(ptr);
    __d("  RESCUE       could not repair -> quarantined (leaked); error suppressed\n\n");
    return true;
  }
  __d("  action       report emitted; existing handler proceeds (fail_state/abort per config)\n\n");
  return false;
}

bool
on_bad_free(byte *ptr, usize len, const char *what, const char *file, int line) noexcept
{
  __reentry re;
  if ( !re ) return false;
  __scoped_lock g;
  __fault_head(what, file, line);
  __d("  call size    len=");
  __d_u(len);
  __d_nl();
  __forensics(ptr, false);
  __run_sweep_locked("bad-free");
  __suggest_free_target(ptr);
  if constexpr ( __default_doctor_rescue ) {
    // not routable to a valid free: quarantine (leak) rather than abort/throw
    __quarantine_locked(ptr);
    __d("  RESCUE       quarantined (leaked); exception/abort suppressed\n\n");
    return true;
  }
  __d("  action       report emitted; existing exception/handler proceeds\n\n");
  return false;
}

// cross-sized-free validation
usize
check_free_size(byte *ptr, usize claimed, const char *file, int line) noexcept
{
  if ( !ptr || claimed == 0 ) return claimed;      // size-less free: nothing to validate
  __reentry re;
  if ( !re ) return claimed;      // nested: don't recurse; accept the claimed size unchanged
  __scoped_lock g;
  __rec *s = __dr.__find(reinterpret_cast<uintptr_t>(ptr));
  const usize req = s ? s->req_size : 0;
  usize real = 0;
  if ( __va_contains(ptr) ) {
    __arena *o = __owner_of(ptr);
    if ( !o ) o = __tls_arena;
    // only read __size_of_alloc on our own arena; another thread's arena is racing us
    if ( o && o == __tls_arena ) real = o->__size_of_alloc(reinterpret_cast<addr_t *>(ptr));
  }
  // with redzone off, any claimed size within the block is safe
  if constexpr ( __default_redzone ) {
    if ( (req && claimed == req) || (real && claimed == real) ) return claimed;
  } else {
    if ( (req && claimed <= req) || (real && claimed <= real) ) return claimed;
  }
  if ( !s && real == 0 ) return claimed;      // untracked + unknown block: leave it to on_bad_free

  __fault_head("wrong-sized free (dealloc size does not match the allocation)", file, line);
  __d("  claimed      ");
  __d_u(claimed);
  __d("   recorded req_size ");
  if ( s )
    __d_u(req);
  else
    __d("(untracked)");
  __d("   real block size ");
  __d_u(real);
  __d_nl();
  __forensics(ptr, false);
  __run_sweep_locked("size-mismatch");
  __d("  help:  free with size ");
  __d_u(real ? real : req);
  __d(" -- or use the size-less abc::free(ptr)\n");
  if ( real && claimed > real )
    __note("claimed > block size: the size-based free scrub (poison/zero-on-free) would overrun the block -> heap corruption");
  if constexpr ( __default_doctor_rescue ) {
    const usize safe = (real && claimed > real) ? real : claimed;
    __d("  RESCUE       clamped free size to ");
    __d_u(safe);
    __d("; continuing\n\n");
    return safe;
  }
  const usize safe = (real && claimed > real) ? real : claimed;
  __d("  action       wrong-sized free -> fail_state() per __default_fail_result (size clamped to ");
  __d_u(safe);
  __d(")\n\n");
  abc::fail_state();      // aborts under fail_result 0/1; returns under mode 2
  return safe;            // fail_result==2: proceed with the clamped-safe size so the scrub cannot overrun
}

inline void
fsck(void)
{
  __reentry re;
  if ( !re ) return;
  __scoped_lock g;
  __sweep_all_locked("manual fsck", true);
}

inline void
audit(void)
{
  fsck();
}

inline bool
__selftest_ledger_offheap(void) noexcept
{
  __reentry re;
  if ( !re ) return true;
  __scoped_lock g;
  return !__dr.slots || !__va_contains(__dr.slots);
}

inline bool
__selftest_crash_safe(void) noexcept
{
  if constexpr ( !__crash_safe_on ) {
    return true;
  } else {
    __install_fault_handler();
    volatile int sink = 0;
    const bool completed = __guard_read([&] { sink = *reinterpret_cast<volatile int *>(0x10); });
    (void)sink;
    return !completed;
  }
}

struct __leak_reporter {
  ~__leak_reporter() noexcept
  {
    if constexpr ( __default_doctor_leak_report_at_exit ) {
      __banner("process-exit leak report (live set includes allocator-internal blocks)\n");
      leaks();
      stats();
    }
  }
};

inline __leak_reporter __leak_reporter_instance{};

};      // namespace doctor
};      // namespace abc

#endif      // ABCMALLOC_DOCTOR_HELP
