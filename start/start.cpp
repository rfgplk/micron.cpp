//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "__auxv.hpp"
#include "__crt.hpp"
#include "__stack.hpp"
#include "__tls.hpp"

#include <micron/exit.hpp>      // micron::exit + __exit_internal::__push

// call user declared main from out __micron_user_main
// NOTE: we need to surpress Wodr because the compiler complains about multiple main definitions; we're okay though
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wodr"
extern "C" int __micron_user_main(int argc, char **argv, char **envp) __asm__("main");
// extern "C" void __boot_abcmalloc(void);
extern "C" void __boot_io_buffers(void);
#pragma GCC diagnostic pop

extern "C" {
char **environ = nullptr;
}

extern "C" __attribute__((used, visibility("default"))) int
__micron_startc(int argc, char **argv, char **envp, const micron::auxv_t *auxv) noexcept
{
  environ = envp;
  // stub for now, just so we have thread_local working, we don't support multithreading in freestanding mode YET
  micron::__tls_init(auxv);

  // OBSOLETE abcmalloc doesnt' depend on runtime initialized code so it fires first
  // __boot_abcmalloc();

  // get the primary stack region
  micron::__stack_init(auxv);

  // manually start __attribute__((constructor)), functions here don't go through .init_array
  for ( void (**p)(void) = __preinit_array_start; p < __preinit_array_end; ++p ) (*p)();
  for ( void (**p)(void) = __init_array_start; p < __init_array_end; ++p ) (*p)();

  // io buffer init MUST fire AFTER .init_array
  __boot_io_buffers();
  const int rc = __micron_user_main(argc, argv, envp);

  micron::exit(rc);
}

#if defined(__micron_arch_width_64)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

extern "C" __attribute__((used, optimize("-fno-tree-loop-distribute-patterns"))) unsigned __int128
__udivti3(unsigned __int128 n, unsigned __int128 d) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned __int128 q = 0;
  unsigned __int128 r = 0;
  for ( int i = 0; i < 128; ++i ) {
    r = (r << 1) | static_cast<unsigned __int128>(n >> 127);
    n <<= 1;
    q <<= 1;
    if ( r >= d ) {
      r -= d;
      q |= 1;
    }
  }
  return q;
}

extern "C" __attribute__((used, optimize("-fno-tree-loop-distribute-patterns"))) unsigned __int128
__umodti3(unsigned __int128 n, unsigned __int128 d) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned __int128 r = 0;
  for ( int i = 0; i < 128; ++i ) {
    r = (r << 1) | static_cast<unsigned __int128>(n >> 127);
    n <<= 1;
    if ( r >= d ) r -= d;
  }
  return r;
}

#pragma GCC diagnostic pop
#endif

extern "C" {

int
__cxa_atexit(void (*dtor)(void *), void *arg, void * /*dso_handle*/) noexcept
{
  if ( dtor == nullptr ) return -1;
  return micron::__exit_internal::__push(dtor, arg);
}

// WARNING: thread_local objects with a non-trivial dtor emit a call to __cxa_thread_atexit (Itanium C++ ABI)
// micron currently doesn't support multithreading in freestanding mode
// so a thread_local's lifetime IS the process lifetime
int
__cxa_thread_atexit(void (*dtor)(void *), void *arg, void * /*dso_handle*/) noexcept
{
  if ( dtor == nullptr ) return -1;
  return micron::__exit_internal::__push(dtor, arg);
}

int
__cxa_guard_acquire(long long int *g)
{
  return *reinterpret_cast<unsigned char *>(g) == 0;
}

void
__cxa_guard_release(long long int *g)
{
  *reinterpret_cast<unsigned char *>(g) = 1;
}

void
__cxa_guard_abort(long long int *)
{
}

// __dso_handle is referenced by every TU that schedules a global destructor via __cxa_atexit
__attribute__((used, visibility("hidden"))) void *__dso_handle = &__dso_handle;

}      // extern "C"

#if defined(__micron_arch_arm32)

// armv7-a does not have hardware 64-bit divide, 64-bit count-trailing-zeros,
// 64-bit signed/unsigned <-> float/double conversions, or hardware double-
// precision FMA. GCC emits AEABI helper calls for all of these. libgcc would
// satisfy them, but micron is freestanding so we provide our own

namespace
{
[[gnu::always_inline]] inline void
__udivmod64(unsigned long long n, unsigned long long d, unsigned long long &q_out, unsigned long long &r_out) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned long long q = 0;
  unsigned long long r = 0;
  for ( int i = 0; i < 64; ++i ) {
    r = (r << 1) | (n >> 63);
    n <<= 1;
    q <<= 1;
    if ( r >= d ) {
      r -= d;
      q |= 1;
    }
  }
  q_out = q;
  r_out = r;
}

