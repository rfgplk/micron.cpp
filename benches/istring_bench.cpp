// Per-member benchmark of micron::istring<char>.
//
// istring is move-only and immutable: every "mutating" op returns a fresh
// istring (one alloc + one or more memcpy + a destructor for the old copy).
// That cost is part of what the user experiences and is included in each
// cell. The class is much smaller than sstring/hstring (one-shot construct,
// minimal mutation surface) but ships several known footguns that the
// benches deliberately exercise:
//
//   - `find(const istring &, pos)` at istring.hpp:268-271 is a stub that
//     unconditionally returns `npos`. The bench pins the current cost so
//     the latency is measurable; once the stub is replaced the bench shows
//     the gain. See the optimisation report.
//   - `find(F ch, pos)` is a scalar `for ... if == ch` loop — every
//     other string class in this tree routes the equivalent through
//     `__simd_find_byte` (AVX2 / SSE2 / NEON). Anomaly candidate.
//   - `operator==` overloads at istring.hpp:633-702 are scalar bytewise
//     loops; all other string classes route through `__lexcmp` which uses
//     `micron::memcmp<byte>` (already SIMD). Anomaly candidate.
//
// Sizes swept: 16/64/256/1024/4096.

#include "_string_bench_common.hpp"

#include "../src/string/istring.hpp"
#include "../src/string/sstring.hpp"

#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

namespace
{

using mb::clobber;
using mb::g_mixed_ascii;
using mb::print_header;
using mb::print_row;
using mb::sink_bool;
using mb::sink_char;
using mb::sink_ptr;
using mb::sink_size;

using IS = micron::istring<char>;
using SS = micron::sstring<2048, char>;

constexpr u64 SIZES[] = { 16, 64, 256, 1024, 4096 };

constexpr u64 IS_MAX_REPS = 1ULL << 13;

template<typename Fn>
inline mb::row
bench_one(const char *op, const char *impl, u64 size, u64 bytes_per_op, Fn &&fn)
{
  return mb::bench_one(op, impl, size, bytes_per_op, micron::forward<Fn>(fn), IS_MAX_REPS);
}

void
fill_mixed(char *dst, u64 sz)
{
  for ( u64 i = 0; i < sz; ++i ) dst[i] = g_mixed_ascii[i];
  dst[sz] = 0;
}

[[gnu::always_inline]] inline const char *
as_cstr(const char *p) noexcept
{
  return p;
}

void
sweep_construct()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);

    print_row(bench_one("construct(default)", "default", sz, 16, [&]() {
      IS s;
      clobber(s.data());
    }));
    print_row(bench_one("construct(usize n)", "prealloc", sz, sz, [&]() {
      IS s(sz);
      clobber(s.data());
    }));
    print_row(bench_one("construct(n, ch)", "fill", sz, sz, [&]() {
      IS s(sz, 'x');
      clobber(s.data());
    }));
    print_row(bench_one("construct(C-str)", "C-str", sz, sz, [&]() {
      const char *p = as_cstr(tmp);
      IS s(p);
      clobber(s.data());
    }));
    print_row(bench_one("construct(move)", "move", sz, sz, [&]() {
      const char *p = as_cstr(tmp);
      IS donor(p);
      IS s(micron::move(donor));
      clobber(s.data());
    }));
  }
}

void
sweep_access()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  const char *p = as_cstr(tmp);
  IS s(p);

  print_row(bench_one("operator[]", "mid", sz, 1, [&]() { sink_char(s[sz / 2]); }));
  print_row(bench_one("at(idx)", "mid", sz, 1, [&]() { sink_char(s.at(sz / 2)); }));
  print_row(bench_one("data()", "default", sz, 1, [&]() { sink_ptr(s.data()); }));
  print_row(bench_one("c_str()", "default", sz, 1, [&]() { sink_ptr(s.c_str()); }));
  print_row(bench_one("w_str()", "default", sz, 1, [&]() { sink_ptr(s.w_str()); }));
  print_row(bench_one("uni_str()", "default", sz, 1, [&]() { sink_ptr(s.uni_str()); }));
  print_row(bench_one("size()", "default", sz, 1, [&]() { sink_size(s.size()); }));
  print_row(bench_one("max_size()", "default", sz, 1, [&]() { sink_size(s.max_size()); }));
  print_row(bench_one("empty()", "default", sz, 1, [&]() { sink_bool(s.empty()); }));
  print_row(bench_one("into_bytes()", "default", sz, 1, [&]() {
    auto sl = s.into_bytes();
    sink_ptr(sl.begin());
  }));
  print_row(bench_one("begin()", "default", sz, 1, [&]() { sink_ptr(s.begin()); }));
  print_row(bench_one("end()", "default", sz, 1, [&]() { sink_ptr(s.end()); }));
  print_row(bench_one("cbegin()", "default", sz, 1, [&]() { sink_ptr(s.cbegin()); }));
  print_row(bench_one("cend()", "default", sz, 1, [&]() { sink_ptr(s.cend()); }));
  print_row(bench_one("last()", "default", sz, 1, [&]() { sink_ptr(s.last()); }));
}

