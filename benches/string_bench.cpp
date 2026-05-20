// Benchmark harness comparing the new SIMD-backed `micron::hstring` /
// `micron::sstring` against the preserved scalar originals at
// `micron::__old::hstring` / `micron::__old::sstring`.
//
// Per (op, size, impl) cell the harness reports:
//   - cyc/op  (median across K_MEASUREMENTS samples)
//   - IPC     (instructions / cycles)
//   - bmiss%  (branch-misses / branches)
//
// Counters come from the same bbench 4-event group used by the other micron
// benchmarks. Per the project convention all SIMD widths are unlocked at
// compile time via -mavx2; the new path picks the AVX2 primitive while the
// `__old::` path keeps its scalar loops.
//
// Layout: identical to benches/cmemory_bench.cpp — same `line` formatter, same
// sched_setaffinity pin, same alignas(64) static buffers, same clobber. The
// `sink` global defeats DCE of boolean-returning ops.

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/std.hpp"

// __old_*.hpp first so they parse before `micron::sstr` enters scope (see
// note in tests/rigor/string_simd.cpp).
#include "../src/string/__old_sstring.hpp"
#include "../src/string/__old_string.hpp"
#include "../src/string/sstring.hpp"
#include "../src/string/string.hpp"

#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

namespace
{

// %%% Tunables %%%
constexpr u32 K_MEASUREMENTS = 7;
constexpr u64 WARMUP_REPS = 4;
constexpr u64 TARGET_BYTES_PER_MEAS = 1ULL << 22;      // 4 MiB worth of work
constexpr u64 MIN_REPS = 32;

using mem_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

// %%% Output formatting %%%
struct row {
  const char *op;
  const char *impl;
  u64 size;
  f64 cyc_per_op;
  f64 ipc;
  f64 bmiss_rate;
};

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v)
{
  if ( v < 0 ) v = 0;
  u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else {
      u64 vv = v;
      while ( vv ) {
        tmp[n++] = '0' + (vv % 10);
        vv /= 10;
      }
    }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = '0' + (w % 10);
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 / 10);
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 % 10);
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  void
  s_lj_at(const char *p, u32 end_col) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
    while ( pos < end_col ) buf[pos++] = ' ';
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

// Column right-edges: size=10, op=40, impl=50, cyc/op=62, IPC=72, bmiss=82.
[[gnu::cold]] void
print_header()
{
  line h;
  h.s_at("size(B)", 10);
  h.pad_to(12, 0);
  h.s_lj_at("op", 40);
  h.s_lj_at("impl", 50);
  h.s_at("cyc/op", 62);
  h.s_at("IPC", 72);
  h.s_at("bmiss%", 82);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------"
                      "--------------------------");
}

[[gnu::cold]] void
print_row(const row &r)
{
  fmt2 cpo = to_fmt2(r.cyc_per_op);
  fmt2 ipc = to_fmt2(r.ipc);
  fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.u_at(r.size, 10);
  ln.pad_to(12, 0);
  ln.s_lj_at(r.op, 40);
  ln.s_lj_at(r.impl, 50);
  ln.f2_at(cpo, 62);
  ln.f2_at(ipc, 72);
  ln.f2_at(bm, 82);
  micron::io::println(ln.str());
}

// %%% Anti-DCE %%%
static volatile u64 sink_u64 = 0;

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

[[gnu::always_inline]] inline void
sink_bool(bool b) noexcept
{
  sink_u64 += static_cast<u64>(b);
}

[[gnu::always_inline]] inline void
sink_size(usize v) noexcept
{
  sink_u64 += static_cast<u64>(v);
}

// %%% Median %%%
u64
median_u64(u64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; i++ ) {
    u64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; i++ ) {
    f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

struct sample {
  u64 cyc;
  u64 inst;
  u64 br;
  u64 bm;
};

template<typename Fn>
[[gnu::noinline]] sample
measure_once(Fn &&fn, u64 reps) noexcept
{
  mem_events evs{ bbench::quiet{} };
  evs.open();
  evs.begin();
  for ( u64 i = 0; i < reps; i++ ) fn();
  evs.end();
  return { static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve()),
           static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve()), static_cast<u64>(evs.get<bbench::branches>().retrieve()),
           static_cast<u64>(evs.get<bbench::branch_misses>().retrieve()) };
}

