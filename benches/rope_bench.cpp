// Per-member benchmark of micron::rope<char>.
//
// Rope is a persistent balanced binary tree of contiguous leaves (<=256
// bytes each). Copies are O(1) (refcount bump). Mutating ops return a new
// rope built by path-copy + leaf-merge, so each cell here includes one
// alloc + many memcpy + destructors of unreferenced nodes.
//
// The bench tags variants in the "impl" column:
//   - "tree"/"flat"  : measures the same op cold (tree-walk) vs after
//                      __ensure_flat() has populated the contiguous cache.
//   - "share"/"copy" : copies that bump refcount vs ones that allocate a
//                      distinct tree.
//   - "early"/"late"/"miss" : as for the other benches.
//
// Anomaly candidates the report should rank:
//   - find(ch)  — per-byte leaf-walk inside the for_each_chunk lambda
//   - find_substr — scalar j-walk after __ensure_flat
//   - __cmp_raw / operator==(rope,rope) — iterator-based per-byte loop
//   - const_iterator::operator++ — per-byte advance (no batched skip)
//
// Sizes swept: 64/256/1024/4096/16384 (large enough to actually traverse
// multiple leaves at 256-byte max-leaf).

#include "_string_bench_common.hpp"

#include "../src/string/rope.hpp"
#include "../src/string/sstring.hpp"
#include "../src/string/string.hpp"

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

using RP = micron::rope<char>;
using HS = micron::string;
using SS = micron::sstring<2048, char>;

constexpr u64 SIZES[] = { 64, 256, 1024, 4096, 16384 };

// rope mutators allocate (path-copy, new leaves, etc.). Cap reps so cells
// finish quickly; the differential is what matters, not absolute throughput.
constexpr u64 RP_MAX_REPS = 1ULL << 12;

template<typename Fn>
inline mb::row
bench_one(const char *op, const char *impl, u64 size, u64 bytes_per_op, Fn &&fn)
{
  return mb::bench_one(op, impl, size, bytes_per_op, micron::forward<Fn>(fn), RP_MAX_REPS);
}

void
fill_mixed(char *dst, u64 sz)
{
  for ( u64 i = 0; i < sz; ++i ) dst[i] = g_mixed_ascii[i];
  dst[sz] = 0;
}

// rope(const char *) takes a value-typed const char* — no decay surprise
// like istring's, but for consistency we route through a helper.
[[gnu::always_inline]] inline const char *
as_cstr(const char *p) noexcept
{
  return p;
}

// ----- construction -----

void
sweep_construct()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    fill_mixed(tmp, sz);

    print_row(bench_one("construct(default)", "default", sz, 16, [&]() {
      RP r;
      clobber(r.data());
    }));
    print_row(bench_one("construct(usize n)", "noop", sz, 16, [&]() {
      RP r(sz);
      clobber(r.data());
    }));
    print_row(bench_one("construct(n, ch)", "fill", sz, sz, [&]() {
      RP r(sz, 'x');
      clobber(r.data());
    }));
    print_row(bench_one("construct(C-str)", "balanced", sz, sz, [&]() {
      RP r(as_cstr(tmp));
      clobber(r.data());
    }));
    RP src(as_cstr(tmp));
    print_row(bench_one("construct(copy)", "share/O(1)", sz, 16, [&]() {
      RP r(src);
      clobber(r.data());
    }));
    print_row(bench_one("construct(move)", "move", sz, sz, [&]() {
      RP donor(as_cstr(tmp));
      RP r(micron::move(donor));
      clobber(r.data());
    }));
    print_row(bench_one("construct(iter,iter)", "balanced", sz, sz, [&]() {
      RP r(tmp, tmp + sz);
      clobber(r.data());
    }));
    HS hs(as_cstr(tmp));
    print_row(bench_one("construct(is_string)", "balanced", sz, sz, [&]() {
      RP r(hs);
      clobber(r.data());
    }));
  }
}

// ----- assignment -----

