// Per-member benchmark of micron::sstring<N, char>.
//
// Coverage scope per the approved plan:
//   construction / assignment / element access / capacity / modifiers /
//   search (already-SIMD) / search (scalar candidates) / starts_with /
//   ends_with / contains / mutation in place (to_lower, to_upper, reverse,
//   trim_left, trim_right, trim) / comparison / iterators.
//
// The "impl" column is repurposed to tag variant: "default", "early",
// "late", "miss", "k=1", "k=4", "k=8", etc. Sizes swept: 16/64/256/1024.

#include "_string_bench_common.hpp"

#include "../src/string/sstring.hpp"

#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

namespace
{

using mb::bench_one;
using mb::clobber;
using mb::g_lower_ascii;
using mb::g_mixed_ascii;
using mb::g_padded_ws;
using mb::g_upper_ascii;
using mb::print_header;
using mb::print_row;
using mb::sink_bool;
using mb::sink_char;
using mb::sink_ptr;
using mb::sink_size;

constexpr u64 SIZES[] = { 16, 64, 256, 1024 };

using SS = micron::sstring<2048, char>;

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
    char tmp[1025];
    fill_mixed(tmp, sz);

    print_row(bench_one("construct(default)", "default", sz, 16, [&]() {
      SS s;
      clobber(s.data());
    }));
    print_row(bench_one("construct(C-str)", "C-str", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      clobber(s.data());
    }));
    print_row(bench_one("construct(iter,iter)", "iter", sz, sz, [&]() {
      SS s(tmp, tmp + sz);
      clobber(s.data());
    }));
    SS src(as_cstr(tmp));
    print_row(bench_one("construct(copy)", "copy", sz, sz, [&]() {
      SS s(src);
      clobber(s.data());
    }));
    print_row(bench_one("construct(move)", "move", sz, sz, [&]() {
      SS donor(as_cstr(tmp));
      SS s(micron::move(donor));
      clobber(s.data());
    }));
  }
}

void
sweep_assign()
{
  for ( u64 sz : SIZES ) {
    char tmp[1025];
    fill_mixed(tmp, sz);
    SS src(as_cstr(tmp));
    SS dst;

    print_row(bench_one("operator=(copy)", "copy", sz, sz, [&]() {
      dst = src;
      clobber(dst.data());
    }));
    print_row(bench_one("operator=(move)", "move", sz, sz, [&]() {
      SS donor(as_cstr(tmp));
      dst = micron::move(donor);
      clobber(dst.data());
    }));
    const char *p = tmp;
    print_row(bench_one("operator=(C-str)", "C-str", sz, sz, [&]() {
      dst = p;
      clobber(dst.data());
    }));
  }
}

void
sweep_access()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  SS s(as_cstr(tmp));

  print_row(bench_one("operator[]", "mid", sz, 1, [&]() { sink_char(s[sz / 2]); }));
  print_row(bench_one("at(idx)", "mid", sz, 1, [&]() { sink_char(s.at(sz / 2)); }));
  print_row(bench_one("data()", "default", sz, 1, [&]() { sink_ptr(s.data()); }));
  print_row(bench_one("c_str()", "default", sz, 1, [&]() { sink_ptr(s.c_str()); }));
  print_row(bench_one("size()", "default", sz, 1, [&]() { sink_size(s.size()); }));
  print_row(bench_one("len()", "default", sz, 1, [&]() { sink_size(s.len()); }));
  print_row(bench_one("capacity()", "default", sz, 1, [&]() { sink_size(s.capacity()); }));
  print_row(bench_one("max_size()", "default", sz, 1, [&]() { sink_size(s.max_size()); }));
  print_row(bench_one("empty()", "default", sz, 1, [&]() { sink_bool(s.empty()); }));
  print_row(bench_one("into_chars()", "default", sz, 1, [&]() {
    auto sp = s.into_chars();
    sink_ptr(sp.data());
  }));
  print_row(bench_one("into_bytes()", "default", sz, 1, [&]() {
    auto sp = s.into_bytes();
    sink_ptr(sp.data());
  }));
}

