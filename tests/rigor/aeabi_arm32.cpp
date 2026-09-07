//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE ARM EABI DIVISION HELPERS, AGAINST THE HARDWARE'S OWN ANSWER.
//
// math/__gcc_int_syms.hpp carries these because a freestanding -nostdlib link has no libgcc and
// GCC's ARM back end lowers every divide to them. Before they existed, `duck --arm --metal` did not
// link at all: four `undefined reference to __aeabi_uldivmod' out of print.hpp's printk<u64>.
//
// WHAT THIS FILE ASSERTS THAT A LINK CANNOT. Linking proves the symbols are present. It says
// nothing about whether they compute the right number, and nothing at all about the half of the
// contract that lives in the REGISTER ALLOCATION: __aeabi_uidivmod hands back a pair in {r0, r1}
// and __aeabi_uldivmod hands back two 64-bit values in {r0:r1} and {r2:r3}. No C++ return type
// names the second shape -- a 16-byte struct goes home through memory under AAPCS -- so the two
// ldivmod entries are file-scope asm, and a stub that computed perfectly while returning the
// remainder in the wrong register pair would link, run, and silently corrupt every `%` in the tree.
// __mc_probe_uldivmod/__mc_probe_ldivmod below reach into r0-r3 explicitly for exactly that reason.
//
// The oracle is the compiler's own `/` and `%` on the same operands. That is legitimate here and
// only here: this test is built HOSTED, where libgcc exists and the hardware or its runtime answers
// natively, so the two sides are genuinely independent implementations. (Under --metal there is no
// second implementation to compare against, which is why this test is hosted-only.)
//
//     duck test tests/rigor/aeabi_arm32.cpp --arm -o bin/t
//     duck emulate tests/rigor/aeabi_arm32.cpp --arm -o bin/t
//
// It is a no-op returning the PASS sentinel on every other target, so it can sit in a sweep.
//
// NEGATIVE CONTROL, and it is not optional -- a differential test whose two sides are the same code
// passes forever. There is no -D switch for this on purpose; a mutation knob living in production
// code is a branch no cell compiles. Edit math/__gcc_int_syms.hpp by hand, run, revert. Measured,
// -O2 -marm under qemu-arm-static, against the version of this file that shipped:
//
//   __aeabi_uidiv            + 1 on the returned quotient                    -> 134
//   __aeabi_uidivmod         q and r swapped between r0 and r1               -> 135
//   __aeabi_uldivmod         "ldr r2,[sp,#8]" -> "ldr r0,[sp,#8]"            ->   6
//   __divmoddi4              __rneg = (d < 0) instead of (n < 0)             ->   6
//   unmutated                                                                ->   1
//
// The first two abort rather than reporting a require() failure, and that is not the test being
// sloppy -- these symbols are `weak`, so in a hosted static link they displace libgcc's for the
// WHOLE binary, snowball's own output path included. A helper that divides wrongly takes the
// harness down with it. Any exit that is not 1 is a failure; 6 is merely the tidiest one.
//
// Seeds are fixed hex literals. Never time-based.

#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

#if defined(__micron_arch_arm32) && defined(__ARM_EABI__)

#include "../../src/math/__gcc_int_syms.hpp"

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the register-pair probes
//
// The operands go in by hand and all four result registers come back out, because the placement IS
// the contract. A plain call would let the compiler read whatever it believed the ABI to be.

struct upair {
  u64 q;
  u64 r;
};

