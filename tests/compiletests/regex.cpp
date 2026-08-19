//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate for src/regex/. Not run.
//
// This file exists because `grep -i regex verify_compile.duck` used to return nothing: the regex engine
// had never been built at --isa base. When it finally was, it did not compile -- rgx::dfa_sheng_has_match
// spends a v128::shuffle per input byte, v128::shuffle emitted _mm_shuffle_epi8 with no SSSE3 gate, and
// micron's _mm_shuffle_epi8 is always_inline + target("ssse3"). Below SSSE3 that is not a slow path and
// not a SIGILL, it is `error: inlining failed in call to always_inline ... target specific option
// mismatch` -- 3 of them, at codegen time only.
//
// So this gate has two requirements that are easy to get wrong:
//   1. it must ODR-USE the entry points. every rgx::dfa_* is `inline`; a bare #include emits nothing and
//      passes at every tier.
//   2. it must be built with real CODEGEN. `-fsyntax-only` passes even with the defect present, because
//      always_inline failures are not diagnosed by the front end. verify_compile.duck's --raw-obj does
//      the right thing; do not "optimise" this into a syntax-only check.

#include "../../src/regex.hpp"

#include "../../src/regex/classscan.hpp"
#include "../../src/regex/dfa.hpp"

namespace mc = micron;
namespace rgx = micron::rgx;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the sheng path is a compile-time tier decision, never a runtime one (hard rule 3: no runtime CPU
// detection). assert the constant exists and is a constant.

static_assert(rgx::kShengAvailable == true || rgx::kShengAvailable == false);

#if defined(__micron_arch_x86_any) && defined(__micron_x86_ssse3)
static_assert(rgx::kShengAvailable, "x86 with SSSE3 must take the PSHUFB sheng path");
#endif
#if defined(__micron_arch_x86_any) && !defined(__micron_x86_ssse3)
static_assert(!rgx::kShengAvailable, "below SSSE3 the sheng path must not be selected");
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// odr-use every dfa entry point, including the sheng one directly. taking the address alone is enough
// to force emission; calling them keeps the optimiser from deciding otherwise at higher -O.

using dfa_match_fn = bool (*)(const rgx::dfa *, const char *, usize);

static volatile dfa_match_fn __force_emit[] = {
  &rgx::dfa_has_match,
  &rgx::dfa_sheng_has_match,
  &rgx::dfa_table_has_match,
  &rgx::dfa_accel_has_match,
};

[[gnu::noinline]] int
force_dfa(const rgx::dfa *d, const char *s, usize n)
{
  int r = 0;
  r += rgx::dfa_has_match(d, s, n) ? 1 : 0;
  r += rgx::dfa_sheng_has_match(d, s, n) ? 2 : 0;      // <-- the PSHUFB loop
  r += rgx::dfa_table_has_match(d, s, n) ? 4 : 0;
  r += rgx::dfa_accel_has_match(d, s, n) ? 8 : 0;
  return r;
}

// the truffle set-scan in classscan.hpp: AVX2 leg, NEON leg, scalar tail
[[gnu::noinline]] usize
force_classscan(const rgx::truffle_masks &t, const char *p, usize n)
{
  return rgx::truffle_find_first(p, n, t);
}

[[gnu::noinline]] rgx::truffle_masks
force_truffle_build(const rgx::charreach &cls)
{
  return rgx::truffle_build(cls);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// v128::shuffle itself, on every 128-bit integer lane width, so the SSSE3 gate and its SSE2 leg are
// both codegen'd rather than merely parsed.

[[gnu::noinline]] void
force_shuffle(unsigned char *out, const unsigned char *a, const unsigned char *b)
{
  mc::simd::v8 x, c;
  x.uload(const_cast<unsigned char *>(a));
  c.uload(const_cast<unsigned char *>(b));
  x.shuffle(c).get(reinterpret_cast<signed char *>(out));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the porcelain, which is what actually reaches the DFA in practice

[[gnu::noinline]] int
force_porcelain(const char *pat, const char *in)
{
  mc::regex re(pat);
  int r = 0;
  r += re.has_match(in) ? 1 : 0;
  r += re.has_match_path();
  r += re.uses_sheng() ? 16 : 0;
  r += re.uses_dfa() ? 32 : 0;
  return r;
}

int
main()
{
  return (__force_emit[0] != nullptr) ? 1 : 0;
}