void
sweep_iters()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  SS s(as_cstr(tmp));
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
    char tmp[1025];
    fill_mixed(tmp, sz);

    print_row(bench_one("push_back(ch)", "default", sz, 1, [&]() {
      SS s;
      for ( u64 i = 0; i < sz; ++i ) s.push_back('a');
      clobber(s.data());
    }));
    print_row(bench_one("pop_back", "default", sz, 1, [&]() {
      SS s(as_cstr(tmp));
      for ( u64 i = 0; i < sz; ++i ) s.pop_back();
      clobber(s.data());
    }));
    print_row(bench_one("clear", "default", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.clear();
      clobber(s.data());
    }));
    print_row(bench_one("fast_clear", "default", sz, 1, [&]() {
      SS s(as_cstr(tmp));
      s.fast_clear();
      clobber(s.data());
    }));
    {
      SS lhs(as_cstr(tmp));
      SS rhs(as_cstr(tmp));
      print_row(bench_one("append(sstring)", "default", sz, sz, [&]() {
        lhs.fast_clear();
        lhs.append(rhs);
        clobber(lhs.data());
      }));
    }
    print_row(bench_one("operator+=(ch)", "default", sz, 1, [&]() {
      SS s;
      for ( u64 i = 0; i < sz; ++i ) s += 'x';
      clobber(s.data());
    }));
    {
      SS rhs(as_cstr(tmp));
      print_row(bench_one("operator+=(sstring)", "default", sz, sz, [&]() {
        SS s;
        s += rhs;
        clobber(s.data());
      }));
    }
    print_row(bench_one("fill(ch)", "default", sz, sz, [&]() {
      SS s;
      s.fill('Z', sz);
      clobber(s.data());
    }));

    if ( sz <= 512 ) {
      print_row(bench_one("repeat(2)", "default", sz, sz, [&]() {
        SS s(as_cstr(tmp));
        s.repeat(2);
        clobber(s.data());
      }));
    }
    print_row(bench_one("insert(idx,ch,n)", "n=1", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.insert(static_cast<usize>(sz / 2), 'Q', 1);
      clobber(s.data());
    }));
    print_row(bench_one("insert(iter,ch,n)", "n=1", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.insert(s.begin() + sz / 2, 'Q', 1);
      clobber(s.data());
    }));
    print_row(bench_one("erase(idx,n)", "n=1", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.erase(static_cast<usize>(sz / 2), 1);
      clobber(s.data());
    }));
    print_row(bench_one("erase(iter,n)", "n=1", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.erase(s.begin() + sz / 2, 1);
      clobber(s.data());
    }));
    print_row(bench_one("truncate(idx)", "half", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.truncate(static_cast<usize>(sz / 2));
      clobber(s.data());
    }));
    print_row(bench_one("truncate(iter)", "half", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.truncate(s.begin() + sz / 2);
      clobber(s.data());
    }));
    print_row(bench_one("replace(pos,cnt,with)", "8b", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.replace(static_cast<usize>(sz / 2), static_cast<usize>(8), "REPLACE_");
      clobber(s.data());
    }));
    print_row(bench_one("replace_all(needle,with)", "1m", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.replace_all("Ab", "XY");
      clobber(s.data());
    }));
    print_row(bench_one("remove(needle)", "1m", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.remove("Ab");
      clobber(s.data());
    }));
    print_row(bench_one("remove_all(needle)", "scalar-loop", sz, sz, [&]() {
      SS s(as_cstr(tmp));
      s.remove_all("Ab");
      clobber(s.data());
    }));
  }
}