struct ipair {
  i64 q;
  i64 r;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// THE PROBES ARE FILE-SCOPE ASM, AND THE FIRST VERSION OF THEM WAS NOT
//
// The obvious spelling -- local register variables pinned to r0-r3 with "+r" and a `bl` inside an
// asm volatile -- WORKS AT -O1 AND SEGFAULTS AT -O2. A `bl` clobbers r12/ip and the whole
// caller-saved set as a side effect of being a call, and an asm statement can only declare what it
// is told to declare; nothing in the constraint list says "this is a function call". GCC kept a live
// value in r12 across the asm and the callee ate it.
//
// So the probe is a real function with a real AAPCS frame, exactly like the stubs it is testing.
// r0:r1 and r2:r3 come back from __aeabi_{u,}ldivmod and are stored through the two pointers the
// caller passed on the stack. r4/r5 are callee-saved, so they survive the bl and can hold those
// pointers across it.
//
// Frame: sp is 8-aligned on entry; push of three registers leaves it 4 mod 8, and the sub of 4
// restores it before the call. The two stacked arguments were at [sp_entry+0] and [sp_entry+4], so
// after 16 bytes of frame they are at [sp,#16] and [sp,#20].

extern "C" void __mc_probe_uldivmod(u64 __n, u64 __d, u64 *__q, u64 *__r) noexcept;
extern "C" void __mc_probe_ldivmod(i64 __n, i64 __d, i64 *__q, i64 *__r) noexcept;

#if defined(__thumb__)
#define __mc_probe_thumb ".thumb_func\n\t"
#else
#define __mc_probe_thumb ""
#endif

#define __mc_probe(__nm, __callee)                                                                                                         \
  __asm__(".text\n\t"                                                                                                                      \
          ".syntax unified\n\t"                                                                                                            \
          ".align 2\n\t"                                                                                                                   \
          ".global " __nm "\n\t"                                                                                                           \
          ".type " __nm ", %function\n\t" __mc_probe_thumb __nm ":\n\t"                                                                    \
          "push {r4, r5, lr}\n\t"                                                                                                          \
          "sub  sp, sp, #4\n\t"                                                                                                            \
          "ldr  r4, [sp, #16]\n\t"                                                                                                         \
          "ldr  r5, [sp, #20]\n\t"                                                                                                         \
          "bl   " __callee "\n\t"                                                                                                          \
          "str  r0, [r4]\n\t"                                                                                                              \
          "str  r1, [r4, #4]\n\t"                                                                                                          \
          "str  r2, [r5]\n\t"                                                                                                              \
          "str  r3, [r5, #4]\n\t"                                                                                                          \
          "add  sp, sp, #4\n\t"                                                                                                            \
          "pop  {r4, r5, pc}\n\t"                                                                                                          \
          ".size " __nm ", . - " __nm "\n\t"                                                                                               \
          ".previous\n\t")

__mc_probe("__mc_probe_uldivmod", "__aeabi_uldivmod");
__mc_probe("__mc_probe_ldivmod", "__aeabi_ldivmod");

#undef __mc_probe
#undef __mc_probe_thumb

upair
call_uldivmod(u64 __n, u64 __d) noexcept
{
  upair __o{ 0, 0 };
  __mc_probe_uldivmod(__n, __d, &__o.q, &__o.r);
  return __o;
}

ipair
call_ldivmod(i64 __n, i64 __d) noexcept
{
  ipair __o{ 0, 0 };
  __mc_probe_ldivmod(__n, __d, &__o.q, &__o.r);
  return __o;
}

// the oracle side must not be constant-folded into the same expression tree as the probe
[[gnu::noinline]] u64
opaque(u64 __v) noexcept
{
  asm volatile("" : "+r"(__v));
  return __v;
}

u64
lcg(u64 &__s) noexcept
{
  __s = __s * 6364136223846793005ull + 1442695040888963407ull;
  return __s ^ (__s >> 29);
}

};      // namespace

