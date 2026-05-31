//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Driver TU of the regex comparison benchmark: micron::regex (via the extern
// "C" wrapper in regex_bench_micron.cpp) vs std::regex vs boost::regex, all in
// POSIX-ERE mode over identical inputs. This TU contains NO micron headers, so
// <regex>/<boost/regex.hpp> cannot collide with micron's globally-injected SIMD
// intrinsics. Build (single g++ invocation, two isolated TUs):
//   g++ -O3 -std=c++23 -march=native -I./src \
//       benches/regex/regex_bench_micron.cpp benches/regex/regex_bench.cpp \
//       -lboost_regex -o bin/regex_bench
//
// Each cell adapts its repetition count to a ~0.15 s wall target, so the fast
// (micron) and slow (backtracking) engines are both measured accurately.

#include <boost/regex.hpp>

#include <chrono>
#include <cstdio>
#include <regex>
#include <string>

extern "C" {
void *mc_re_compile(const char *pat);
int mc_has_match(void *h, const char *in, long n);
long mc_search(void *h, const char *in, long n);
int mc_valid(void *h);
int mc_path(void *h);
int mc_nstates(void *h);
void mc_re_free(void *h);
}

using clk = std::chrono::steady_clock;

template<class F>
static double
bench_ns(F &&f, bool &ok, bool pathological = false)
{
  ok = true;
  volatile long sink = 0;
  try {
    for ( int i = 0; i < 2 && !pathological; ++i ) sink ^= f();      // warmup
    long reps = 1;
    for ( ;; ) {
      auto t0 = clk::now();
      for ( long i = 0; i < reps; ++i ) sink ^= f();
      auto t1 = clk::now();
      double sec = std::chrono::duration<double>(t1 - t0).count();
      if ( sec > 0.15 || reps > (1L << 30) ) return sec * 1e9 / (double)reps;
      reps *= 2;
    }
  } catch ( ... ) {
    ok = false;      // backtracking engine bailed (complexity limit)
    return -1.0;
  }
}

struct Case {
  const char *name;
  const char *pat;
  std::string input;
  bool pathological;
};

// build inputs: fill `n` of `pad` then append `tail`
static std::string
mk(std::size_t n, char pad, const char *tail)
{
  std::string s(n, pad);
  s += tail;
  return s;
}