[[gnu::always_inline]] inline void
__udivmod32(unsigned int n, unsigned int d, unsigned int &q_out, unsigned int &r_out) noexcept
{
  if ( d == 0 ) __builtin_trap();
  unsigned int q = 0;
  unsigned int r = 0;
  for ( int i = 0; i < 32; ++i ) {
    r = (r << 1) | (n >> 31);
    n <<= 1;
    q <<= 1;
    if ( r >= d ) {
      r -= d;
      q |= 1;
    }
  }
  q_out = q;
  r_out = r;
}
}      // anonymous namespace

extern "C" {

__attribute__((used)) int
__aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) noexcept
{
  return __cxa_atexit(destructor, object, dso_handle);
}

// NOTE: AEABI fns must use the soft-float core-register PCS regardless of -mfloat-abi
// without pcs("aapcs") a hard-float build receives float/double args in s0/d0
__attribute__((used, weak, pcs("aapcs"))) double
fma(double a, double b, double c) noexcept
{
  return a * b + c;
}

__attribute__((used, weak, pcs("aapcs"))) float
fmaf(float a, float b, float c) noexcept
{
  return a * b + c;
}

__attribute__((used)) unsigned long long
__udivdi3(unsigned long long n, unsigned long long d) noexcept
{
  unsigned long long q;
  unsigned long long r;
  __udivmod64(n, d, q, r);
  return q;
}

__attribute__((used)) unsigned long long
__umoddi3(unsigned long long n, unsigned long long d) noexcept
{
  unsigned long long q;
  unsigned long long r;
  __udivmod64(n, d, q, r);
  return r;
}

__attribute__((used)) long long
__divdi3(long long n, long long d) noexcept
{
  const bool neg = (n < 0) ^ (d < 0);
  const unsigned long long un = (n < 0) ? -static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  const unsigned long long ud = (d < 0) ? -static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long q;
  unsigned long long r;
  __udivmod64(un, ud, q, r);
  return neg ? -static_cast<long long>(q) : static_cast<long long>(q);
}

__attribute__((used)) long long
__moddi3(long long n, long long d) noexcept
{
  const bool neg_n = n < 0;
  const unsigned long long un = neg_n ? -static_cast<unsigned long long>(n) : static_cast<unsigned long long>(n);
  const unsigned long long ud = (d < 0) ? -static_cast<unsigned long long>(d) : static_cast<unsigned long long>(d);
  unsigned long long q;
  unsigned long long r;
  __udivmod64(un, ud, q, r);
  return neg_n ? -static_cast<long long>(r) : static_cast<long long>(r);
}

__attribute__((used)) unsigned int
__aeabi_uidiv(unsigned int n, unsigned int d) noexcept
{
  unsigned int q;
  unsigned int r;
  __udivmod32(n, d, q, r);
  return q;
}

__attribute__((used)) int
__aeabi_idiv(int n, int d) noexcept
{
  const bool neg = (n < 0) ^ (d < 0);
  const unsigned int un = (n < 0) ? -static_cast<unsigned int>(n) : static_cast<unsigned int>(n);
  const unsigned int ud = (d < 0) ? -static_cast<unsigned int>(d) : static_cast<unsigned int>(d);
  unsigned int q;
  unsigned int r;
  __udivmod32(un, ud, q, r);
  return neg ? -static_cast<int>(q) : static_cast<int>(q);
}

__attribute__((naked, used)) void
__aeabi_uldivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, r6, r7, r8, lr}\n"
                   "mov    r4, r0\n"         // r4 = num_lo
                   "mov    r5, r1\n"         // r5 = num_hi
                   "mov    r6, r2\n"         // r6 = den_lo
                   "mov    r7, r3\n"         // r7 = den_hi
                   "bl     __udivdi3\n"      // r0:r1 = quot (lo:hi)
                   "mov    r8, r0\n"         // save quot_lo
                   // truncated 64x64 -> 64 multiply: r2:r3 = quot * den (mod 2^64)
                   "umull  r2, r3, r0, r6\n"      // r3:r2 = quot_lo * den_lo
                   "mla    r3, r1, r6, r3\n"      // r3 += quot_hi * den_lo (low 32)
                   "mla    r3, r0, r7, r3\n"      // r3 += quot_lo * den_hi (low 32)
                   // r2:r3 = num - product
                   "subs   r2, r4, r2\n"
                   "sbc    r3, r5, r3\n"
                   "mov    r0, r8\n"      // restore quot_lo
                   // r1 still holds quot_hi (untouched by mla/umull on r0/r1 inputs)
                   "pop    {r4, r5, r6, r7, r8, pc}\n");
}