void
sweep_assign()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    fill_mixed(tmp, sz);
    RP src(as_cstr(tmp));

    print_row(bench_one("operator=(copy)", "share", sz, 16, [&]() {
      RP dst;
      dst = src;
      clobber(dst.data());
    }));
    print_row(bench_one("operator=(move)", "move", sz, sz, [&]() {
      RP dst;
      RP donor(as_cstr(tmp));
      dst = micron::move(donor);
      clobber(dst.data());
    }));
  }
}

// ----- element access / capacity / iters -----

void
sweep_access()
{
  constexpr u64 sz = 1024;
  char tmp[1025];
  fill_mixed(tmp, sz);
  RP r(as_cstr(tmp));

  print_row(bench_one("operator[]", "tree-walk", sz, 1, [&]() { sink_char(r[sz / 2]); }));
  print_row(bench_one("at(idx)", "tree-walk", sz, 1, [&]() { sink_char(r.at(sz / 2)); }));
  print_row(bench_one("front()", "tree-walk", sz, 1, [&]() { sink_char(r.front()); }));
  print_row(bench_one("back()", "tree-walk", sz, 1, [&]() { sink_char(r.back()); }));

  // First call to data() flattens. Bench separately:
  //  - "tree" : fresh rope each call (forces flatten every time)
  //  - "flat" : warm cache, just returns __flat ptr
  print_row(bench_one("data()", "tree(no-cache)", sz, sz, [&]() {
    RP rr(as_cstr(tmp));
    sink_ptr(rr.data());
  }));
  print_row(bench_one("data()", "flat(cache-hit)", sz, 1, [&]() { sink_ptr(r.data()); }));
  print_row(bench_one("c_str()", "flat(cache-hit)", sz, 1, [&]() { sink_ptr(r.c_str()); }));

  print_row(bench_one("size()", "default", sz, 1, [&]() { sink_size(r.size()); }));
  print_row(bench_one("len()", "default", sz, 1, [&]() { sink_size(r.len()); }));
  print_row(bench_one("capacity()", "default", sz, 1, [&]() { sink_size(r.capacity()); }));
  print_row(bench_one("max_size()", "default", sz, 1, [&]() { sink_size(r.max_size()); }));
  print_row(bench_one("empty()", "default", sz, 1, [&]() { sink_bool(r.empty()); }));
  print_row(bench_one("identity()", "default", sz, 1, [&]() { sink_ptr(r.identity()); }));
  print_row(bench_one("operator bool()", "default", sz, 1, [&]() { sink_bool(static_cast<bool>(r)); }));
  print_row(bench_one("operator!()", "default", sz, 1, [&]() { sink_bool(!r); }));
  print_row(bench_one("into_bytes()", "default", sz, 1, [&]() {
    auto sl = r.into_bytes();
    sink_ptr(sl.begin());
  }));
  print_row(bench_one("begin()", "default", sz, 1, [&]() {
    auto it = r.begin();
    sink_size(it.index());
  }));
  print_row(bench_one("end()", "default", sz, 1, [&]() {
    auto it = r.end();
    sink_size(it.index());
  }));
  print_row(bench_one("clone()", "share", sz, 16, [&]() {
    RP c = r.clone();
    clobber(c.data());
  }));
}

// ----- iterator stepping -----

void
sweep_iter()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    fill_mixed(tmp, sz);
    RP r(as_cstr(tmp));

    print_row(bench_one("iterator++", "sweep-all", sz, sz, [&]() {
      auto it = r.begin();
      auto e = r.end();
      usize count = 0;
      for ( ; it != e; ++it ) ++count;
      sink_size(count);
    }));
    print_row(bench_one("for_each_chunk", "sweep-all", sz, sz, [&]() {
      usize tot = 0;
      r.for_each_chunk([&](const char *, usize l) {
        tot += l;
        return true;
      });
      sink_size(tot);
    }));
    print_row(bench_one("for_each", "sweep-all", sz, sz, [&]() {
      usize tot = 0;
      r.for_each([&](char) { ++tot; });
      sink_size(tot);
    }));
  }
}