void
sweep_search_simd()
{
  for ( u64 sz : SIZES ) {
    char tmp[1025];
    fill_mixed(tmp, sz);
    SS s(as_cstr(tmp));

    if ( sz >= 16 ) {
      char ch_early = tmp[8];
      print_row(bench_one("find(ch)", "early", sz, sz, [&]() { sink_size(s.find(ch_early)); }));
      char ch_late = tmp[sz - 4];

      char tmp2[1025];
      for ( u64 i = 0; i < sz; ++i ) tmp2[i] = (tmp[i] == ch_late && i < sz - 4) ? '!' : tmp[i];
      tmp2[sz - 4] = ch_late;
      tmp2[sz] = 0;
      SS s2(as_cstr(tmp2));
      print_row(bench_one("find(ch)", "late", sz, sz, [&]() { sink_size(s2.find(ch_late)); }));
      print_row(bench_one("find(ch)", "miss", sz, sz, [&]() { sink_size(s.find('@')); }));

      print_row(bench_one("rfind(ch)", "late", sz, sz, [&]() { sink_size(s2.rfind(ch_late)); }));
      print_row(bench_one("rfind(ch)", "miss", sz, sz, [&]() { sink_size(s.rfind('@')); }));
    }
    if ( sz >= 64 ) {
      const char *needle8 = "ZyXwVuTs";
      char tmp3[1025];
      for ( u64 i = 0; i < sz; ++i ) tmp3[i] = tmp[i];
      for ( u64 i = 0; i < 8; ++i ) tmp3[sz - 16 + i] = needle8[i];
      tmp3[sz] = 0;
      SS s3(as_cstr(tmp3));
      print_row(bench_one("find_substr(8B)", "late", sz, sz, [&]() { sink_size(s3.find_substr(needle8, 8)); }));
      print_row(bench_one("find_substr(8B)", "miss", sz, sz, [&]() { sink_size(s.find_substr(needle8, 8)); }));
    }
    print_row(bench_one("count(ch)", "default", sz, sz, [&]() { sink_size(s.count('A')); }));
    print_row(bench_one("count(needle)", "scalar-loop", sz, sz, [&]() { sink_size(s.count("Ab")); }));
  }
}

void
sweep_search_charset()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  SS s(as_cstr(tmp));

  print_row(bench_one("find_first_of", "k=1", sz, sz, [&]() { sink_size(s.find_first_of("@")); }));
  print_row(bench_one("find_first_of", "k=2", sz, sz, [&]() { sink_size(s.find_first_of("@#")); }));
  print_row(bench_one("find_first_of", "k=4", sz, sz, [&]() { sink_size(s.find_first_of("@#$%")); }));
  print_row(bench_one("find_first_of", "k=8", sz, sz, [&]() { sink_size(s.find_first_of("@#$%^&*~")); }));

  print_row(bench_one("find_last_of", "k=1", sz, sz, [&]() { sink_size(s.find_last_of("@")); }));
  print_row(bench_one("find_last_of", "k=4-scalar", sz, sz, [&]() { sink_size(s.find_last_of("@#$%")); }));
  print_row(bench_one("find_last_of", "k=8-scalar", sz, sz, [&]() { sink_size(s.find_last_of("@#$%^&*~")); }));

  print_row(bench_one("find_first_not_of", "k=1-scalar", sz, sz, [&]() { sink_size(s.find_first_not_of("a")); }));
  print_row(bench_one("find_first_not_of", "k=4-scalar", sz, sz, [&]() { sink_size(s.find_first_not_of("abcd")); }));

  print_row(bench_one("find_last_not_of", "k=1-scalar", sz, sz, [&]() { sink_size(s.find_last_not_of("a")); }));
  print_row(bench_one("find_last_not_of", "k=4-scalar", sz, sz, [&]() { sink_size(s.find_last_not_of("abcd")); }));
}

void
sweep_test()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  SS s(as_cstr(tmp));

  char prefix[65];
  for ( u64 i = 0; i < 64; ++i ) prefix[i] = tmp[i];
  prefix[64] = 0;
  char suffix[65];
  for ( u64 i = 0; i < 64; ++i ) suffix[i] = tmp[sz - 64 + i];
  suffix[64] = 0;

  print_row(bench_one("starts_with(ch)", "default", sz, 1, [&]() { sink_bool(s.starts_with(tmp[0])); }));
  print_row(bench_one("starts_with(C-str)", "64B-hit", sz, 64, [&]() { sink_bool(s.starts_with(prefix)); }));
  print_row(bench_one("ends_with(ch)", "default", sz, 1, [&]() { sink_bool(s.ends_with(tmp[sz - 1])); }));
  print_row(bench_one("ends_with(C-str)", "64B-hit", sz, 64, [&]() { sink_bool(s.ends_with(suffix)); }));
  print_row(bench_one("contains(ch)", "hit", sz, sz, [&]() { sink_bool(s.contains('A')); }));
  print_row(bench_one("contains(C-str)", "8B-miss", sz, sz, [&]() { sink_bool(s.contains("ZyXwVuTs")); }));
}

