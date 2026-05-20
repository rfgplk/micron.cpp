

#include "_string_bench_common.hpp"

#include "../src/string/sstring.hpp"
#include "../src/string/string.hpp"

#pragma GCC push_options
#pragma GCC optimize("-fno-tree-loop-distribute-patterns")

namespace
{

using mb::bench_one;
using mb::clobber;
using mb::g_mixed_ascii;
using mb::print_header;
using mb::print_row;
using mb::sink_bool;
using mb::sink_char;
using mb::sink_ptr;
using mb::sink_size;

using HS = micron::string;
using SS = micron::sstring<2048, char>;

constexpr u64 SIZES[] = { 16, 64, 256, 1024, 4096 };

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
      HS s;
      clobber(s.data());
    }));
    print_row(bench_one("construct(usize n)", "prealloc", sz, sz, [&]() {
      HS s(sz);
      clobber(s.data());
    }));
    print_row(bench_one("construct(n, ch)", "fill", sz, sz, [&]() {
      HS s(sz, 'x');
      clobber(s.data());
    }));
    print_row(bench_one("construct(C-str)", "C-str", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      clobber(s.data());
    }));
    print_row(bench_one("construct(iter,iter)", "iter", sz, sz, [&]() {
      HS s(tmp, tmp + sz);
      clobber(s.data());
    }));
    HS src(as_cstr(tmp));
    print_row(bench_one("construct(copy)", "copy", sz, sz, [&]() {
      HS s(src);
      clobber(s.data());
    }));
    print_row(bench_one("construct(move)", "move", sz, sz, [&]() {
      HS donor(as_cstr(tmp));
      HS s(micron::move(donor));
      clobber(s.data());
    }));
    if ( sz < 2048 ) {
      SS ss(as_cstr(tmp));
      print_row(bench_one("construct(sstring)", "from-stack", sz, sz, [&]() {
        HS s(ss);
        clobber(s.data());
      }));
    }
  }
}

void
sweep_assign()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);
    HS src(as_cstr(tmp));

    print_row(bench_one("operator=(copy)", "copy", sz, sz, [&]() {
      HS dst(sz);
      dst = src;
      clobber(dst.data());
    }));
    print_row(bench_one("operator=(move)", "move", sz, sz, [&]() {
      HS dst(sz);
      HS donor(as_cstr(tmp));
      dst = micron::move(donor);
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
  HS s(as_cstr(tmp));

  print_row(bench_one("operator[]", "mid", sz, 1, [&]() { sink_char(s[sz / 2]); }));
  print_row(bench_one("at(idx)", "mid", sz, 1, [&]() { sink_char(s.at(sz / 2)); }));
  print_row(bench_one("data()", "default", sz, 1, [&]() { sink_ptr(s.data()); }));
  print_row(bench_one("c_str()", "default", sz, 1, [&]() { sink_ptr(s.c_str()); }));
  print_row(bench_one("front()", "default", sz, 1, [&]() { sink_char(s.front()); }));
  print_row(bench_one("back()", "default", sz, 1, [&]() { sink_char(s.back()); }));
  print_row(bench_one("size()", "default", sz, 1, [&]() { sink_size(s.size()); }));
  print_row(bench_one("len()", "default", sz, 1, [&]() { sink_size(s.len()); }));
  print_row(bench_one("capacity()", "default", sz, 1, [&]() { sink_size(s.max_size()); }));
  print_row(bench_one("max_size()", "default", sz, 1, [&]() { sink_size(s.max_size()); }));
  print_row(bench_one("empty()", "default", sz, 1, [&]() { sink_bool(s.empty()); }));
  print_row(bench_one("into_chars()", "default", sz, 1, [&]() {
    auto sl = s.into_chars();
    sink_ptr(sl.begin());
  }));
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
sweep_capacity()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);

    print_row(bench_one("reserve(grow)", "x2", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.reserve(sz * 2 + 16);
      clobber(s.data());
    }));
    print_row(bench_one("reserve(no-op)", "n<cap", sz, 1, [&]() {
      HS s(as_cstr(tmp));
      s.reserve(1);
      clobber(s.data());
    }));
    print_row(bench_one("resize(grow,ch)", "x2", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.resize(sz * 2, 'x');
      clobber(s.data());
    }));
    print_row(bench_one("clear()", "full-wipe", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.clear();
      clobber(s.data());
    }));
    print_row(bench_one("fast_clear()", "len=0", sz, 1, [&]() {
      HS s(as_cstr(tmp));
      s.fast_clear();
      clobber(s.data());
    }));
  }
}