// ----- modifiers (each returns a fresh rope) -----

void
sweep_modifiers()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    fill_mixed(tmp, sz);
    const char *p = as_cstr(tmp);

    print_row(bench_one("push_back(ch)", "spine-copy", sz, 1, [&]() {
      RP r(p);
      auto t = r.push_back('x');
      clobber(t.data());
    }));
    {
      RP rhs(p);
      print_row(bench_one("push_back(rope)", "share", sz, 16, [&]() {
        RP r(p);
        auto t = r.push_back(rhs);
        clobber(t.data());
      }));
    }
    print_row(bench_one("pop_back", "split", sz, sz, [&]() {
      RP r(p);
      auto t = r.pop_back();
      clobber(t.data());
    }));
    print_row(bench_one("append(arr)", "8B", sz, 8, [&]() {
      RP r(p);
      auto t = r.append("abcdefgh");
      clobber(t.data());
    }));
    {
      RP rhs(p);
      print_row(bench_one("append(rope)", "share", sz, 16, [&]() {
        RP r(p);
        auto t = r.append(rhs);
        clobber(t.data());
      }));
    }
    print_row(bench_one("insert(idx,ch,n)", "n=1 split", sz, sz, [&]() {
      RP r(p);
      auto t = r.insert(sz / 2, 'Q', 1);
      clobber(t.data());
    }));
    print_row(bench_one("insert(idx,arr)", "8B split", sz, sz, [&]() {
      RP r(p);
      auto t = r.insert(sz / 2, "abcdefgh", 1);
      clobber(t.data());
    }));
    print_row(bench_one("erase(idx,n)", "n=1 split", sz, sz, [&]() {
      RP r(p);
      auto t = r.erase(sz / 2, 1);
      clobber(t.data());
    }));
    print_row(bench_one("substr(pos,cnt)", "half split", sz, sz, [&]() {
      RP r(p);
      auto t = r.substr(0, sz / 2);
      clobber(t.data());
    }));
    print_row(bench_one("truncate(idx)", "half split", sz, sz, [&]() {
      RP r(p);
      auto t = r.truncate(static_cast<usize>(sz / 2));
      clobber(t.data());
    }));
    print_row(bench_one("resize(grow,ch)", "+sz fill", sz, sz, [&]() {
      RP r(p);
      auto t = r.resize(sz * 2, 'x');
      clobber(t.data());
    }));
    print_row(bench_one("resize(shrink,ch)", "half trunc", sz, sz, [&]() {
      RP r(p);
      auto t = r.resize(sz / 2, 'x');
      clobber(t.data());
    }));
    print_row(bench_one("flatten()", "rebuild", sz, sz, [&]() {
      RP r(p);
      auto t = r.flatten();
      clobber(t.data());
    }));
    print_row(bench_one("operator+=(ch)", "spine-copy", sz, 1, [&]() {
      RP r(p);
      r += 'x';
      clobber(r.data());
    }));
    print_row(bench_one("operator+=(C-str)", "8B", sz, 8, [&]() {
      RP r(p);
      const char *cs = "abcdefgh";
      r += cs;
      clobber(r.data());
    }));
  }
}

// ----- search -----

