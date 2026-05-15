//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../src/memory/allocation/abcmalloc/__abc.hpp"
#include "../src/memory/allocation/abcmalloc/config.hpp"
#include "../src/memory/allocation/abcmalloc/hooks.hpp"

namespace
{

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) {}

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

enum class formula_kind : u8 { cache, small_f, medium_f, large_f, huge_f, bulk_f };

struct cell {
  u64 sz;
  u64 sheet_bytes;
  u64 pages;
  u64 blocks;
};

static u64
invoke_formula(formula_kind k, u64 sz) noexcept
{
  switch ( k ) {
  case formula_kind::cache :
    return abc::__calculate_space_cache(abc::__default_cache_step);
  case formula_kind::small_f :
    return abc::__calculate_space_small(sz);
  case formula_kind::medium_f :
    return abc::__calculate_space_medium(sz);
  case formula_kind::large_f :
    return abc::__calculate_space_large(sz);
  case formula_kind::huge_f :
    return abc::__calculate_space_huge(sz);
  case formula_kind::bulk_f :
    return abc::__calculate_space_bulk(sz);
  }
  return 0;
}

static const char *
formula_label(formula_kind k) noexcept
{
  switch ( k ) {
  case formula_kind::cache :
    return "cache(768)";
  case formula_kind::small_f :
    return "small";
  case formula_kind::medium_f :
    return "medium";
  case formula_kind::large_f :
    return "large";
  case formula_kind::huge_f :
    return "huge";
  case formula_kind::bulk_f :
    return "bulk";
  }
  return "?";
}

static cell
eval_cell(formula_kind k, u64 sz, u64 class_size) noexcept
{
  cell c;
  c.sz = sz;
  c.sheet_bytes = invoke_formula(k, sz);
  c.pages = c.sheet_bytes / abc::__system_pagesize;
  c.blocks = class_size > 0 ? c.sheet_bytes / class_size : 0;
  return c;
}

struct sz_str {
  char buf[24];
};

static sz_str
sz_pretty(u64 b) noexcept
{
  sz_str out{};
  u32 p = 0;
  char unit = 0;
  u64 v = b;
  if ( b == 0 ) {
    out.buf[p++] = '0';
    out.buf[p] = '\0';
    return out;
  }
  if ( (b & ((1ULL << 30) - 1)) == 0 ) {
    v = b >> 30;
    unit = 'G';
  } else if ( (b & ((1ULL << 20) - 1)) == 0 ) {
    v = b >> 20;
    unit = 'M';
  } else if ( (b & ((1ULL << 10) - 1)) == 0 ) {
    v = b >> 10;
    unit = 'K';
  }

  char tmp[24];
  u32 n = 0;
  if ( v == 0 )
    tmp[n++] = '0';
  else
    while ( v ) {
      tmp[n++] = '0' + (v % 10);
      v /= 10;
    }
  while ( n ) out.buf[p++] = tmp[--n];
  if ( unit ) out.buf[p++] = unit;
  out.buf[p] = '\0';
  return out;
}

static void
print_header()
{
  line h;
  h.s_lj_at("tier", 10);
  h.s_at("class", 20);
  h.s_at("input", 32);
  h.pad_to(34, 0);
  h.s_lj_at("formula", 44);
  h.s_at("sheet", 58);
  h.s_at("pages", 68);
  h.s_at("blocks", 82);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------------");
}

static void
print_row(const char *tier, u64 class_sz, u64 input_sz, formula_kind k, const cell &c)
{
  sz_str cls = sz_pretty(class_sz);
  sz_str inp = sz_pretty(input_sz);
  sz_str sht = sz_pretty(c.sheet_bytes);
  line ln;
  ln.s_lj_at(tier, 10);
  ln.s_at(cls.buf, 20);
  ln.s_at(inp.buf, 32);
  ln.pad_to(34, 0);
  ln.s_lj_at(formula_label(k), 44);
  ln.s_at(sht.buf, 58);
  ln.u_at(c.pages, 68);
  ln.u_at(c.blocks, 82);
  micron::io::println(ln.str());
}

struct row_spec {
  const char *tier;
  u64 class_size;
  u64 inputs[3];
  formula_kind formulas[3];
};

constexpr row_spec kSpecs[] = {
  { "precise", abc::__class_precise, { 1u, 256u, 512u }, { formula_kind::cache, formula_kind::cache, formula_kind::cache } },
  { "small", abc::__class_small, { 513u, 2048u, 4095u }, { formula_kind::small_f, formula_kind::small_f, formula_kind::small_f } },
  { "medium", abc::__class_medium, { 4096u, 16384u, 32768u }, { formula_kind::medium_f, formula_kind::medium_f, formula_kind::medium_f } },
  { "large", abc::__class_large, { 32769u, 131072u, 262144u }, { formula_kind::large_f, formula_kind::large_f, formula_kind::large_f } },
  { "huge", abc::__class_huge, { 262145u, 524288u, 1048575u }, { formula_kind::huge_f, formula_kind::huge_f, formula_kind::huge_f } },
  { "huge", abc::__class_huge, { 1048576u, (1u << 24), (1u << 28) }, { formula_kind::huge_f, formula_kind::huge_f, formula_kind::huge_f } },
  { "huge",
    abc::__class_huge,
    { (1ULL << 30), (1ULL << 32), (1ULL << 33) },
    { formula_kind::bulk_f, formula_kind::bulk_f, formula_kind::bulk_f } },
};

}     // namespace

int
main(int, char **)
{
  micron::io::println("abcmalloc sheet-expansion sizing (per-tier __calculate_space_*)");
  micron::io::println("");
  print_header();

  for ( const row_spec &spec : kSpecs ) {
    for ( u32 i = 0; i < 3; ++i ) {
      cell c = eval_cell(spec.formulas[i], spec.inputs[i], spec.class_size);
      print_row(spec.tier, spec.class_size, spec.inputs[i], spec.formulas[i], c);
    }
  }

  micron::io::println("");
  micron::io::println("note: blocks = sheet / class_size; actual buddy/TLSF usable count is slightly lower.");
  return 0;
}