void
sweep_mutate()
{
  for ( u64 sz : SIZES ) {
    char tmp_lower[1025];
    char tmp_upper[1025];
    char tmp_mixed[1025];
    char tmp_padded[1025];
    for ( u64 i = 0; i < sz; ++i ) {
      tmp_lower[i] = g_lower_ascii[i];
      tmp_upper[i] = g_upper_ascii[i];
      tmp_mixed[i] = g_mixed_ascii[i];
      tmp_padded[i] = g_padded_ws[i];
    }
    tmp_lower[sz] = 0;
    tmp_upper[sz] = 0;
    tmp_mixed[sz] = 0;
    tmp_padded[sz] = 0;

    SS s_upper(as_cstr(tmp_upper));
    print_row(bench_one("to_lower", "AVX2/NEON", sz, sz, [&]() {
      for ( u64 i = 0; i < sz; ++i ) s_upper.data()[i] = tmp_upper[i];
      s_upper.to_lower();
      clobber(s_upper.data());
    }));
    SS s_lower(as_cstr(tmp_lower));
    print_row(bench_one("to_upper", "AVX2/NEON", sz, sz, [&]() {
      for ( u64 i = 0; i < sz; ++i ) s_lower.data()[i] = tmp_lower[i];
      s_lower.to_upper();
      clobber(s_lower.data());
    }));
    print_row(bench_one("reverse", "AVX2/NEON", sz, sz, [&]() {
      SS s(as_cstr(tmp_mixed));
      s.reverse();
      clobber(s.data());
    }));
    print_row(bench_one("trim_left", "AVX2/NEON", sz, sz, [&]() {
      SS s(as_cstr(tmp_padded));
      s.trim_left();
      clobber(s.data());
    }));
    print_row(bench_one("trim_right", "scalar", sz, sz, [&]() {
      SS s(as_cstr(tmp_padded));
      s.trim_right();
      clobber(s.data());
    }));
    print_row(bench_one("trim", "L=SIMD+R=scalar", sz, sz, [&]() {
      SS s(as_cstr(tmp_padded));
      s.trim();
      clobber(s.data());
    }));
  }
}

void
sweep_compare()
{
  for ( u64 sz : SIZES ) {
    char tmp[1025];
    char tmp2[1025];
    fill_mixed(tmp, sz);
    fill_mixed(tmp2, sz);
    if ( sz > 0 ) tmp2[sz - 1] ^= 1;
    SS a(as_cstr(tmp)), b(tmp), c(tmp2);

    print_row(bench_one("operator==(eq)", "eq", sz, sz, [&]() { sink_bool(a == b); }));
    print_row(bench_one("operator==(diff)", "diff-last", sz, sz, [&]() { sink_bool(a == c); }));
    print_row(bench_one("operator<", "diff-last", sz, sz, [&]() { sink_bool(a < c); }));
    print_row(bench_one("compare(C-str)", "eq", sz, sz, [&]() { sink_size(static_cast<usize>(a.compare(tmp) + 1)); }));
  }
}

}      // namespace

int
main(void)
{
  mb::pin_cpu0();
  mb::init_corpus();
  mb::print_preamble("micron::sstring<2048,char> per-member benchmark");

  micron::io::println("[sstring: construct/assign]");
  print_header();
  sweep_construct();
  sweep_assign();

  micron::io::println("");
  micron::io::println("[sstring: access/capacity/iters]");
  print_header();
  sweep_access();
  sweep_iters();

  micron::io::println("");
  micron::io::println("[sstring: modifiers]");
  print_header();
  sweep_modifiers();

  micron::io::println("");
  micron::io::println("[sstring: search - single byte / substr (SIMD)]");
  print_header();
  sweep_search_simd();

  micron::io::println("");
  micron::io::println("[sstring: search - charset (mixed SIMD/scalar)]");
  print_header();
  sweep_search_charset();

  micron::io::println("");
  micron::io::println("[sstring: starts_with/ends_with/contains]");
  print_header();
  sweep_test();

  micron::io::println("");
  micron::io::println("[sstring: in-place mutate]");
  print_header();
  sweep_mutate();

  micron::io::println("");
  micron::io::println("[sstring: comparison]");
  print_header();
  sweep_compare();

  mb::print_epilogue();
  return 0;
}

#pragma GCC pop_options
