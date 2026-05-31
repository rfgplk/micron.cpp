//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::regex side of the regex comparison benchmark, isolated in its OWN
// translation unit. micron headers inject SIMD intrinsics at global scope and
// define base types, which would clash with <regex>/<boost/regex.hpp> in the
// driver TU -- so the driver never sees a micron header and reaches the engine
// only through these extern "C" entry points. Linked against the driver:
//   g++ regex_bench_micron.cpp regex_bench.cpp -lboost_regex -o regex_bench

#include "../../src/regex.hpp"

extern "C" {

void *
mc_re_compile(const char *pat)
{
  // placement-new into micron-allocated storage: avoids the global operator new
  // (which micron may interpose) so this object file links cleanly with libstdc++.
  void *mem = micron::alloc<micron::regex>(sizeof(micron::regex));
  return static_cast<void *>(new (mem) micron::regex(pat));
}

// Sheng / table DFA boolean "does it match anywhere".
int
mc_has_match(void *h, const char *in, long n)
{
  return static_cast<micron::regex *>(h)->has_match_n(in, (usize)n) ? 1 : 0;
}

// Pike VM + SIMD prefilter; returns the end offset of the leftmost match (-1).
long
mc_search(void *h, const char *in, long n)
{
  micron::rmatch m = static_cast<micron::regex *>(h)->search_n(in, (usize)n);
  return m.has_match() ? (long)m.group_end(0) : -1;
}

int
mc_valid(void *h)
{
  return static_cast<micron::regex *>(h)->valid() ? 1 : 0;
}

// 2 = Sheng SIMD DFA, 1 = table DFA, 0 = Pike VM (has_match path)
int
mc_path(void *h)
{
  micron::regex *r = static_cast<micron::regex *>(h);
  return r->uses_sheng() ? 2 : (r->uses_dfa() ? 1 : 0);
}

int
mc_nstates(void *h)
{
  return static_cast<micron::regex *>(h)->dfa_states();
}

void
mc_re_free(void *h)
{
  micron::regex *r = static_cast<micron::regex *>(h);
  r->~regex();
  micron::free(r);
}

}      // extern "C"
