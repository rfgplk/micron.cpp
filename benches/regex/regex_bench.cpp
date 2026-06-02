//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Driver TU of the regex comparison benchmark. Pits micron::regex (reached only
// through the extern "C" wrapper in regex_bench_micron.cpp) against five mature
// engines, all in (mostly) POSIX-ERE mode over identical inputs:
//   std::regex (libstdc++)   boost::regex   PCRE1 + JIT   vectorscan (Hyperscan)
// This TU contains NO micron headers, so <regex>/<boost/regex.hpp>/<hs/hs.h>/
// <pcre.h> cannot collide with micron's globally-injected SIMD intrinsics and
// base-type definitions; the engine is touched solely via the C entry points.
//
// Build (single g++ invocation, two isolated TUs -- no pkg-config needed, the
// vectorscan header lives at /usr/include/hs/hs.h):
//   g++ -O3 -std=c++23 -march=native -I./src
//       benches/regex/regex_bench_micron.cpp benches/regex/regex_bench.cpp
//       -lboost_regex -lhs -lpcre -o bin/regex_bench
//   (duck does not apply here -- this driver TU must stay micron-header-free)
//   colour: micron's columns are bold on a TTY; force with REGEX_BENCH_COLOR=1.
//
// Every measured cell adapts its repetition count to a wall-time target, so the
// fast (SIMD/DFA) engines and the slow (backtracking) engines are both timed
// accurately. Two columns are reported for every engine: raw ns/search and raw
// GB/s (= bytes scanned / search time); external engines additionally show a
// "/mc" ratio (engine time over the comparable micron time -- >1 means micron
// is faster). micron's has_match path is compared against vectorscan (both do a
// boolean "does it occur" scan); micron's search path is compared against
// std/boost/pcre (all return a leftmost match position).

#include <boost/regex.hpp>
#include <hs/hs.h>
#include <pcre.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <string>
#include <unistd.h>
#include <vector>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron engine -- reached only through these C symbols (see regex_bench_micron.cpp)
extern "C" {
void *mc_re_compile(const char *pat);
int mc_has_match(void *h, const char *in, long n);
long mc_search(void *h, const char *in, long n);
int mc_valid(void *h);
int mc_path(void *h);      // 3=Sheng DFA 2=table DFA 1=SIMD prefilter+Pike 0=plain Pike
int mc_nstates(void *h);
void mc_re_free(void *h);
}

using clk = std::chrono::steady_clock;

static double kTarget = 0.10;         // per-cell wall-time target (seconds)
static volatile long g_sink = 0;      // defeats dead-code elimination of the timed call