__attribute__((naked, used)) void
__aeabi_ldivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, r6, r7, r8, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "mov    r6, r2\n"
                   "mov    r7, r3\n"
                   "bl     __divdi3\n"
                   "mov    r8, r0\n"
                   "umull  r2, r3, r0, r6\n"
                   "mla    r3, r1, r6, r3\n"
                   "mla    r3, r0, r7, r3\n"
                   "subs   r2, r4, r2\n"
                   "sbc    r3, r5, r3\n"
                   "mov    r0, r8\n"
                   "pop    {r4, r5, r6, r7, r8, pc}\n");
}

__attribute__((naked, used)) void
__aeabi_idivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, lr}\n"
                   "mov    r4, r0\n"      // num
                   "mov    r5, r1\n"      // den
                   "bl     __aeabi_idiv\n"
                   "mul    r1, r0, r5\n"      // r1 = quot * den (truncated)
                   "sub    r1, r4, r1\n"      // r1 = num - quot*den = remainder
                   "pop    {r4, r5, pc}\n");
}

__attribute__((naked, used)) void
__aeabi_uidivmod() noexcept
{
  __asm__ volatile(".syntax unified\n"
                   "push   {r4, r5, lr}\n"
                   "mov    r4, r0\n"
                   "mov    r5, r1\n"
                   "bl     __aeabi_uidiv\n"
                   "mul    r1, r0, r5\n"
                   "sub    r1, r4, r1\n"
                   "pop    {r4, r5, pc}\n");
}

__attribute__((used)) int
__ctzdi2(unsigned long long x) noexcept
{
  const unsigned int lo = static_cast<unsigned int>(x);
  if ( lo ) return __builtin_ctz(lo);
  const unsigned int hi = static_cast<unsigned int>(x >> 32);
  if ( hi ) return 32 + __builtin_ctz(hi);
  return 64;
}

__attribute__((used, pcs("aapcs"))) double
__aeabi_ul2d(unsigned long long x) noexcept
{
  const unsigned int hi = static_cast<unsigned int>(x >> 32);
  const unsigned int lo = static_cast<unsigned int>(x);
  return static_cast<double>(hi) * 4294967296.0 + static_cast<double>(lo);
}

__attribute__((used, pcs("aapcs"))) double
__aeabi_l2d(long long x) noexcept
{
  if ( x < 0 ) return -__aeabi_ul2d(static_cast<unsigned long long>(-x));
  return __aeabi_ul2d(static_cast<unsigned long long>(x));
}

__attribute__((used, pcs("aapcs"))) float
__aeabi_ul2f(unsigned long long x) noexcept
{
  return static_cast<float>(__aeabi_ul2d(x));
}

__attribute__((used, pcs("aapcs"))) float
__aeabi_l2f(long long x) noexcept
{
  return static_cast<float>(__aeabi_l2d(x));
}

__attribute__((used, pcs("aapcs"))) unsigned long long
__aeabi_d2ulz(double x) noexcept
{
  if ( !(x >= 0.0) ) return 0;      // negatives and NaN saturate low
  if ( x >= 18446744073709551616.0 ) return ~0ULL;
  if ( x < 4294967296.0 ) return static_cast<unsigned int>(x);      // single VCVT.U32.F64
  unsigned int hi = static_cast<unsigned int>(x * (1.0 / 4294967296.0));
  double lo_d = x - static_cast<double>(hi) * 4294967296.0;
  if ( lo_d < 0.0 ) {
    --hi;
    lo_d += 4294967296.0;
  } else if ( lo_d >= 4294967296.0 ) {
    ++hi;
    lo_d -= 4294967296.0;
  }
  const unsigned int lo = static_cast<unsigned int>(lo_d);
  return (static_cast<unsigned long long>(hi) << 32) | lo;
}

__attribute__((used, pcs("aapcs"))) long long
__aeabi_d2lz(double x) noexcept
{
  if ( x != x ) return 0;      // NaN
  if ( x >= 9223372036854775808.0 ) return 0x7FFFFFFFFFFFFFFFLL;
  if ( x < -9223372036854775808.0 ) return static_cast<long long>(0x8000000000000000ULL);
  if ( x >= 0.0 ) return static_cast<long long>(__aeabi_d2ulz(x));
  return -static_cast<long long>(__aeabi_d2ulz(-x));
}

__attribute__((used, pcs("aapcs"))) unsigned long long
__aeabi_f2ulz(float x) noexcept
{
  return __aeabi_d2ulz(static_cast<double>(x));      // VCVT.F64.F32 then 64-bit path
}

__attribute__((used, pcs("aapcs"))) long long
__aeabi_f2lz(float x) noexcept
{
  return __aeabi_d2lz(static_cast<double>(x));
}

}      // extern "C"

#endif      // __micron_arch_arm32