void
sweep_modifiers()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);

    print_row(bench_one("push_back(ch)", "default", sz, 1, [&]() {
      HS s(sz + 16);
      for ( u64 i = 0; i < sz; ++i ) s.push_back('a');
      clobber(s.data());
    }));
    print_row(bench_one("pop_back", "default", sz, 1, [&]() {
      HS s(as_cstr(tmp));
      for ( u64 i = 0; i < sz; ++i ) s.pop_back();
      clobber(s.data());
    }));
    {
      HS rhs(as_cstr(tmp));
      print_row(bench_one("append(hstring)", "default", sz, sz, [&]() {
        HS s(sz + 16);
        s.append(rhs);
        clobber(s.data());
      }));
    }
    if ( sz < 2048 ) {
      SS rhs_s(as_cstr(tmp));
      print_row(bench_one("append(sstring)", "default", sz, sz, [&]() {
        HS s(sz + 16);
        s.append(rhs_s);
        clobber(s.data());
      }));
    }
    print_row(bench_one("append(arr)", "8B", sz, 8, [&]() {
      HS s(sz + 16);
      s.append("abcdefgh");
      clobber(s.data());
    }));
    {
      HS rhs(as_cstr(tmp));
      print_row(bench_one("operator+=(hstring)", "default", sz, sz, [&]() {
        HS s(sz + 16);
        s += rhs;
        clobber(s.data());
      }));
    }
    print_row(bench_one("operator+=(ch)", "default", sz, 1, [&]() {
      HS s(sz + 16);
      for ( u64 i = 0; i < sz; ++i ) s += 'x';
      clobber(s.data());
    }));
    print_row(bench_one("operator+=(C-str)", "8B", sz, 8, [&]() {
      HS s(sz + 16);
      s += "abcdefgh";
      clobber(s.data());
    }));
    print_row(bench_one("insert(idx,ch,n)", "n=1", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.insert(sz / 2, 'Q', 1);
      clobber(s.data());
    }));
    print_row(bench_one("insert(idx,arr)", "8B", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.insert(sz / 2, "abcdefgh", 1);
      clobber(s.data());
    }));
    print_row(bench_one("insert(iter,ch,n)", "n=1", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.insert(s.begin() + sz / 2, 'Q', 1);
      clobber(s.data());
    }));
    print_row(bench_one("erase(idx,n)", "n=1", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.erase(sz / 2, 1);
      clobber(s.data());
    }));
    print_row(bench_one("erase(iter,n)", "n=1", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.erase(s.begin() + sz / 2, 1);
      clobber(s.data());
    }));
    print_row(bench_one("truncate(idx)", "half", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.truncate(static_cast<usize>(sz / 2));
      clobber(s.data());
    }));
    print_row(bench_one("truncate(iter)", "half", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.truncate(s.begin() + sz / 2);
      clobber(s.data());
    }));
  }
}

void
sweep_search()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);
    HS s(as_cstr(tmp));

    if ( sz >= 16 ) {
      char ch_early = tmp[8];
      print_row(bench_one("find(ch)", "early", sz, sz, [&]() { sink_size(s.find(ch_early)); }));

      char ch_late = tmp[sz - 4];
      char tmp2[4097];
      for ( u64 i = 0; i < sz; ++i ) tmp2[i] = (tmp[i] == ch_late && i < sz - 4) ? '!' : tmp[i];
      tmp2[sz - 4] = ch_late;
      tmp2[sz] = 0;
      HS s2(as_cstr(tmp2));
      print_row(bench_one("find(ch)", "late", sz, sz, [&]() { sink_size(s2.find(ch_late)); }));
      print_row(bench_one("find(ch)", "miss", sz, sz, [&]() { sink_size(s.find('@')); }));
    }
    if ( sz >= 64 ) {
      const char *needle8 = "ZyXwVuTs";
      char tmp3[4097];
      for ( u64 i = 0; i < sz; ++i ) tmp3[i] = tmp[i];
      for ( u64 i = 0; i < 8; ++i ) tmp3[sz - 16 + i] = needle8[i];
      tmp3[sz] = 0;
      HS s3(as_cstr(tmp3));
      print_row(bench_one("find_substr(8B)", "late", sz, sz, [&]() { sink_size(s3.find_substr(needle8, 8)); }));
      print_row(bench_one("find_substr(8B)", "miss", sz, sz, [&]() { sink_size(s.find_substr(needle8, 8)); }));
    }
  }
}