// adaptive timer: returns ns/op; ok=false if the call threw (backtracking blow-up)
template<class F>
static double
bench_ns(F &&f, bool &ok, bool pathological = false)
{
  ok = true;
  try {
    for ( int i = 0; i < 2 && !pathological; ++i ) g_sink ^= f();      // warmup
    long reps = 1;
    for ( ;; ) {
      auto t0 = clk::now();
      for ( long i = 0; i < reps; ++i ) g_sink ^= f();
      auto t1 = clk::now();
      double sec = std::chrono::duration<double>(t1 - t0).count();
      if ( sec > kTarget || reps > (1L << 30) ) return sec * 1e9 / (double)reps;
      reps *= 2;
    }
  } catch ( ... ) {
    ok = false;      // std/boost hit their complexity limit
    return -1.0;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vectorscan (Hyperscan) wrapper: block-mode boolean "does it match anywhere"
static int
hs_first_cb(unsigned, unsigned long long, unsigned long long, unsigned, void *ctx)
{
  *(int *)ctx = 1;
  return 1;      // non-zero -> terminate the scan at the first match
}

struct vscan_engine {
  hs_database_t *db = nullptr;
  hs_scratch_t *sc = nullptr;
  bool ok = false;

  void
  compile(const char *pat)
  {
    hs_compile_error_t *err = nullptr;
    unsigned flags = HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH | HS_FLAG_ALLOWEMPTY;
    if ( hs_compile(pat, flags, HS_MODE_BLOCK, nullptr, &db, &err) == HS_SUCCESS ) {
      ok = (hs_alloc_scratch(db, &sc) == HS_SUCCESS);
    } else {
      if ( err ) hs_free_compile_error(err);
      ok = false;
    }
  }

  bool
  has(const char *p, unsigned n) const
  {
    int hit = 0;
    hs_scan(db, p, n, 0, sc, hs_first_cb, &hit);
    return hit != 0;
  }

  ~vscan_engine()
  {
    if ( sc ) hs_free_scratch(sc);
    if ( db ) hs_free_database(db);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// PCRE1 wrapper with JIT compilation + a match-limit so catastrophic patterns
// cannot run away (they return PCRE_ERROR_MATCHLIMIT instead).
struct pcre_engine {
  pcre *re = nullptr;
  pcre_extra *ex = nullptr;
  bool owns_ex = false;
  bool ok = false;
  bool jit = false;

  void
  compile(const char *pat)
  {
    const char *e = nullptr;
    int eo = 0;
    re = pcre_compile(pat, 0, &e, &eo, nullptr);
    if ( !re ) {
      ok = false;
      return;
    }
    const char *se = nullptr;
    ex = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &se);      // builds the JIT code, if possible
    if ( !ex ) {
      ex = (pcre_extra *)std::calloc(1, sizeof(pcre_extra));
      owns_ex = true;
    }
    ex->flags |= PCRE_EXTRA_MATCH_LIMIT | PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    ex->match_limit = 2000000;
    ex->match_limit_recursion = 2000000;
    int hj = 0;
    pcre_fullinfo(re, ex, PCRE_INFO_JIT, &hj);
    jit = hj != 0;
    ok = true;
  }

  // 1 = match, 0 = no match, -1 = limit/error (catastrophic backtracking capped)
  int
  exec(const char *p, int n) const
  {
    int ov[6];
    int rc = pcre_exec(re, ex, p, n, 0, 0, ov, 6);
    if ( rc >= 0 ) return 1;
    if ( rc == PCRE_ERROR_NOMATCH ) return 0;
    return -1;
  }

  ~pcre_engine()
  {
    if ( ex ) {
      if ( owns_ex )
        std::free(ex);
      else
        pcre_free_study(ex);
    }
    if ( re ) pcre_free(re);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cell rendering -- every header label and every data value passes through the
// SAME fixed-width fields (see ROWFMT), so columns can never drift.
//   status: 1 ok | 0 threw | -1 limit | -2 not-compiled
static void
cell_ns(char *b, double ns, int status)
{
  if ( status == 1 )
    std::snprintf(b, 16, "%.1f", ns);
  else
    std::snprintf(b, 16, "%s", status == 0 ? "THREW" : status == -1 ? "LIMIT" : "n/a");
}

static void
cell_gb(char *b, double gb, int status)
{
  if ( status != 1 || gb <= 0.0 )
    std::snprintf(b, 16, "-");
  else if ( gb >= 99999.0 )
    std::snprintf(b, 16, ">99999");
  else
    std::snprintf(b, 16, "%.2f", gb);
}

static void
cell_x(char *b, double base_ns, double eng_ns, int status)
{
  double r = base_ns > 0.0 ? eng_ns / base_ns : -1.0;
  if ( status != 1 || base_ns <= 0.0 || eng_ns < 0.0 )
    std::snprintf(b, 16, "-");
  else if ( r >= 99999.0 )
    std::snprintf(b, 16, ">99999");
  else
    std::snprintf(b, 16, "%.1f", r);
}

// shared plain layout (no path column); also used for the header + dash-rule width.
#define ROWFMT "  %-20s %8s | %10s %7s | %10s %7s | %11s %7s %7s | %11s %7s %7s | %11s %7s %7s | %11s %7s %7s\n"

struct cells {
  char name[40], len[24];
  char ah[16], ag[16];              // micron has_match: ns, GB/s
  char bh[16], bg[16];              // micron search:    ns, GB/s
  char ch[16], cg[16], cx[16];      // std
  char dh[16], dg[16], dx[16];      // boost
  char eh[16], eg[16], ex[16];      // pcre
  char fh[16], fg[16], fx[16];      // vscan
};

// ANSI bold around micron's own columns (set from isatty / $REGEX_BENCH_COLOR).
// Emitted OUTSIDE the width-counted %-fields, so alignment is unaffected.
static const char *g_b = "";
static const char *g_r = "";

// prints one row; the micron block (has + search) is bolded. The plain layout is
// identical to ROWFMT (used for the header + the dash rule width), so columns
// stay perfectly aligned whether or not the bold escapes are present.
static void
prow(const cells &c)
{
  std::printf("  %-20s %8s | ", c.name, c.len);
  std::printf("%s%10s %7s | %10s %7s%s", g_b, c.ah, c.ag, c.bh, c.bg, g_r);
  std::printf(" | %11s %7s %7s | %11s %7s %7s | %11s %7s %7s | %11s %7s %7s\n", c.ch, c.cg, c.cx, c.dh, c.dg, c.dx, c.eh, c.eg, c.ex, c.fh,
              c.fg, c.fx);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct Case {
  const char *group;      // section divider when it changes
  const char *name;
  const char *pat;
  std::string input;
  bool patho;      // backtracking blow-up demo (tiny input)
  bool thru;       // full-scan throughput case (counts toward the geomean)
};

// build inputs: `n` copies of `pad`, then append `tail`
static std::string
mk(std::size_t n, char pad, const char *tail)
{
  std::string s(n, pad);
  s += tail;
  return s;
}

// literal then a long run then a literal (forces a full O(n) scan for `x.*y`)
static std::string
span(char a, std::size_t n, char fill, const char *tail)
{
  std::string s(1, a);
  s.append(n, fill);
  s += tail;
  return s;
}

int
main()
{
  // bold micron's own columns when writing to a terminal (override with
  // REGEX_BENCH_COLOR=1 to force, =0 to disable -- e.g. when piping to a file).
  const char *col = std::getenv("REGEX_BENCH_COLOR");
  bool use_bold = col ? (col[0] == '1') : (isatty(STDOUT_FILENO) != 0);
  if ( use_bold ) {
    g_b = "\033[1m";
    g_r = "\033[0m";
  }

  const std::size_t BIG = 1u << 20;      // 1 MiB default throughput input
  std::vector<Case> cs;

  // ---- literals -----------------------------------------------------------
  cs.push_back({ "literals", "lit_miss needle", "needle", mk(BIG, 'a', ""), false, true });
  cs.push_back({ "literals", "lit_hit needle", "needle", mk(BIG - 6, 'a', "needle"), false, true });
  cs.push_back({ "literals", "char_miss z", "z", mk(BIG, 'a', ""), false, true });
  cs.push_back({ "literals", "char_hit z", "z", mk(BIG - 1, 'a', "z"), false, true });
  cs.push_back({ "literals", "lit16_miss", "abcdefghijklmnop", mk(BIG, '.', ""), false, true });
  cs.push_back({ "literals", "lit16_hit", "abcdefghijklmnop", mk(BIG - 16, '.', "abcdefghijklmnop"), false, true });

  // ---- single character classes -------------------------------------------
  cs.push_back({ "classes", "digit_miss [0-9]+", "[0-9]+", mk(BIG, 'a', ""), false, true });
  cs.push_back({ "classes", "digit_hit [0-9]+", "[0-9]+", mk(BIG - 5, 'a', "12345"), false, true });
  cs.push_back({ "classes", "alpha [A-Za-z]+", "[A-Za-z]+", mk(BIG - 5, '0', "Hello"), false, true });
  cs.push_back({ "classes", "alnum [A-Za-z0-9]+", "[A-Za-z0-9]+", mk(BIG - 3, '!', "abc"), false, true });
  cs.push_back({ "classes", "hex [0-9a-fA-F]+", "[0-9a-fA-F]+", mk(BIG - 8, 'g', "DeadBeef"), false, true });
  cs.push_back({ "classes", "negated [^a]+", "[^a]+", mk(BIG, 'a', ""), false, true });
  cs.push_back({ "classes", "negated_hit [^a]+", "[^a]+", mk(BIG - 1, 'a', "X"), false, true });

  // ---- POSIX named classes ------------------------------------------------
  cs.push_back({ "posix", "[[:digit:]]+", "[[:digit:]]+", mk(BIG - 5, 'a', "12345"), false, true });
  cs.push_back({ "posix", "[[:alpha:]]+", "[[:alpha:]]+", mk(BIG - 5, '0', "Hello"), false, true });
  cs.push_back({ "posix", "[[:space:]]+", "[[:space:]]+", mk(BIG - 2, 'a', " \t"), false, true });
  cs.push_back({ "posix", "[[:punct:]]+", "[[:punct:]]+", mk(BIG - 3, 'a', "!?#"), false, true });
  cs.push_back({ "posix", "[x[:digit:]]+", "[x[:digit:]]+", mk(BIG - 4, 'A', "x1y2"), false, true });

  // ---- quantifiers & intervals --------------------------------------------
  cs.push_back({ "intervals", "exact a{16}", "a{16}", mk(BIG - 16, 'b', "aaaaaaaaaaaaaaaa"), false, true });
  cs.push_back({ "intervals", "exact_miss a{16}", "a{16}", mk(BIG, 'b', ""), false, true });
  cs.push_back({ "intervals", "atleast a{8,}", "a{8,}", mk(BIG - 8, 'b', "aaaaaaaa"), false, true });
  cs.push_back({ "intervals", "bounded a{10,20}", "a{10,20}", mk(BIG - 60, 'b', std::string(60, 'a').c_str()), false, true });
  cs.push_back({ "intervals", "optional colou?r", "colou?r", mk(BIG - 6, 'x', "colour"), false, true });

  // ---- alternation --------------------------------------------------------
  cs.push_back({ "alternation", "alt3 (foo|bar|baz)", "(foo|bar|baz)", mk(BIG - 3, 'x', "baz"), false, true });
  cs.push_back({ "alternation", "alt3_miss", "(foo|bar|baz)", mk(BIG, 'x', ""), false, true });
  cs.push_back({ "alternation", "alt10 words", "(alpha|bravo|charlie|delta|echo|foxtrot|golf|hotel|india|juliet)",
                 mk(BIG - 6, '_', "juliet"), false, true });
  cs.push_back({ "alternation", "alt_scheme", "(http|https|ftp|file)", mk(BIG - 4, '/', "file"), false, true });

  // ---- dot / greedy spans (literal-anchored start: stays O(n) in backtrackers) --
  cs.push_back({ "dot/greedy", "dotstar a.*z", "a.*z", span('a', BIG, 'x', "z"), false, true });
  cs.push_back({ "dot/greedy", "dotplus a.+z", "a.+z", span('a', BIG, 'x', "z"), false, true });
  cs.push_back({ "dot/greedy", "dotseq a.c.e", "a.c.e", mk(BIG - 5, 'x', "abcde"), false, true });
  cs.push_back({ "dot/greedy", "a.*z_miss", "a.*z", span('a', BIG, 'x', ""), false, true });

  // ---- anchored (the $ forces a full scan; no short-circuit) --------------
  cs.push_back({ "anchored", "full ^[a-z]+$", "^[a-z]+$", std::string(BIG, 'm'), false, true });
  cs.push_back({ "anchored", "full_miss ^[a-z]+$", "^[a-z]+$", mk(BIG - 1, 'm', "1"), false, true });
  cs.push_back({ "anchored", "tail [0-9]+$", "[0-9]+$", mk(BIG - 3, 'a', "123"), false, true });

  // ---- realistic shapes ---------------------------------------------------
  cs.push_back({ "realworld", "email", "[a-z]+@[a-z]+\\.[a-z]+", mk(BIG - 13, '0', "user@host.com"), false, true });
  cs.push_back({ "realworld", "ipv4", "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}", mk(BIG - 11, 'x', "192.168.0.1"), false, true });
  cs.push_back({ "realworld", "url", "(http|https)://[a-zA-Z0-9./]+", mk(BIG - 18, ' ', "https://host.com/x"), false, true });
  cs.push_back({ "realworld", "iso_date", "[0-9]{4}-[0-9]{2}-[0-9]{2}", mk(BIG - 10, 'x', "2026-06-01"), false, true });
  cs.push_back({ "realworld", "float", "[0-9]+\\.[0-9]+", mk(BIG - 7, 'x', "3.14159"), false, true });
  cs.push_back({ "realworld", "hexcolor #rrggbb", "#[0-9a-fA-F]{6}", mk(BIG - 7, 'x', "#1a2b3c"), false, true });
  cs.push_back({ "realworld", "uuid_head", "[0-9a-f]{8}-[0-9a-f]{4}", mk(BIG - 13, 'x', "deadbeef-1234"), false, true });

  // ---- input-size scaling (literal miss, pure scan) -----------------------
  cs.push_back({ "scaling", "scan 64KiB", "needle", mk(64u << 10, 'a', ""), false, true });
  cs.push_back({ "scaling", "scan 256KiB", "needle", mk(256u << 10, 'a', ""), false, true });
  cs.push_back({ "scaling", "scan 1MiB", "needle", mk(1u << 20, 'a', ""), false, true });
  cs.push_back({ "scaling", "scan 4MiB", "needle", mk(4u << 20, 'a', ""), false, true });

  // ---- pathological: catastrophic backtracking (tiny inputs; sizes picked so
  //      std/boost cannot stall the run). micron + vectorscan stay linear. ----
  cs.push_back({ "pathological", "(a+)+$", "(a+)+$", mk(20, 'a', "!"), true, false });
  cs.push_back({ "pathological", "(a*)*b", "(a*)*b", mk(12, 'a', "c"), true, false });
  cs.push_back({ "pathological", "(a|a)*c", "(a|a)*c", mk(18, 'a', "b"), true, false });
  cs.push_back({ "pathological", "(.*)*x", "(.*)*x", mk(12, 'a', ""), true, false });

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  std::printf("\n  micron::regex  vs  std::regex  vs  boost::regex  vs  PCRE1(+JIT)  vs  vectorscan/Hyperscan\n");
  std::printf("  POSIX-ERE patterns; ns = ns/search; GB/s = bytes scanned / search time; per-cell target %.0f ms.\n", kTarget * 1e3);
  std::printf("  /mc = engine time over micron (std/boost/pcre vs mc_srch; vscan vs mc_has). >1 -> micron faster.\n\n");

  // group banner: micron's has_match + search unite under the single public
  // "regex" entry point; each external engine spans its own (ns, GB/s, /mc)
  // block. Centered to match the ROWFMT column geometry exactly (left block 38,
  // the two micron sub-columns 43, every external block 28; " | " separators).
  {
    auto ctr = [](const char *s, int w) {
      int l = (int)std::strlen(s);
      if ( l >= w ) return std::string(s);
      int a = (w - l) / 2;
      return std::string(a, ' ') + s + std::string(w - l - a, ' ');
    };
    std::string gl = "  ";
    gl += ctr("", 29);
    gl += " | ", gl += g_b, gl += ctr("regex", 39), gl += g_r;      // micron banner, bolded
    gl += " | ", gl += ctr("std::regex", 27);
    gl += " | ", gl += ctr("boost::regex", 27);
    gl += " | ", gl += ctr("PCRE1 + JIT", 27);
    gl += " | ", gl += ctr("vectorscan", 27);
    std::printf("%s\n", gl.c_str());
  }

  cells h{};
  std::snprintf(h.name, 40, "%s", "case");
  std::snprintf(h.len, 16, "%s", "bytes");
  std::snprintf(h.ah, 16, "%s", "has ns");
  std::snprintf(h.ag, 16, "%s", "GB/s");
  std::snprintf(h.bh, 16, "%s", "srch ns");
  std::snprintf(h.bg, 16, "%s", "GB/s");
  std::snprintf(h.ch, 16, "%s", "ns");
  std::snprintf(h.cg, 16, "%s", "GB/s");
  std::snprintf(h.cx, 16, "%s", "/mc");
  std::snprintf(h.dh, 16, "%s", "ns");
  std::snprintf(h.dg, 16, "%s", "GB/s");
  std::snprintf(h.dx, 16, "%s", "/mc");
  std::snprintf(h.eh, 16, "%s", "ns");
  std::snprintf(h.eg, 16, "%s", "GB/s");
  std::snprintf(h.ex, 16, "%s", "/mc");
  std::snprintf(h.fh, 16, "%s", "ns");
  std::snprintf(h.fg, 16, "%s", "GB/s");
  std::snprintf(h.fx, 16, "%s", "/mc");

  // dash rule sized to the real header width
  char hbuf[512];
  int hw = std::snprintf(hbuf, sizeof hbuf, ROWFMT, h.name, h.len, h.ah, h.ag, h.bh, h.bg, h.ch, h.cg, h.cx, h.dh, h.dg, h.dx, h.eh, h.eg,
                         h.ex, h.fh, h.fg, h.fx);
  if ( hw > 0 && hbuf[hw - 1] == '\n' ) --hw;
  prow(h);
  std::printf("  %.*s\n", hw - 2 > 0 ? hw - 2 : 0, std::string(512, '-').c_str());

  // geomean accumulators: 0 mc_has 1 mc_srch 2 std 3 boost 4 pcre 5 vscan
  double sumlog[6] = { 0, 0, 0, 0, 0, 0 };
  int gcnt[6] = { 0, 0, 0, 0, 0, 0 };
  int disagreements = 0;
  const char *prev_group = nullptr;

  for ( auto &c : cs ) {
    if ( !prev_group || std::strcmp(prev_group, c.group) != 0 ) {
      std::printf("  %s\n", c.group);
      prev_group = c.group;
    }

    const char *ip = c.input.data();
    long il = (long)c.input.size();
    double bytes = (double)c.input.size();

    // ---- micron (the public regex entry point: has_match + search) ----
    void *mh = mc_re_compile(c.pat);
    int mc_ok = mc_valid(mh);
    bool okA = false, okB = false;
    double a_ns = -1, b_ns = -1;
    if ( mc_ok ) {
      a_ns = bench_ns([&] { return (long)mc_has_match(mh, ip, il); }, okA, c.patho);
      b_ns = bench_ns([&] { return mc_search(mh, ip, il); }, okB, c.patho);
    }
    int as_st = !mc_ok ? -2 : (okA ? 1 : 0);
    int bs_st = !mc_ok ? -2 : (okB ? 1 : 0);
    int m_mc = mc_ok ? mc_has_match(mh, ip, il) : 0;

    // ---- std::regex ----
    std::regex sre;
    bool std_comp = true;
    try {
      sre.assign(c.pat, std::regex::extended);
    } catch ( ... ) {
      std_comp = false;
    }
    bool okC = false;
    double c_ns = -1;
    if ( std_comp ) c_ns = bench_ns([&] { return (long)std::regex_search(c.input.cbegin(), c.input.cend(), sre); }, okC, c.patho);
    int cs_st = !std_comp ? -2 : (okC ? 1 : 0);

    // ---- boost::regex ----
    boost::regex bre;
    bool bo_comp = true;
    try {
      bre.assign(c.pat, boost::regex::extended);
    } catch ( ... ) {
      bo_comp = false;
    }
    bool okD = false;
    double d_ns = -1;
    if ( bo_comp ) d_ns = bench_ns([&] { return (long)boost::regex_search(c.input.cbegin(), c.input.cend(), bre); }, okD, c.patho);
    int ds_st = !bo_comp ? -2 : (okD ? 1 : 0);

    // ---- PCRE1 + JIT ----  (classify once: limit cases are not timed)
    pcre_engine pe;
    pe.compile(c.pat);
    int pe_class = pe.ok ? pe.exec(ip, (int)il) : -2;      // 1/0 ok, -1 limit, -2 n/a
    bool okE = false;
    double e_ns = -1;
    int es_st;
    if ( !pe.ok )
      es_st = -2;
    else if ( pe_class == -1 )
      es_st = -1;      // caught a runaway -> LIMIT, leave untimed
    else {
      e_ns = bench_ns([&] { return (long)pe.exec(ip, (int)il); }, okE, c.patho);
      es_st = okE ? 1 : 0;
    }

    // ---- vectorscan ----
    vscan_engine ve;
    ve.compile(c.pat);
    bool okF = false;
    double f_ns = -1;
    if ( ve.ok ) f_ns = bench_ns([&] { return (long)ve.has(ip, (unsigned)il); }, okF, c.patho);
    int fs_st = !ve.ok ? -2 : (okF ? 1 : 0);

    // ---- render the row ----
    cells z{};
    std::snprintf(z.name, 40, "%s", c.name);
    std::snprintf(z.len, sizeof z.len, "%ld", il);
    cell_ns(z.ah, a_ns, as_st);
    cell_gb(z.ag, bytes / a_ns, as_st);
    cell_ns(z.bh, b_ns, bs_st);
    cell_gb(z.bg, bytes / b_ns, bs_st);
    cell_ns(z.ch, c_ns, cs_st);
    cell_gb(z.cg, bytes / c_ns, cs_st);
    cell_x(z.cx, b_ns, c_ns, cs_st);      // std vs micron search
    cell_ns(z.dh, d_ns, ds_st);
    cell_gb(z.dg, bytes / d_ns, ds_st);
    cell_x(z.dx, b_ns, d_ns, ds_st);      // boost vs micron search
    cell_ns(z.eh, e_ns, es_st);
    cell_gb(z.eg, bytes / e_ns, es_st);
    cell_x(z.ex, b_ns, e_ns, es_st);      // pcre vs micron search
    cell_ns(z.fh, f_ns, fs_st);
    cell_gb(z.fg, bytes / f_ns, fs_st);
    cell_x(z.fx, a_ns, f_ns, fs_st);      // vscan vs micron has_match
    prow(z);

    // ---- geomean of throughput GB/s ----
    if ( c.thru ) {
      auto acc = [&](int i, double ns, bool ok) {
        if ( ok && ns > 0 ) {
          sumlog[i] += std::log(bytes / ns);
          ++gcnt[i];
        }
      };
      acc(0, a_ns, as_st == 1);
      acc(1, b_ns, bs_st == 1);
      acc(2, c_ns, cs_st == 1);
      acc(3, d_ns, ds_st == 1);
      acc(4, e_ns, es_st == 1);
      acc(5, f_ns, fs_st == 1);
    }

    // ---- correctness cross-check: only engines that returned a DEFINITE
    //      boolean are compared (-1 = skipped: limit/threw/too-slow-to-recheck).
    if ( mc_ok ) {
      int b_vs = ve.ok ? (int)ve.has(ip, (unsigned)il) : -1;
      int b_pc = -1, b_std = -1, b_bo = -1;
      if ( !c.patho ) {
        b_pc = (pe.ok && pe_class != -1) ? (pe_class == 1) : -1;
        // re-run std/boost only when they were cheap, so the check never stalls
        b_std = (std_comp && okC && c_ns >= 0 && c_ns < 2e6) ? (int)std::regex_search(c.input.cbegin(), c.input.cend(), sre) : -1;
        b_bo = (bo_comp && okD && d_ns >= 0 && d_ns < 2e6) ? (int)boost::regex_search(c.input.cbegin(), c.input.cend(), bre) : -1;
      }
      bool dis = false;
      for ( int v : { b_vs, b_pc, b_std, b_bo } )
        if ( v >= 0 && v != m_mc ) dis = true;
      if ( dis ) {
        auto tb = [](int v) { return v < 0 ? "?" : (v ? "1" : "0"); };
        std::printf("      !! disagreement: micron=%d std=%s boost=%s pcre=%s vscan=%s  pat=`%s`\n", m_mc, tb(b_std), tb(b_bo), tb(b_pc),
                    tb(b_vs), c.pat);
        ++disagreements;
      }
    }

    mc_re_free(mh);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  std::printf("\n  geometric-mean throughput over the %d full-scan cases (GB/s, higher is better):\n", gcnt[0]);
  const char *enames[6] = { "micron has_match", "micron search", "std::regex", "boost::regex", "PCRE1+JIT", "vectorscan" };
  for ( int i = 0; i < 6; ++i ) {
    double g = gcnt[i] ? std::exp(sumlog[i] / gcnt[i]) : 0.0;
    const char *b = i < 2 ? g_b : "";      // bold micron's two rows
    const char *r = i < 2 ? g_r : "";
    std::printf("    %s%-18s %8.2f GB/s%s   (n=%d)\n", b, enames[i], g, r, gcnt[i]);
  }

  std::printf("\n  legend:\n");
  std::printf("    regex = micron's public engine: has = has_match (Sheng/table DFA or SIMD prefilter),\n");
  std::printf("            srch = search (Pike VM + SIMD prefilter, returns the leftmost match position).\n");
  std::printf("    THREW = std/boost hit their backtracking complexity limit and bailed.\n");
  std::printf("    LIMIT = PCRE exceeded its 2,000,000 match-step budget (catastrophic backtracking, capped).\n");
  std::printf("    n/a   = engine could not compile the pattern.  '-' = not applicable.\n");
  std::printf("    vectorscan is a multi-match scanner (no POSIX leftmost-longest); it is therefore compared\n");
  std::printf("    only on the boolean has-match task, against micron's has_match path.\n");
  if ( disagreements == 0 )
    std::printf("\n  all engines agreed on every boolean match result.\n\n");
  else
    std::printf("\n  %d boolean disagreement(s) flagged above.\n\n", disagreements);
  return 0;
}
