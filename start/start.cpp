//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <micron/bits/__arch.hpp>
#include <micron/config.hpp>

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
extern "C" void __boot_io_buffers(void);
#pragma GCC diagnostic pop

// microns entry/exit points
extern "C" __attribute__((weak)) void
__boot_io_sigpipe(void)
{
}

extern "C" __attribute__((weak)) void
__boot_threadpool(void)
{
}

extern "C" __attribute__((weak)) void
__shutdown_io_buffers(void)
{
}

// mem* tunables probe
// resolves to null when the binary never pulls in cmemory
extern "C" __attribute__((weak)) void __micron_mem_init(void) noexcept;

extern "C" {
char **environ = nullptr;
}

#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse)
// Flush denormals to zero (FTZ + DAZ)
// (MXCSR builtins require SSE)
[[gnu::always_inline]] inline void
enable_fast_fp() noexcept
{
  unsigned int mxcsr = __builtin_ia32_stmxcsr();
  mxcsr |= 0x8040u;      // 0x8000 = FTZ, 0x0040 = DAZ
  __builtin_ia32_ldmxcsr(mxcsr);
}
#endif
#if defined(__micron_arch_arm64)
// On AArch64 FPCR.FZ flushes subnormal inputs *and* results (FTZ + DAZ in one)
[[gnu::always_inline]] inline void
enable_fast_fp() noexcept
{
  unsigned long long fpcr;
  __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
  fpcr |= (1ull << 24);      // FZ
  // fpcr |= (1ull << 19);     // FZ16: half-precision FTZ, only with FEAT_FP16
  __asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));
}
#endif
#if defined(__micron_arch_arm32)
// FPSCR.FZ controls VFP scalar ops
// apparently SIMD/NEON already flushes subnormals unconditionally, regardless of this bit
[[gnu::always_inline]] inline void
enable_fast_fp() noexcept
{
  unsigned int fpscr;
  __asm__ __volatile__("vmrs %0, fpscr" : "=r"(fpscr));
  fpscr |= (1u << 24);      // FZ
  // fpscr |= (1u << 19);      // FZ16: needs the FP16 extension
  __asm__ __volatile__("vmsr fpscr, %0" : : "r"(fpscr));
}