template<typename Fn>
row
bench_one(const char *op, const char *impl, u64 size, u64 bytes_per_op, Fn &&fn) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; i++ ) fn();

  u64 reps = TARGET_BYTES_PER_MEAS / (bytes_per_op == 0 ? 1 : bytes_per_op);
  if ( reps < MIN_REPS ) reps = MIN_REPS;
  if ( reps > (1ULL << 18) ) reps = 1ULL << 18;

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; m++ ) {
    sample s = measure_once(fn, reps);
    cpo_samples[m] = static_cast<f64>(s.cyc) / static_cast<f64>(reps);
    ipc_samples[m] = s.cyc > 0 ? static_cast<f64>(s.inst) / static_cast<f64>(s.cyc) : 0.0;
    bm_samples[m] = s.br > 0 ? static_cast<f64>(s.bm) / static_cast<f64>(s.br) : 0.0;
  }
  return row{
    op, impl, size, median_f64(cpo_samples, K_MEASUREMENTS), median_f64(ipc_samples, K_MEASUREMENTS), median_f64(bm_samples, K_MEASUREMENTS)
  };
}

// %%% Corpus %%%
constexpr u64 CORPUS_BYTES = 1ULL << 13;      // 8 KiB
alignas(64) static char g_lower_ascii[CORPUS_BYTES + 1];
alignas(64) static char g_upper_ascii[CORPUS_BYTES + 1];
alignas(64) static char g_mixed_ascii[CORPUS_BYTES + 1];
alignas(64) static char g_padded_ws[CORPUS_BYTES + 1];      // 32 leading + 32 trailing
                                                            // spaces around mixed

void
init_corpus()
{
  for ( u64 i = 0; i < CORPUS_BYTES; ++i ) {
    g_lower_ascii[i] = 'a' + static_cast<char>(i % 26);
    g_upper_ascii[i] = 'A' + static_cast<char>(i % 26);
    char m = (i & 1) ? ('A' + static_cast<char>(i % 26)) : ('a' + static_cast<char>(i % 26));
    g_mixed_ascii[i] = m;
    if ( i < 32 || i >= CORPUS_BYTES - 32 )
      g_padded_ws[i] = ' ';
    else
      g_padded_ws[i] = m;
  }
  g_lower_ascii[CORPUS_BYTES] = 0;
  g_upper_ascii[CORPUS_BYTES] = 0;
  g_mixed_ascii[CORPUS_BYTES] = 0;
  g_padded_ws[CORPUS_BYTES] = 0;
}

// %%% Sweep helpers %%%
constexpr u64 SIZES[] = { 16, 64, 256, 1024 };

// hstring sweeps
template<typename HS>
void
sweep_hstring_construct_copy_eq(const char *impl_tag)
{
  for ( u64 sz : SIZES ) {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
    tmp[sz] = 0;

    // construct from C-string
    {
      auto fn = [&]() {
        HS h(tmp);
        clobber(h.data());
      };
      print_row(bench_one("construct(C-str)", impl_tag, sz, sz, fn));
    }
    // copy-construct
    {
      HS src(tmp);
      auto fn = [&]() {
        HS h(src);
        clobber(h.data());
      };
      print_row(bench_one("copy-construct", impl_tag, sz, sz, fn));
    }
    // operator== equal
    {
      HS a(tmp), b(tmp);
      auto fn = [&]() { sink_bool(a == b); };
      print_row(bench_one("operator==(eq)", impl_tag, sz, sz, fn));
    }
    // operator== differ at last byte
    {
      char tmp2[1025];
      for ( u64 i = 0; i < sz; ++i ) tmp2[i] = tmp[i];
      tmp2[sz] = 0;
      if ( sz > 0 ) tmp2[sz - 1] ^= 0x01;
      HS a(tmp), b(tmp2);
      auto fn = [&]() { sink_bool(a == b); };
      print_row(bench_one("operator==(diff last)", impl_tag, sz, sz, fn));
    }
  }
}