int
main()
{
  const std::size_t BIG = 1u << 20;      // 1 MiB throughput inputs

  std::vector<Case> cases;
  cases.push_back({ "literal_miss", "needle", mk(BIG, 'a', ""), false });
  cases.push_back({ "literal_hit", "needle", mk(BIG - 6, 'a', "needle"), false });
  cases.push_back({ "class_miss[0-9]+", "[0-9]+", mk(BIG, 'a', ""), false });
  cases.push_back({ "class_hit[0-9]+", "[0-9]+", mk(BIG - 5, 'a', "12345"), false });
  cases.push_back({ "alternation", "(foo|bar|baz)", mk(BIG - 3, 'x', "baz"), false });
  cases.push_back({ "email", "[a-z]+@[a-z]+", mk(BIG - 9, 'A', "user@host"), false });
  cases.push_back({ "dotstar a.*z", "a.*z", std::string("a") + std::string(BIG, 'x') + "z", false });
  cases.push_back({ "anchored ^[a-z]+$", "^[a-z]+$", std::string(BIG, 'm'), false });
  cases.push_back({ "PATHOLOGICAL (a+)+$", "(a+)+$", mk(26, 'a', "!"), true });

  std::printf("\n  micron::regex  vs  std::regex  vs  boost::regex   (POSIX ERE, regex_search semantics)\n");
  std::printf("  inputs: 1 MiB unless noted; ns = ns/search; GB/s = bytes scanned / search time; xN = engine/micron\n\n");
  std::printf("  %-20s %8s | %10s %7s | %10s %7s | %12s %6s | %12s %6s\n", "case", "len", "mc_hasm(ns)", "GB/s", "mc_srch(ns)", "GB/s",
              "std (ns)", "x.mc", "boost (ns)", "x.mc");
  std::printf("  %s\n", std::string(112, '-').c_str());

  for ( auto &c : cases ) {
    void *mh = mc_re_compile(c.pat);
    bool mc_valid_ok = mc_valid(mh);
    int path = mc_path(mh);
    int nst = mc_nstates(mh);
    const char *ptag = path == 2 ? "sheng" : path == 1 ? "table" : "pike";
    double in_bytes = (double)c.input.size();
    const char *ip = c.input.data();
    long il = (long)c.input.size();

    bool okA = false, okB = false, okC = false, okD = false;
    double mc_hm = bench_ns([&] { return (long)mc_has_match(mh, ip, il); }, okA, c.pathological);
    double mc_se = bench_ns([&] { return mc_search(mh, ip, il); }, okB, c.pathological);

    std::regex sre;
    bool std_compiled = true;
    try {
      sre.assign(c.pat, std::regex::extended);
    } catch ( ... ) {
      std_compiled = false;
    }
    double std_ns = -1;
    if ( std_compiled )
      std_ns = bench_ns([&] { return (long)std::regex_search(c.input.cbegin(), c.input.cend(), sre); }, okC, c.pathological);

    boost::regex bre;
    bool boost_compiled = true;
    try {
      bre.assign(c.pat, boost::regex::extended);
    } catch ( ... ) {
      boost_compiled = false;
    }
    double boost_ns = -1;
    if ( boost_compiled )
      boost_ns = bench_ns([&] { return (long)boost::regex_search(c.input.cbegin(), c.input.cend(), bre); }, okD, c.pathological);

    auto gb = [&](double ns) { return ns > 0 ? in_bytes / ns : 0.0; };      // bytes/ns == GB/s

    std::printf("  %-20s %8ld | ", c.name, il);
    std::printf("[%-5s] ", ptag);
    if ( okA )
      std::printf("%10.1f %7.2f | ", mc_hm, gb(mc_hm));
    else
      std::printf("%10s %7s | ", "-", "-");
    if ( okB )
      std::printf("%10.1f %7.2f | ", mc_se, gb(mc_se));
    else
      std::printf("%10s %7s | ", "-", "-");
    if ( std_compiled && okC )
      std::printf("%12.1f %6.1f | ", std_ns, mc_se > 0 ? std_ns / mc_se : 0.0);
    else
      std::printf("%12s %6s | ", std_compiled ? "THREW" : "n/a", "-");
    if ( boost_compiled && okD )
      std::printf("%12.1f %6.1f", boost_ns, mc_se > 0 ? boost_ns / mc_se : 0.0);
    else
      std::printf("%12s %6s", boost_compiled ? "THREW" : "n/a", "-");
    std::printf("\n");

    // correctness cross-check (boolean match agreement on this input)
    int m_mc = mc_has_match(mh, ip, il);
    int m_std = std_compiled ? (int)std::regex_search(c.input.cbegin(), c.input.cend(), sre) : m_mc;
    int m_bo = boost_compiled ? (c.pathological ? m_mc : (int)boost::regex_search(c.input.cbegin(), c.input.cend(), bre)) : m_mc;
    if ( !(m_mc == m_std && m_mc == m_bo) )
      std::printf("      !! disagreement: micron=%d std=%d boost=%d  pat=`%s`\n", m_mc, m_std, m_bo, c.pat);

    (void)mc_valid_ok;
    mc_re_free(mh);
  }
  std::printf("\n  (mc_hasm = micron has_match / Sheng+table DFA;  mc_srch = micron search / Pike VM + SIMD prefilter)\n");
  std::printf("  (THREW = backtracking engine hit its complexity limit on the pathological case)\n\n");
  return 0;
}