void
sweep_modifiers()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);
    const char *p = as_cstr(tmp);

    print_row(bench_one("clear()", "full-wipe", sz, sz, [&]() {
      IS s(p);
      s.clear();
      clobber(s.data());
    }));
    print_row(bench_one("fast_clear()", "len=0", sz, 1, [&]() {
      IS s(p);
      s.fast_clear();
      clobber(s.data());
    }));
    print_row(bench_one("append(ptr,n)", "8B", sz, sz, [&]() {
      IS s(p);
      auto t = s.append("abcdefgh", 8);
      clobber(t.data());
    }));
    {
      IS rhs(p);
      print_row(bench_one("append(istring)", "default", sz, sz, [&]() {
        IS s(p);
        auto t = s.append(rhs);
        clobber(t.data());
      }));
    }
    if ( sz < 2048 ) {
      SS rhs_s(p);
      print_row(bench_one("append(sstring)", "default", sz, sz, [&]() {
        IS s(p);
        auto t = s.append(rhs_s);
        clobber(t.data());
      }));
    }
    print_row(bench_one("push_back(ch)", "default", sz, 1, [&]() {
      IS s(p);
      auto t = s.push_back('x');
      clobber(t.data());
    }));
    {
      IS rhs(p);
      print_row(bench_one("push_back(istring)", "default", sz, sz, [&]() {
        IS s(p);
        auto t = s.push_back(rhs);
        clobber(t.data());
      }));
    }
    print_row(bench_one("insert(idx,ch,n)", "n=1", sz, sz, [&]() {
      IS s(p);
      auto t = s.insert(sz / 2, 'Q', 1);
      clobber(t.data());
    }));
    print_row(bench_one("operator+=(ptr)", "default", sz, sz, [&]() {
      IS s(p);
      const char *q = p;
      auto t = s += q;
      clobber(t.data());
    }));
    {
      IS rhs(p);
      print_row(bench_one("operator+=(istring)", "default", sz, sz, [&]() {
        IS s(p);
        auto t = s += rhs;
        clobber(t.data());
      }));
    }
    print_row(bench_one("substr(pos,cnt)", "half", sz, sz, [&]() {
      IS s(p);
      auto t = s.substr(0, sz / 2);
      clobber(t.data());
    }));
  }
}

void
sweep_search()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);
    const char *p = as_cstr(tmp);
    IS s(p);

    if ( sz >= 16 ) {
      char ch_early = tmp[8];
      print_row(bench_one("find(ch)", "scalar early", sz, sz, [&]() { sink_size(s.find(ch_early)); }));
      char ch_late = tmp[sz - 4];
      char tmp2[4097];
      for ( u64 i = 0; i < sz; ++i ) tmp2[i] = (tmp[i] == ch_late && i < sz - 4) ? '!' : tmp[i];
      tmp2[sz - 4] = ch_late;
      tmp2[sz] = 0;
      const char *q = as_cstr(tmp2);
      IS s2(q);
      print_row(bench_one("find(ch)", "scalar late", sz, sz, [&]() { sink_size(s2.find(ch_late)); }));
      print_row(bench_one("find(ch)", "scalar miss", sz, sz, [&]() { sink_size(s.find('@')); }));
    }
    {
      IS needle(p);
      print_row(bench_one("find(istring)", "STUB->npos", sz, sz, [&]() { sink_size(s.find(needle)); }));
    }
  }
}

void
sweep_compare()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    char tmp2[4097];
    fill_mixed(tmp, sz);
    fill_mixed(tmp2, sz);
    if ( sz > 0 ) tmp2[sz - 1] ^= 1;
    const char *p = as_cstr(tmp);
    const char *q = as_cstr(tmp2);
    IS a(p), b(p), c(q);

    print_row(bench_one("operator==(C-str,eq)", "scalar", sz, sz, [&]() { sink_bool(a == p); }));
    print_row(bench_one("operator==(C-str,diff)", "scalar diff", sz, sz, [&]() { sink_bool(a == q); }));
    print_row(bench_one("operator==(istring,eq)", "scalar", sz, sz, [&]() { sink_bool(a == b); }));
    print_row(bench_one("operator==(istring,diff)", "scalar diff", sz, sz, [&]() { sink_bool(a == c); }));
  }
}

void
sweep_misc()
{
  for ( u64 sz : { (u64)16, (u64)64, (u64)200 } ) {
    char tmp[256];
    fill_mixed(tmp, sz);
    const char *p = as_cstr(tmp);
    IS s(p);
    print_row(bench_one("stack()", "->sstring<256>", sz, sz, [&]() {
      auto ss = s.stack();
      clobber(ss.data());
    }));
  }
  for ( u64 sz : { (u64)64, (u64)256, (u64)1024 } ) {
    char tmp[1025];
    char tmp2[1025];
    fill_mixed(tmp, sz);
    fill_mixed(tmp2, sz);
    const char *p = as_cstr(tmp);
    const char *q = as_cstr(tmp2);
    IS a(p), b(q);
    print_row(bench_one("swap(istring)", "default", sz, 1, [&]() {
      a.swap(b);
      clobber(a.data());
    }));
  }
}

}      // namespace

int
main(void)
{
  mb::pin_cpu0();
  mb::init_corpus();
  mb::print_preamble("micron::istring<char> per-member benchmark");

  micron::io::println("[istring: construct]");
  print_header();
  sweep_construct();

  micron::io::println("");
  micron::io::println("[istring: access/capacity/iters]");
  print_header();
  sweep_access();

  micron::io::println("");
  micron::io::println("[istring: modifiers (each returns new istring)]");
  print_header();
  sweep_modifiers();

  micron::io::println("");
  micron::io::println("[istring: search (scalar — anomaly candidates)]");
  print_header();
  sweep_search();

  micron::io::println("");
  micron::io::println("[istring: comparison (scalar — anomaly candidates)]");
  print_header();
  sweep_compare();

  micron::io::println("");
  micron::io::println("[istring: misc (stack/swap)]");
  print_header();
  sweep_misc();

  mb::print_epilogue();
  return 0;
}

#pragma GCC pop_options