template<typename HS>
void
sweep_hstring_search(const char *impl_tag)
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
  tmp[sz] = 0;
  HS h(tmp);

  // find(ch) early hit (idx 8)
  {
    char target = tmp[8];
    auto fn = [&]() { sink_size(h.find(target)); };
    print_row(bench_one("find(ch) early", impl_tag, sz, sz, fn));
  }
  // late hit (idx 1000)
  {
    char target = tmp[1000];
    // Make this character unique-ish by mutating earlier copies
    char tmp2[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp2[i] = (tmp[i] == target && i < 1000) ? '!' : tmp[i];
    tmp2[1000] = target;
    tmp2[sz] = 0;
    HS h2(tmp2);
    auto fn = [&]() { sink_size(h2.find(target)); };
    print_row(bench_one("find(ch) late", impl_tag, sz, sz, fn));
  }
  // miss
  {
    auto fn = [&]() { sink_size(h.find('@')); };
    print_row(bench_one("find(ch) miss", impl_tag, sz, sz, fn));
  }
  // find_substr 8-byte needle, late hit
  {
    const char *needle8 = "ZyXwVuTs";
    char tmp2[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp2[i] = tmp[i];
    for ( u64 i = 0; i < 8; ++i ) tmp2[900 + i] = needle8[i];
    tmp2[sz] = 0;
    HS h2(tmp2);
    auto fn = [&]() { sink_size(h2.find_substr(needle8, 8)); };
    print_row(bench_one("find_substr(8,late)", impl_tag, sz, sz, fn));
  }
  // miss
  {
    const char *needle8 = "ZyXwVuTs";
    auto fn = [&]() { sink_size(h.find_substr(needle8, 8)); };
    print_row(bench_one("find_substr(8,miss)", impl_tag, sz, sz, fn));
  }
}

// (hstring has no starts_with/ends_with in the public API — those live only on
// sstring. Prefix ops are exercised in the sstring sweep below.)

// %%% sstring-only operations %%%
template<usize N, typename SS>
void
sweep_sstring_case(const char *impl_tag, u64 sz)
{
  // input must avoid an internal NUL because the ctor uses strlen
  char tmp[1025];
  for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_upper_ascii[i];
  tmp[sz] = 0;
  SS s(tmp);
  auto fn_lower = [&]() {
    // restore to uppercase, then lower (work the SIMD path)
    for ( u64 i = 0; i < sz; ++i ) s.data()[i] = g_upper_ascii[i];
    s.to_lower();
    clobber(s.data());
  };
  print_row(bench_one("to_lower", impl_tag, sz, sz, fn_lower));

  auto fn_upper = [&]() {
    for ( u64 i = 0; i < sz; ++i ) s.data()[i] = g_lower_ascii[i];
    s.to_upper();
    clobber(s.data());
  };
  print_row(bench_one("to_upper", impl_tag, sz, sz, fn_upper));
}

template<usize N, typename SS>
void
sweep_sstring_prefix(const char *impl_tag)
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
  tmp[sz] = 0;
  SS s(tmp);

  char prefix[65];
  for ( u64 i = 0; i < 64; ++i ) prefix[i] = tmp[i];
  prefix[64] = 0;
  char prefix_bad[65];
  for ( u64 i = 0; i < 64; ++i ) prefix_bad[i] = tmp[i];
  prefix_bad[63] ^= 0x01;
  prefix_bad[64] = 0;
  char suffix[65];
  for ( u64 i = 0; i < 64; ++i ) suffix[i] = tmp[sz - 64 + i];
  suffix[64] = 0;

  {
    auto fn = [&]() { sink_bool(s.starts_with(prefix)); };
    print_row(bench_one("starts_with match", impl_tag, sz, 64, fn));
  }
  {
    auto fn = [&]() { sink_bool(s.starts_with(prefix_bad)); };
    print_row(bench_one("starts_with miss-last", impl_tag, sz, 64, fn));
  }
  {
    auto fn = [&]() { sink_bool(s.ends_with(suffix)); };
    print_row(bench_one("ends_with match", impl_tag, sz, 64, fn));
  }
}