int
main(void)
{
  sb::print("=== ARM EABI DIVISION HELPERS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("the boundary cases, by hand");
  {
    bool ok = true;

    // divide by zero is DEFINED here as 0/0 -- freestanding has no SIGFPE to raise and a trap in a
    // divide would be a worse failure mode on a board than a wrong number
    if ( __aeabi_uidiv(1u, 0u) != 0u ) ok = false;
    if ( call_uldivmod(1ull, 0ull).q != 0ull ) ok = false;

    if ( __aeabi_uidiv(0u, 7u) != 0u ) ok = false;
    if ( __aeabi_uidiv(7u, 7u) != 1u ) ok = false;
    if ( __aeabi_uidiv(0xFFFFFFFFu, 1u) != 0xFFFFFFFFu ) ok = false;
    if ( __aeabi_uidiv(0xFFFFFFFFu, 0xFFFFFFFFu) != 1u ) ok = false;

    // the asymmetric extremes: |INT_MIN| is not representable, so a signed helper that negates
    // through the SIGNED type is UB and this is where it shows
    if ( __aeabi_idiv(-2147483647 - 1, 1) != (-2147483647 - 1) ) ok = false;
    if ( __aeabi_idiv(-2147483647 - 1, 2) != -1073741824 ) ok = false;
    if ( __aeabi_idiv(-7, 2) != -3 ) ok = false;      // C++ truncates toward zero
    if ( __aeabi_idiv(7, -2) != -3 ) ok = false;

    const i64 i64_min = -9223372036854775807LL - 1;
    if ( call_ldivmod(i64_min, 1).q != i64_min ) ok = false;
    if ( call_ldivmod(i64_min, 2).q != -4611686018427387904LL ) ok = false;

    // the remainder takes the sign of the DIVIDEND in C++, not of the divisor
    {
      const ipair a = call_ldivmod(-7, 2);
      const ipair b = call_ldivmod(7, -2);
      if ( a.q != -3 || a.r != -1 ) ok = false;
      if ( b.q != -3 || b.r != 1 ) ok = false;
    }

    // the u32 pair, unpacked from {r0, r1}
    {
      const u64 m = __aeabi_uidivmod(17u, 5u);
      if ( static_cast<u32>(m) != 3u || static_cast<u32>(m >> 32) != 2u ) ok = false;
    }
    {
      const i64 m = __aeabi_idivmod(-17, 5);
      if ( static_cast<i32>(m) != -3 || static_cast<i32>(static_cast<u64>(m) >> 32) != -2 ) ok = false;
    }

    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("every power-of-two divisor, and every operand width");
  {
    bool ok = true;
    // the shift-subtract loop walks bit 31 (or 63) down to 0, so a bound that is off by one is
    // visible only at the ends -- sweep both operand widths across their whole range
    for ( int nb = 0; nb < 64 && ok; ++nb ) {
      const u64 n = (nb == 63) ? 0x8000000000000000ull : ((1ull << nb) | 0x5A5Aull);
      for ( int db = 0; db < 64 && ok; ++db ) {
        const u64 d = 1ull << db;
        const upair g = call_uldivmod(opaque(n), opaque(d));
        if ( g.q != opaque(n) / opaque(d) || g.r != opaque(n) % opaque(d) ) ok = false;
      }
      const u32 n32 = static_cast<u32>(n);
      for ( int db = 0; db < 32 && ok; ++db ) {
        const u32 d32 = 1u << db;
        if ( __aeabi_uidiv(n32, d32) != static_cast<u32>(opaque(n32) / opaque(d32)) ) ok = false;
        const u64 m = __aeabi_uidivmod(n32, d32);
        if ( static_cast<u32>(m) != static_cast<u32>(opaque(n32) / opaque(d32)) ) ok = false;
        if ( static_cast<u32>(m >> 32) != static_cast<u32>(opaque(n32) % opaque(d32)) ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("200000 seeded rounds against the native answer, all six entries");
  {
    u64 s = 0xC0FFEE1234567891ull;
    long bad = 0;
    for ( long i = 0; i < 200000 && bad == 0; ++i ) {
      u64 a = lcg(s) >> (lcg(s) & 63);
      u64 b = lcg(s) >> (lcg(s) & 63);
      if ( b == 0 ) b = 1;

      const upair g = call_uldivmod(a, b);
      if ( g.q != opaque(a) / opaque(b) || g.r != opaque(a) % opaque(b) ) ++bad;

      const i64 sa = static_cast<i64>(a);
      i64 sb_ = static_cast<i64>(b);
      if ( sb_ == 0 ) sb_ = 1;
      // INT64_MIN / -1 overflows and is UB on the ORACLE side too -- excluded, not papered over
      if ( !(sa == (-9223372036854775807LL - 1) && sb_ == -1) ) {
        const ipair t = call_ldivmod(sa, sb_);
        if ( t.q != sa / sb_ || t.r != sa % sb_ ) ++bad;
      }

      const u32 ua = static_cast<u32>(a);
      u32 ub = static_cast<u32>(b);
      if ( ub == 0 ) ub = 1;
      if ( __aeabi_uidiv(ua, ub) != ua / ub ) ++bad;
      const u64 um = __aeabi_uidivmod(ua, ub);
      if ( static_cast<u32>(um) != ua / ub || static_cast<u32>(um >> 32) != ua % ub ) ++bad;

      const i32 ia = static_cast<i32>(a);
      i32 ib = static_cast<i32>(b);
      if ( ib == 0 ) ib = 1;
      if ( !(ia == (-2147483647 - 1) && ib == -1) ) {
        if ( __aeabi_idiv(ia, ib) != ia / ib ) ++bad;
        const i64 im = __aeabi_idivmod(ia, ib);
        if ( static_cast<i32>(im) != ia / ib ) ++bad;
        if ( static_cast<i32>(static_cast<u64>(im) >> 32) != ia % ib ) ++bad;
      }
    }
    sb::require(bad == 0);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("__divmoddi4 returns both halves in one call");
  {
    // it is a real libgcc entry GCC may call directly, and it is the only path that computes the
    // quotient and the remainder together -- __divdi3/__moddi3 each recompute the other
    bool ok = true;
    u64 s = 0x243F6A8885A308D3ull;
    for ( int i = 0; i < 20000 && ok; ++i ) {
      const i64 n = static_cast<i64>(lcg(s) >> (lcg(s) & 63));
      i64 d = static_cast<i64>(lcg(s) >> (lcg(s) & 63));
      if ( d == 0 ) d = 1;
      if ( n == (-9223372036854775807LL - 1) && d == -1 ) continue;
      i64 r = 0;
      const i64 q = __divmoddi4(n, d, &r);
      if ( q != n / d || r != n % d ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

#else

int
main(void)
{
  sb::print("=== ARM EABI DIVISION HELPERS ===");
  sb::skip("not an ARM EABI target -- these symbols do not exist here");
  return 1;
}

#endif