void
sweep_search()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    fill_mixed(tmp, sz);
    RP r(as_cstr(tmp));

    if ( sz >= 16 ) {
      char ch_early = tmp[8];
      print_row(bench_one("find(ch)", "early leaf-walk", sz, sz, [&]() { sink_size(r.find(ch_early)); }));
      char ch_late = tmp[sz - 4];
      char tmp2[16385];
      for ( u64 i = 0; i < sz; ++i ) tmp2[i] = (tmp[i] == ch_late && i < sz - 4) ? '!' : tmp[i];
      tmp2[sz - 4] = ch_late;
      tmp2[sz] = 0;
      RP r2(as_cstr(tmp2));
      print_row(bench_one("find(ch)", "late leaf-walk", sz, sz, [&]() { sink_size(r2.find(ch_late)); }));
      print_row(bench_one("find(ch)", "miss leaf-walk", sz, sz, [&]() { sink_size(r.find('@')); }));
    }
    if ( sz >= 64 ) {
      const char *needle8 = "ZyXwVuTs";
      char tmp3[16385];
      for ( u64 i = 0; i < sz; ++i ) tmp3[i] = tmp[i];
      for ( u64 i = 0; i < 8; ++i ) tmp3[sz - 16 + i] = needle8[i];
      tmp3[sz] = 0;
      RP r3(as_cstr(tmp3));
      // Pre-flatten r3 to amortize the cache fill (flat warm-up).
      sink_ptr(r3.data());
      print_row(bench_one("find_substr(8B)", "late scalar-j", sz, sz, [&]() { sink_size(r3.find_substr(needle8, 8)); }));
      sink_ptr(r.data());
      print_row(bench_one("find_substr(8B)", "miss scalar-j", sz, sz, [&]() { sink_size(r.find_substr(needle8, 8)); }));
    }
  }
}

// ----- comparison -----

void
sweep_compare()
{
  for ( u64 sz : SIZES ) {
    char tmp[16385];
    char tmp2[16385];
    fill_mixed(tmp, sz);
    fill_mixed(tmp2, sz);
    if ( sz > 0 ) tmp2[sz - 1] ^= 1;
    const char *p = as_cstr(tmp);
    const char *q = as_cstr(tmp2);
    RP a(p), b(p), c(q);

    print_row(bench_one("operator==(rope,share)", "share-fast", sz, sz, [&]() { sink_bool(a == b); }));
    print_row(bench_one("operator==(rope,distinct)", "iter-walk", sz, sz, [&]() {
      RP a2(p);
      sink_bool(a2 == b);
    }));
    print_row(bench_one("operator==(rope,diff)", "iter-walk diff", sz, sz, [&]() { sink_bool(a == c); }));
    print_row(bench_one("operator==(C-str,eq)", "__cmp_raw", sz, sz, [&]() { sink_bool(a == p); }));
    print_row(bench_one("operator==(C-str,diff)", "__cmp_raw diff", sz, sz, [&]() { sink_bool(a == q); }));
    print_row(bench_one("operator<(C-str)", "__cmp_raw", sz, sz, [&]() { sink_bool(a < p); }));
  }
}

// ----- remove -----

void
sweep_remove()
{
  for ( u64 sz : { (u64)256, (u64)1024, (u64)4096 } ) {
    char tmp[16385];
    fill_mixed(tmp, sz);
    const char *p = as_cstr(tmp);

    print_row(bench_one("remove(C-str)", "1m", sz, sz, [&]() {
      RP r(p);
      auto t = r.remove("Ab");
      clobber(t.data());
    }));
    print_row(bench_one("remove_all(C-str)", "scalar-loop", sz, sz, [&]() {
      RP r(p);
      auto t = r.remove_all("Ab");
      clobber(t.data());
    }));
  }
}

}      // namespace

int
main(void)
{
  mb::pin_cpu0();
  mb::init_corpus();
  mb::print_preamble("micron::rope<char> per-member benchmark");

  micron::io::println("[rope: construct]");
  print_header();
  sweep_construct();

  micron::io::println("");
  micron::io::println("[rope: assign]");
  print_header();
  sweep_assign();

  micron::io::println("");
  micron::io::println("[rope: access/iters/capacity]");
  print_header();
  sweep_access();

  micron::io::println("");
  micron::io::println("[rope: iterator traversal]");
  print_header();
  sweep_iter();

  micron::io::println("");
  micron::io::println("[rope: modifiers (each returns new rope)]");
  print_header();
  sweep_modifiers();

  micron::io::println("");
  micron::io::println("[rope: search]");
  print_header();
  sweep_search();

  micron::io::println("");
  micron::io::println("[rope: comparison]");
  print_header();
  sweep_compare();

  micron::io::println("");
  micron::io::println("[rope: remove/remove_all]");
  print_header();
  sweep_remove();

  mb::print_epilogue();
  return 0;
}

#pragma GCC pop_options