void
sweep_remove()
{
  for ( u64 sz : SIZES ) {
    char tmp[4097];
    fill_mixed(tmp, sz);

    print_row(bench_one("remove(C-str)", "1m", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.remove("Ab");
      clobber(s.data());
    }));
    {
      SS needle("Ab");
      print_row(bench_one("remove(sstring)", "1m", sz, sz, [&]() {
        HS s(as_cstr(tmp));
        s.remove(needle);
        clobber(s.data());
      }));
    }
    {
      HS needle("Ab");
      print_row(bench_one("remove(hstring)", "1m", sz, sz, [&]() {
        HS s(as_cstr(tmp));
        s.remove(needle);
        clobber(s.data());
      }));
    }
    print_row(bench_one("remove_all(C-str)", "scalar-loop", sz, sz, [&]() {
      HS s(as_cstr(tmp));
      s.remove_all("Ab");
      clobber(s.data());
    }));
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
    HS a(as_cstr(tmp)), b(tmp), c(tmp2);

    print_row(bench_one("operator==(C-str,eq)", "C-str", sz, sz, [&]() { sink_bool(a == tmp); }));
    print_row(bench_one("operator==(hstring,eq)", "hstring", sz, sz, [&]() { sink_bool(a == b); }));
    print_row(bench_one("operator==(hstring,diff)", "hstring-diff", sz, sz, [&]() { sink_bool(a == c); }));
    if ( sz < 2048 ) {
      SS as(as_cstr(tmp));
      print_row(bench_one("operator==(sstring)", "sstring", sz, sz, [&]() { sink_bool(a == as); }));
    }
    print_row(bench_one("operator!=(C-str)", "C-str", sz, sz, [&]() { sink_bool(a != tmp); }));
    print_row(bench_one("operator<(C-str)", "C-str", sz, sz, [&]() { sink_bool(a < tmp); }));
  }
}

void
sweep_conv()
{

  for ( u64 sz : { (u64)16, (u64)64, (u64)200 } ) {
    char tmp[256];
    fill_mixed(tmp, sz);
    HS s(as_cstr(tmp));
    print_row(bench_one("stack()", "->sstring<256>", sz, sz, [&]() {
      auto ss = s.stack();
      clobber(ss.data());
    }));
  }
}

}      // namespace

int
main(void)
{
  mb::pin_cpu0();
  mb::init_corpus();
  mb::print_preamble("micron::string (hstring<char>) per-member benchmark");

  micron::io::println("[hstring: construct]");
  print_header();
  sweep_construct();

  micron::io::println("");
  micron::io::println("[hstring: assign]");
  print_header();
  sweep_assign();

  micron::io::println("");
  micron::io::println("[hstring: access/capacity-query/iters]");
  print_header();
  sweep_access();

  micron::io::println("");
  micron::io::println("[hstring: capacity mutators]");
  print_header();
  sweep_capacity();

  micron::io::println("");
  micron::io::println("[hstring: modifiers]");
  print_header();
  sweep_modifiers();

  micron::io::println("");
  micron::io::println("[hstring: search]");
  print_header();
  sweep_search();

  micron::io::println("");
  micron::io::println("[hstring: remove/remove_all]");
  print_header();
  sweep_remove();

  micron::io::println("");
  micron::io::println("[hstring: comparison]");
  print_header();
  sweep_compare();

  micron::io::println("");
  micron::io::println("[hstring: conversion (stack)]");
  print_header();
  sweep_conv();

  mb::print_epilogue();
  return 0;
}

#pragma GCC pop_options
