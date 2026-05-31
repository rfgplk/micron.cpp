//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../alloc.hpp"
#include "../concepts.hpp"
#include "../memory/cmemory/memchr.hpp"
#include "../simd/strings.hpp"
#include "../slice.hpp"
#include "../types.hpp"

#include "ast.hpp"
#include "charreach.hpp"
#include "classscan.hpp"
#include "dfa.hpp"
#include "fixed_string.hpp"
#include "pike.hpp"
#include "prefilter.hpp"
#include "program.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// main micron regex code

// NOTE:Grammar is POSIX ERE only; matching is leftmost, greedy

namespace micron
{
namespace rgx
{

inline constexpr usize kMaxGroups = 32;      // capture groups 1..32 (+ whole-match 0)

// {ptr,len} normalisation: accept a raw C string or any has_cstr object.
struct subject_t {
  const char *p = nullptr;
  usize n = 0;
};

constexpr subject_t
as_subject(const char *s) noexcept
{
  usize n = 0;
  if ( s )
    while ( s[n] ) ++n;
  return { s, n };
}

template<has_cstr S>
constexpr subject_t
as_subject(const S &s) noexcept
{
  return { s.c_str(), s.size() };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rmatch
struct rmatch {
  bool matched = false;
  u32 ng = 0;      // number of groups, including group 0
  const char *base = nullptr;
  i64 caps[2 * (kMaxGroups + 1)] = {};

  constexpr bool
  has_match() const noexcept
  {
    return matched;
  }

  constexpr explicit
  operator bool() const noexcept
  {
    return matched;
  }

  constexpr usize
  groups() const noexcept
  {
    return ng;
  }

  constexpr i64
  group_start(usize i) const noexcept
  {
    return (matched && i < ng) ? caps[2 * i] : -1;
  }

  constexpr i64
  group_end(usize i) const noexcept
  {
    return (matched && i < ng) ? caps[2 * i + 1] : -1;
  }

  // span of capture group i, empty/invalid if unset
  constexpr raw_slice<const char>
  group(usize i) const noexcept
  {
    if ( !matched || i >= ng ) return raw_slice<const char>{};
    i64 a = caps[2 * i], b = caps[2 * i + 1];
    if ( a < 0 || b < 0 || b < a ) return raw_slice<const char>{};
    return raw_slice<const char>{ base + a, (usize)(b - a) };
  }
};

constexpr void
finish_match(rmatch &r, vm &m, const char *base, u32 ngroups) noexcept
{
  bool ok = m.run();
  r.matched = ok;
  r.base = base;
  r.ng = ngroups + 1;
  if ( ok )
    for ( usize s = 0; s < m.nslots && s < 2 * (kMaxGroups + 1); ++s ) r.caps[s] = m.mcaps[s];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// cmatch<"pattern">(input)
template<fixed_string P, class S>
constexpr rmatch
cmatch(const S &input) noexcept
{
  constexpr usize n = P.size();
  constexpr usize maxn = n * 4 + 8;
  constexpr usize maxc = n + 4;
  constexpr usize maxi = n * 8 + 16;
  constexpr usize maxg = (n < kMaxGroups ? n : kMaxGroups);
  constexpr usize maxslots = 2 * (maxg + 1);

  rmatch r;

  node nodes[maxn]{};
  charreach cls[maxc]{};
  inst code[maxi]{};
  usize ncode = 0, ncls = 0;
  u32 ngroups = 0;
  if ( !compile_regex(P.data(), n, nodes, maxn, cls, maxc, code, maxi, ncode, ncls, ngroups) ) return r;
  if ( ngroups > maxg ) return r;

  usize nslots = 2 * (ngroups + 1);

  u32 a_pc[maxi]{};
  u32 b_pc[maxi]{};
  i64 a_sav[maxi * maxslots]{};
  i64 b_sav[maxi * maxslots]{};
  u32 seen[maxi]{};
  i64 work[maxslots]{};
  i64 mcaps[maxslots]{};

  subject_t sub = as_subject(input);

  vm m;
  m.pv = prog_view{ code, ncode, cls, ncls, ngroups };
  m.nslots = nslots;
  m.cap = ncode;
  m.in = sub.p;
  m.n = sub.n;
  m.a_pc = a_pc;
  m.a_sav = a_sav;
  m.b_pc = b_pc;
  m.b_sav = b_sav;
  m.seen = seen;
  m.gen = 0;
  m.work = work;
  m.mcaps = mcaps;

  finish_match(r, m, sub.p, ngroups);
  return r;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// regex: runtime pattern; heap allocated, move only
class regex
{
  inst *__code = nullptr;
  charreach *__cls = nullptr;
  usize __ncode = 0;
  usize __ncls = 0;
  u32 __ngroups = 0;
  bool __ok = false;
  charreach __first{};            // bytes that can begin a match (prefilter)
  bool __nullable = false;        // pattern can match the empty string
  dfa *__dfa = nullptr;           // SIMD/table has_match accelerator (or null)
  bool __prefer_dfa = false;      // pattern has .* / wide-class loop -> DFA beats the prefilter

  void
  build(const char *pat, usize len) noexcept
  {
    usize maxn = len * 4 + 16, maxc = len + 8, maxi = len * 8 + 32;
    node *nodes = micron::alloc<node>(maxn * sizeof(node));
    __cls = micron::alloc<charreach>(maxc * sizeof(charreach));
    __code = micron::alloc<inst>(maxi * sizeof(inst));
    usize ncls = 0;
    u32 ng = 0;
    __ok = compile_regex(pat, len, nodes, maxn, __cls, maxc, __code, maxi, __ncode, ncls, ng);
    __ncls = ncls;
    __ngroups = ng;
    micron::free(nodes);
    if ( ng > kMaxGroups ) __ok = false;
    if ( __ok ) {
      prog_view pv{ __code, __ncode, __cls, __ncls, __ngroups };
      char *seen = micron::alloc<char>(__ncode ? __ncode : 1);
      first_info fi = compute_first(pv, seen);
      __first = fi.first;
      __nullable = fi.nullable;
      __dfa = build_dfa(pv, seen);      // null if unsuitable -> Pike VM fallback
      micron::free(seen);
      // NOTE: Pike is slow when a match can span a long run (`.*`, `[^x]*`)
      for ( usize k = 0; k < __ncode; ++k ) {
        if ( __code[k].code == op::Any || __code[k].code == op::Bol ) {      // .* / ^-anchored -> DFA
          __prefer_dfa = true;
          break;
        }
        if ( __code[k].code == op::Class && __code[k].x < __ncls && __cls[__code[k].x].count() > 200 ) {
          __prefer_dfa = true;
          break;
        }
      }
    }
  }

  rmatch
  do_search(const char *p, usize n, bool anchored) const noexcept
  {
    rmatch r;
    if ( !__ok ) return r;
    usize nslots = 2 * (__ngroups + 1);

    u32 *a_pc = micron::alloc<u32>(__ncode * sizeof(u32));
    u32 *b_pc = micron::alloc<u32>(__ncode * sizeof(u32));
    i64 *a_sav = micron::alloc<i64>(__ncode * nslots * sizeof(i64));
    i64 *b_sav = micron::alloc<i64>(__ncode * nslots * sizeof(i64));
    u32 *seen = micron::alloc<u32>(__ncode * sizeof(u32));
    i64 *work = micron::alloc<i64>(nslots * sizeof(i64));
    i64 *mcaps = micron::alloc<i64>(nslots * sizeof(i64));
    for ( usize i = 0; i < __ncode; ++i ) seen[i] = 0;

    vm m;
    m.pv = prog_view{ __code, __ncode, __cls, __ncls, __ngroups };
    m.nslots = nslots;
    m.cap = __ncode;
    m.in = p;
    m.n = n;
    m.anchored = anchored;
    m.a_pc = a_pc;
    m.a_sav = a_sav;
    m.b_pc = b_pc;
    m.b_sav = b_sav;
    m.seen = seen;
    m.gen = 0;
    m.work = work;
    m.mcaps = mcaps;

    usize fc = __first.count();
    bool prefiltered = false;
    if ( !anchored && !__nullable && fc >= 1 && fc < 256 ) {
      const int kMaxFailedAttempts = 32;
      unsigned char single = (fc == 1) ? (unsigned char)__first.single() : 0;
      truffle_masks tm;
      if ( fc > 1 ) tm = truffle_build(__first);

      m.anchored = true;
      bool hit = false;
      int attempts = 0;
      for ( usize pos = 0; pos < n; ) {
        usize cstart;
        if ( fc == 1 ) {
          const char *fp = micron::memchr(p + pos, single, n - pos);
          if ( !fp ) break;
          cstart = (usize)(fp - p);
        } else {
          usize idx = truffle_find_first(p + pos, n - pos, tm);
          if ( idx >= n - pos ) break;
          cstart = pos + idx;
        }
        m.start_at = cstart;
        if ( m.run() ) {
          hit = true;
          break;
        }
        pos = cstart + 1;
        if ( ++attempts > kMaxFailedAttempts ) break;      // dense candidates -> fall back
      }
      if ( hit ) {
        r.matched = true;
        r.base = p;
        r.ng = __ngroups + 1;
        for ( usize s = 0; s < nslots && s < 2 * (kMaxGroups + 1); ++s ) r.caps[s] = m.mcaps[s];
        prefiltered = true;
      } else if ( attempts <= kMaxFailedAttempts ) {
        // candidates exhausted with no match -> genuinely no match
        r.matched = false;
        r.base = p;
        r.ng = __ngroups + 1;
        prefiltered = true;
      }
    }
    if ( !prefiltered ) {
      m.anchored = anchored;
      m.start_at = 0;
      finish_match(r, m, p, __ngroups);
    }

    micron::free(a_pc);
    micron::free(b_pc);
    micron::free(a_sav);
    micron::free(b_sav);
    micron::free(seen);
    micron::free(work);
    micron::free(mcaps);
    return r;
  }

public:
  regex(const char *pattern) noexcept
  {
    subject_t s = as_subject(pattern);
    build(s.p, s.n);
  }

  template<has_cstr S> regex(const S &pattern) noexcept
  {
    subject_t s = as_subject(pattern);
    build(s.p, s.n);
  }

  regex(const regex &) = delete;
  regex &operator=(const regex &) = delete;

  regex(regex &&o) noexcept
      : __code(o.__code), __cls(o.__cls), __ncode(o.__ncode), __ncls(o.__ncls), __ngroups(o.__ngroups), __ok(o.__ok), __first(o.__first),
        __nullable(o.__nullable), __dfa(o.__dfa), __prefer_dfa(o.__prefer_dfa)
  {
    o.__code = nullptr;
    o.__cls = nullptr;
    o.__dfa = nullptr;
    o.__ok = false;
  }

  regex &
  operator=(regex &&o) noexcept
  {
    if ( this != &o ) {
      if ( __code ) micron::free(__code);
      if ( __cls ) micron::free(__cls);
      if ( __dfa ) dfa_free(__dfa);
      __code = o.__code;
      __cls = o.__cls;
      __ncode = o.__ncode;
      __ncls = o.__ncls;
      __ngroups = o.__ngroups;
      __ok = o.__ok;
      __first = o.__first;
      __nullable = o.__nullable;
      __dfa = o.__dfa;
      __prefer_dfa = o.__prefer_dfa;
      o.__code = nullptr;
      o.__cls = nullptr;
      o.__dfa = nullptr;
      o.__ok = false;
    }
    return *this;
  }

  ~regex() noexcept
  {
    if ( __code ) micron::free(__code);
    if ( __cls ) micron::free(__cls);
    if ( __dfa ) dfa_free(__dfa);
  }

  bool
  valid() const noexcept
  {
    return __ok;
  }

  // true if has_match() uses the Sheng SIMD (PSHUFB) DFA fast path
  bool
  uses_sheng() const noexcept
  {
    return __dfa && __dfa->has_sheng;
  }

  // true if has_match() uses any DFA accelerator (Sheng or transition table)
  bool
  uses_dfa() const noexcept
  {
    return __dfa != nullptr;
  }

  // number of DFA states (0 if no DFA was built)
  int
  dfa_states() const noexcept
  {
    return __dfa ? __dfa->nstates : 0;
  }

  template<class S>
  rmatch
  match(const S &input) const noexcept
  {
    subject_t s = as_subject(input);
    return do_search(s.p, s.n, false);
  }

  template<class S>
  rmatch
  search(const S &input) const noexcept
  {
    subject_t s = as_subject(input);
    return do_search(s.p, s.n, false);
  }

  template<class S>
  bool
  has_match(const S &input) const noexcept
  {
    if ( !__ok ) return false;
    subject_t s = as_subject(input);
    if ( __dfa && __prefer_dfa ) return dfa_has_match(__dfa, s.p, s.n);      // .*/wide -> DFA
    return do_search(s.p, s.n, false).matched;                               // else SIMD prefilter
  }

  rmatch
  search_n(const char *p, usize n) const noexcept
  {
    return do_search(p, n, false);
  }

  rmatch
  match_n(const char *p, usize n) const noexcept
  {
    return do_search(p, n, false);
  }

  bool
  has_match_n(const char *p, usize n) const noexcept
  {
    if ( !__ok ) return false;
    if ( __dfa && __prefer_dfa ) return dfa_has_match(__dfa, p, n);      // .*/wide -> DFA
    return do_search(p, n, false).matched;                               // else SIMD prefilter
  }
};

};      // namespace rgx

// public exports
using rgx::cmatch;
using rgx::regex;
using rgx::rmatch;

};      // namespace micron