template<usize N, typename SS>
void
sweep_sstring_misc(const char *impl_tag)
{
  constexpr u64 sz = 1024;

  // count(ch) — find a char that appears ~ 1/26 of the time
  {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
    tmp[sz] = 0;
    SS s(tmp);
    auto fn = [&]() { sink_size(s.count('A')); };
    print_row(bench_one("count(ch)", impl_tag, sz, sz, fn));
  }
  // reverse (mutating)
  {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
    tmp[sz] = 0;
    SS s(tmp);
    auto fn = [&]() {
      s.reverse();
      clobber(s.data());
    };
    print_row(bench_one("reverse", impl_tag, sz, sz, fn));
  }
  // find_first_of whitespace (mostly-no-hit, mostly-miss case on g_mixed)
  {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_padded_ws[i];      // 32 leading + 32 trailing spaces
    tmp[sz] = 0;
    SS s(tmp);
    auto fn = [&]() { sink_size(s.find_first_of(" \t\n\r")); };      // hits at 0
    print_row(bench_one("find_first_of(ws)", impl_tag, sz, sz, fn));
  }
  // trim (full mutate)
  {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_padded_ws[i];
    tmp[sz] = 0;
    auto fn = [&]() {
      SS s(tmp);
      s.trim_left();
      s.trim_right();
      clobber(s.data());
    };
    print_row(bench_one("trim", impl_tag, sz, sz, fn));
  }
}

template<usize N, typename SS>
void
sweep_sstring_reverse_sizes(const char *impl_tag)
{
  for ( u64 sz : SIZES ) {
    char tmp[1025];
    for ( u64 i = 0; i < sz; ++i ) tmp[i] = g_mixed_ascii[i];
    tmp[sz] = 0;
    SS s(tmp);
    auto fn = [&]() {
      s.reverse();
      clobber(s.data());
    };
    print_row(bench_one("reverse", impl_tag, sz, sz, fn));
  }
}

};      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  init_corpus();

  micron::io::println("=== micron string benchmark: new SIMD vs __old scalar ===");
  micron::io::println("warmup ", WARMUP_REPS, " reps; ", K_MEASUREMENTS, " median samples per cell");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");
  micron::io::println("");

  micron::io::println("[hstring]");
  print_header();
  sweep_hstring_construct_copy_eq<micron::hstring<char>>("new");
  sweep_hstring_construct_copy_eq<micron::__old::hstring<char>>("old");
  sweep_hstring_search<micron::hstring<char>>("new");
  sweep_hstring_search<micron::__old::hstring<char>>("old");

  micron::io::println("");
  micron::io::println("[sstring<1024>]");
  print_header();
  using new_ss = micron::sstring<2048, char>;
  using old_ss = micron::__old::sstring<2048, char>;
  for ( u64 sz : SIZES ) {
    sweep_sstring_case<2048, new_ss>("new", sz);
    sweep_sstring_case<2048, old_ss>("old", sz);
  }
  sweep_sstring_prefix<2048, new_ss>("new");
  sweep_sstring_prefix<2048, old_ss>("old");
  sweep_sstring_misc<2048, new_ss>("new");
  sweep_sstring_misc<2048, old_ss>("old");
  sweep_sstring_reverse_sizes<2048, new_ss>("new (rev)");
  sweep_sstring_reverse_sizes<2048, old_ss>("old (rev)");

  micron::io::println("");
  micron::io::println("=== done ===");
  micron::io::println("(anti-DCE sink: ", sink_u64, ")");
  return 0;
}

#pragma GCC pop_options