#endif
#if !((defined(__micron_arch_x86_any) && defined(__micron_x86_sse)) || defined(__micron_arch_arm64) || defined(__micron_arch_arm32))
// no-op fallback so we remain wellformed on arches without fast fp
[[gnu::always_inline]] inline void
enable_fast_fp() noexcept
{
}
#endif
extern "C" __attribute__((used, visibility("default"))) int
__micron_startc(int argc, char **argv, char **envp, const micron::auxv_t *auxv) noexcept
{
  environ = envp;

  // TLS first: thread_local storage depends on it
  micron::__tls_init(auxv);

  // primary stack region
  micron::__stack_init(auxv);

  // probe mem tunables
  if ( __micron_mem_init ) __micron_mem_init();

  // NOTE: we must register shutdown io first so it lands at the bottom of the atexit stack,
  // ensuring it runs last
  micron::atexit([] { __shutdown_io_buffers(); });

  // threading and sig pipes
  __boot_threadpool();      // __global_threadpool ready before any user micron::go()
  __boot_io_sigpipe();      // SIG_IGN installed before any user write()

  // user / third-party global constructors
  for ( void (**p)(void) = __preinit_array_start; p < __preinit_array_end; ++p ) (*p)();
  for ( void (**p)(void) = __init_array_start; p < __init_array_end; ++p ) (*p)();

  // io buffer init MUST fire AFTER .init_array
  __boot_io_buffers();

  if constexpr ( micron::config::fast_math_x86 ) enable_fast_fp();

  const int rc = __micron_user_main(argc, argv, envp);

  micron::group_exit(rc);
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

// IEEE-754 binary128 soft float runtime helpers
namespace
{
using __tf_rep = unsigned __int128;

inline constexpr int __tf_mant = 112;      // stored mantissa bits
inline constexpr int __tf_bias = 16383;
inline constexpr unsigned __tf_einf = 0x7FFFu;                                        // all-ones exponent field
inline constexpr __tf_rep __tf_implicit = static_cast<__tf_rep>(1) << __tf_mant;      // 2^112
inline constexpr __tf_rep __tf_mantmask = __tf_implicit - 1;                          // low 112 bits
inline constexpr __tf_rep __tf_signbit = static_cast<__tf_rep>(1) << 127;
inline constexpr __tf_rep __tf_absmask = ~__tf_signbit;
inline constexpr __tf_rep __tf_infrep = static_cast<__tf_rep>(__tf_einf) << __tf_mant;
inline constexpr __tf_rep __tf_qnanbit = static_cast<__tf_rep>(1) << (__tf_mant - 1);
inline constexpr __tf_rep __tf_qnan = __tf_infrep | __tf_qnanbit;

[[gnu::always_inline]] inline __tf_rep
__tf_bits(_Float128 x) noexcept
{
  return __builtin_bit_cast(__tf_rep, x);
}

[[gnu::always_inline]] inline _Float128
__tf_from(__tf_rep b) noexcept
{
  return __builtin_bit_cast(_Float128, b);
}

[[gnu::always_inline]] inline int
__tf_clz(__tf_rep x) noexcept      // precondition: x != 0
{
  const unsigned long long hi = static_cast<unsigned long long>(x >> 64);
  if ( hi ) return __builtin_clzll(hi);
  return 64 + __builtin_clzll(static_cast<unsigned long long>(x));
}

// shift a (sub)normal significand so its leading 1 lands at bit __tf_mant
[[gnu::always_inline]] inline int
__tf_normalize(__tf_rep &sig) noexcept
{
  const int shift = __tf_clz(sig) - (127 - __tf_mant);
  sig <<= shift;
  return 1 - shift;
}

inline _Float128
__tf_addsub(__tf_rep a, __tf_rep b) noexcept
{
  __tf_rep aAbs = a & __tf_absmask;
  __tf_rep bAbs = b & __tf_absmask;

  if ( aAbs >= __tf_infrep || bAbs >= __tf_infrep || aAbs == 0 || bAbs == 0 ) {
    if ( aAbs > __tf_infrep ) return __tf_from(a | __tf_qnanbit);      // a is NaN -> quiet
    if ( bAbs > __tf_infrep ) return __tf_from(b | __tf_qnanbit);
    if ( aAbs == __tf_infrep ) {
      if ( bAbs == __tf_infrep && ((a ^ b) >> 127) ) return __tf_from(__tf_qnan);      // inf + -inf
      return __tf_from(a);
    }
    if ( bAbs == __tf_infrep ) return __tf_from(b);
    if ( aAbs == 0 ) return __tf_from(bAbs == 0 ? (a & b & __tf_signbit) : b);      // -0 + -0 = -0
    return __tf_from(a);                                                            // b == 0
  }

  if ( bAbs > aAbs ) {      // keep |a| >= |b|
    __tf_rep t = a;
    a = b;
    b = t;
    t = aAbs;
    aAbs = bAbs;
    bAbs = t;
  }

  int aExp = static_cast<int>(aAbs >> __tf_mant);
  int bExp = static_cast<int>(bAbs >> __tf_mant);
  __tf_rep aSig = aAbs & __tf_mantmask;
  __tf_rep bSig = bAbs & __tf_mantmask;
  if ( aExp == 0 )
    aExp = __tf_normalize(aSig);
  else
    aSig |= __tf_implicit;
  if ( bExp == 0 )
    bExp = __tf_normalize(bSig);
  else
    bSig |= __tf_implicit;

  const bool subtraction = ((a ^ b) >> 127) != 0;
  const __tf_rep resultSign = a & __tf_signbit;

  aSig <<= 3;      // make room for guard/round/sticky in the low 3 bits
  bSig <<= 3;
  const int align = aExp - bExp;
  if ( align ) {
    if ( align < 128 ) {
      const __tf_rep sticky = (bSig << (128 - align)) != 0 ? static_cast<__tf_rep>(1) : 0;
      bSig = (bSig >> align) | sticky;
    } else {
      bSig = 1;      // wholly below the sticky bit
    }
  }

  if ( subtraction ) {
    aSig -= bSig;
    if ( aSig == 0 ) return __tf_from(0);      // exact cancellation -> +0
    if ( aSig < (__tf_implicit << 3) ) {       // renormalize after cancellation
      const int shift = __tf_clz(aSig) - (127 - (__tf_mant + 3));
      aSig <<= shift;
      aExp -= shift;
    }
  } else {
    aSig += bSig;
    if ( aSig & (__tf_implicit << 4) ) {      // carry out of the top
      const __tf_rep sticky = aSig & 1;
      aSig = (aSig >> 1) | sticky;
      ++aExp;
    }
  }

  if ( aExp >= static_cast<int>(__tf_einf) ) return __tf_from(resultSign | __tf_infrep);
  if ( aExp <= 0 ) return __tf_from(resultSign);      // subnormal/underflow -> signed zero (FTZ)

  const unsigned grs = static_cast<unsigned>(aSig & 7u);
  const __tf_rep frac = (aSig >> 3) & __tf_mantmask;
  __tf_rep result = resultSign | (static_cast<__tf_rep>(aExp) << __tf_mant) | frac;
  if ( grs > 4u || (grs == 4u && ((aSig >> 3) & 1u)) ) result += 1;      // RNE; carry flows into exp/inf
  return __tf_from(result);
}

[[gnu::always_inline]] inline void
__tf_wide_mul(__tf_rep a, __tf_rep b, __tf_rep &hi, __tf_rep &lo) noexcept
{
  const unsigned long long al = static_cast<unsigned long long>(a), ah = static_cast<unsigned long long>(a >> 64);
  const unsigned long long bl = static_cast<unsigned long long>(b), bh = static_cast<unsigned long long>(b >> 64);
  const __tf_rep ll = static_cast<__tf_rep>(al) * bl;
  const __tf_rep lh = static_cast<__tf_rep>(al) * bh;
  const __tf_rep hl = static_cast<__tf_rep>(ah) * bl;
  const __tf_rep hh = static_cast<__tf_rep>(ah) * bh;
  const __tf_rep mid = (ll >> 64) + static_cast<unsigned long long>(lh) + static_cast<unsigned long long>(hl);
  lo = static_cast<unsigned long long>(ll) | (mid << 64);
  hi = hh + (lh >> 64) + (hl >> 64) + (mid >> 64);
}
}      // namespace

extern "C" __attribute__((used)) _Float128
__addtf3(_Float128 fa, _Float128 fb) noexcept
{
  return __tf_addsub(__tf_bits(fa), __tf_bits(fb));
}

extern "C" __attribute__((used)) _Float128
__subtf3(_Float128 fa, _Float128 fb) noexcept
{
  return __tf_addsub(__tf_bits(fa), __tf_bits(fb) ^ __tf_signbit);
}

extern "C" __attribute__((used)) _Float128
__multf3(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  const __tf_rep aAbs = a & __tf_absmask, bAbs = b & __tf_absmask;
  const __tf_rep sign = (a ^ b) & __tf_signbit;

  if ( aAbs > __tf_infrep ) return __tf_from(a | __tf_qnanbit);      // NaN
  if ( bAbs > __tf_infrep ) return __tf_from(b | __tf_qnanbit);
  if ( aAbs == __tf_infrep || bAbs == __tf_infrep ) {
    if ( aAbs == 0 || bAbs == 0 ) return __tf_from(__tf_qnan);      // inf * 0
    return __tf_from(sign | __tf_infrep);
  }
  if ( aAbs == 0 || bAbs == 0 ) return __tf_from(sign);      // signed zero

  int aExp = static_cast<int>(aAbs >> __tf_mant);
  int bExp = static_cast<int>(bAbs >> __tf_mant);
  __tf_rep aSig = aAbs & __tf_mantmask;
  __tf_rep bSig = bAbs & __tf_mantmask;
  if ( aExp == 0 )
    aExp = __tf_normalize(aSig);
  else
    aSig |= __tf_implicit;
  if ( bExp == 0 )
    bExp = __tf_normalize(bSig);
  else
    bSig |= __tf_implicit;

  __tf_rep hi, lo;
  __tf_wide_mul(aSig, bSig, hi, lo);      // 226-bit product in (hi:lo)

  int exp = aExp + bExp - __tf_bias;
  int shift;                                          // bits below the kept 113-bit significand
  if ( hi & (static_cast<__tf_rep>(1) << 97) ) {      // leading bit at 225
    shift = 113;
    exp += 1;
  } else {      // leading bit at 224
    shift = 112;
  }

  const __tf_rep sig = (lo >> shift) | (hi << (128 - shift));      // P >> shift, a 113-bit significand
  const unsigned guard = static_cast<unsigned>((lo >> (shift - 1)) & 1);
  const bool sticky = (lo & ((static_cast<__tf_rep>(1) << (shift - 1)) - 1)) != 0;

  if ( exp >= static_cast<int>(__tf_einf) ) return __tf_from(sign | __tf_infrep);
  if ( exp <= 0 ) return __tf_from(sign);      // subnormal/underflow -> signed zero (FTZ)

  __tf_rep result = sign | (static_cast<__tf_rep>(exp) << __tf_mant) | (sig & __tf_mantmask);
  if ( guard && (sticky || (sig & 1)) ) result += 1;      // RNE; carry flows into exp/inf
  return __tf_from(result);
}

extern "C" __attribute__((used)) _Float128
__floatditf(long long i) noexcept      // signed 64-bit int -> binary128 (exact)
{
  if ( i == 0 ) return __tf_from(0);
  const bool neg = i < 0;
  const unsigned long long mag = neg ? (0ull - static_cast<unsigned long long>(i)) : static_cast<unsigned long long>(i);
  const int msb = 63 - __builtin_clzll(mag);
  __tf_rep sig = static_cast<__tf_rep>(mag) << (__tf_mant - msb);      // leading 1 -> bit 112; exact (64 <= 113)
  const __tf_rep exp = static_cast<__tf_rep>(msb + __tf_bias);
  return __tf_from((neg ? __tf_signbit : 0) | (exp << __tf_mant) | (sig & __tf_mantmask));
}

extern "C" __attribute__((used)) long long
__fixtfdi(_Float128 fa) noexcept      // binary128 -> signed 64-bit int (truncate toward zero)
{
  const __tf_rep a = __tf_bits(fa);
  const bool neg = (a >> 127) != 0;
  const unsigned exp = static_cast<unsigned>((a >> __tf_mant) & __tf_einf);
  const __tf_rep mant = a & __tf_mantmask;
  constexpr long long imin = -9223372036854775807LL - 1;
  constexpr long long imax = 9223372036854775807LL;
  if ( exp < static_cast<unsigned>(__tf_bias) ) return 0;      // |x| < 1 (incl. zero/subnormal)
  if ( exp == __tf_einf ) return neg ? imin : imax;            // inf/NaN saturate
  const int unbiased = static_cast<int>(exp) - __tf_bias;      // >= 0
  if ( unbiased >= 63 ) {
    if ( neg && unbiased == 63 && mant == 0 ) return imin;      // exactly -2^63
    return neg ? imin : imax;                                   // out of range -> saturate
  }
  const __tf_rep sig = __tf_implicit | mant;      // 113-bit
  const unsigned long long m = static_cast<unsigned long long>(sig >> (__tf_mant - unbiased));
  return neg ? -static_cast<long long>(m) : static_cast<long long>(m);
}

extern "C" __attribute__((used)) int
__lttf2(_Float128 fa, _Float128 fb) noexcept      // <0 if a<b, 0 if a==b, >0 otherwise; unordered -> >0
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  const __tf_rep aAbs = a & __tf_absmask, bAbs = b & __tf_absmask;
  if ( aAbs > __tf_infrep || bAbs > __tf_infrep ) return 1;      // NaN -> unordered
  if ( (aAbs | bAbs) == 0 ) return 0;                            // +0 == -0
  const bool aNeg = (a >> 127) != 0, bNeg = (b >> 127) != 0;
  if ( aNeg != bNeg ) return aNeg ? -1 : 1;
  if ( aAbs == bAbs ) return 0;
  return ((aAbs < bAbs) != aNeg) ? -1 : 1;
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
