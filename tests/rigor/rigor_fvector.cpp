// Exhaustive, adversarial rigor suite for micron::fvector<T, Alloc, Sf>.
//
// fvector is a heap-allocated, growable, MOVE-ONLY container (copy ctor/assign
// deleted) for copyable element types (is_regular_object<T>). It is the "fast"
// vector: it performs NO bounds checks and throws nothing on out-of-range, so
// this suite drives the in-bounds API against the ref_vec oracle
// (tests/support/vector_rigor.hpp) and leans on Tracked balance + ASan +
// instrumented allocators for memory correctness (there are no throw paths to
// assert). clone() MOVES (it empties the source).
//
// Element-type coverage: u8/u16/u32/u64, the 24-byte big, and a non-trivial
// Tracked. Nested fvector<vector<u32>> exercises resource-owning elements.
//
// Build: `duck build tests/rigor/rigor_fvector.cpp`; run `bin/rigor_fvector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/fvector.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/mock_allocators.hpp"
#include "../support/oracles.hpp"
#include "../support/tracked_types.hpp"
#include "../support/vector_rigor.hpp"

using namespace snowball;
using mtest::band;
using mtest::prng;

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 10000
#endif
static constexpr usize ITERS = RIGOR_ITERS;
static constexpr usize CAP = 2048;
static constexpr usize MAXLEN = 256;

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

template<typename A, typename B> inline constexpr bool same_t = false;
template<typename A> inline constexpr bool same_t<A, A> = true;
template<typename E> inline constexpr bool orderable = micron::is_integral_v<E> || same_t<E, mtest::big>;

static void
gen_keys(prng &rng, u64 *ks, usize &n, usize maxlen, band b)
{
  mtest::gen_count(rng, n, maxlen);
  for ( usize i = 0; i < n; ++i ) ks[i] = mtest::gen_raw(rng, b);
}

template<typename E>
static void
from_keys(micron::fvector<E> &v, mtest::ref_vec<CAP> &r, const u64 *ks, usize n)
{
  for ( usize i = 0; i < n; ++i ) {
    E e = mtest::elem<E>::make(ks[i]);
    r.push_back(mtest::elem<E>::key(e));
    v.push_back(micron::move(e));
  }
}

template<typename E>
static void
run_props(void)
{
  using V = micron::fvector<E>;
  static_assert(!micron::is_copy_constructible_v<V>, "fvector must be move-only");

  test_case("fv ctor(n)/fill/generator vs oracle");
  {
    prng rng(0x4101u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_count(rng, n, MAXLEN);
      {
        V v(n);
        mtest::ref_vec<CAP> r;
        r.resize(n);
        ck(mtest::vec_eq<E>(v, r), "ctor-n-zero", it);
      }
      {
        u64 k = mtest::gen_raw(rng, band::small);
        E init = mtest::elem<E>::make(k);
        V v(n, init);
        mtest::ref_vec<CAP> r;
        r.assign(n, mtest::elem<E>::key(init));
        ck(mtest::vec_eq<E>(v, r), "ctor-n-val", it);
      }
      {
        usize ctr = 0;
        auto fn = [&ctr]() { return mtest::elem<E>::make(static_cast<u64>(ctr++)); };
        V v(n, fn);
        mtest::ref_vec<CAP> r;
        for ( usize i = 0; i < n; ++i ) r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(static_cast<u64>(i))));
        ck(mtest::vec_eq<E>(v, r), "ctor-generator", it);
      }
    }
  }
  end_test_case();

  if constexpr ( micron::is_trivially_copyable_v<E> ) {
    test_case("fv initializer_list ctor");
    {
      V v{ mtest::elem<E>::make(3), mtest::elem<E>::make(7), mtest::elem<E>::make(9) };
      mtest::ref_vec<CAP> r;
      r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(3)));
      r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(7)));
      r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(9)));
      ck(mtest::vec_eq<E>(v, r), "init-list", 0);
    }
    end_test_case();
  }

  test_case("fv push/emplace/move_back/pop + access vs oracle");
  {
    prng rng(0x4102u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.push_back(e);
        r.push_back(k);
        ck(mtest::vec_eq<E>(v, r), "push_back-copy", it);
      }
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        mtest::elem<E>::emplace(v, raw);
        r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        ck(mtest::vec_eq<E>(v, r), "emplace_back", it);
      }
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.move_back(micron::move(e));
        r.push_back(k);
        ck(mtest::vec_eq<E>(v, r), "move_back", it);
      }
      if ( !v.empty() ) {
        usize idx = static_cast<usize>(rng.next_in(v.size()));
        ck(mtest::elem<E>::key(v[idx]) == r.buf[idx], "op[]", it);
        ck(mtest::elem<E>::key(v.at(idx)) == r.buf[idx], "at", it);
        ck(mtest::elem<E>::key(v.front()) == r.buf[0], "front", it);
        ck(mtest::elem<E>::key(v.back()) == r.buf[r.len - 1], "back", it);
        const E *d = v.data();
        ck(mtest::elem<E>::key(d[idx]) == r.buf[idx], "data", it);
      }
      usize p = static_cast<usize>(rng.next_in(8));
      for ( usize i = 0; i < p && !v.empty(); ++i ) {
        v.pop_back();
        r.pop_back();
      }
      ck(mtest::vec_eq<E>(v, r), "pop_back", it);
    }
  }
  end_test_case();

  test_case("fv move-ctor/move-assign/clone vs oracle");
  {
    prng rng(0x4103u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::full);
      gen_keys(rng, b, nb, MAXLEN, band::full);
      V va;
      mtest::ref_vec<CAP> ra;
      from_keys<E>(va, ra, a, na);

      V m(micron::move(va));
      ck(mtest::vec_eq<E>(m, ra), "move-ctor", it);
      ck(va.size() == 0u, "move-ctor-donor", it);

      V t;
      mtest::ref_vec<CAP> rt;
      from_keys<E>(t, rt, b, nb);
      t = micron::move(m);
      ck(mtest::vec_eq<E>(t, ra), "move-assign", it);
      ck(m.size() == 0u, "move-assign-donor", it);

      V cl = t.clone();
      ck(mtest::vec_eq<E>(cl, ra), "clone", it);
      ck(t.size() == 0u, "clone-empties-src", it);
    }
  }
  end_test_case();

  test_case("fv reserve/resize grow+shrink vs oracle");
  {
    prng rng(0x4104u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      usize cap = n + static_cast<usize>(rng.next_in(400)) + 1;
      v.reserve(cap);
      ck(mtest::vec_eq<E>(v, r), "reserve-keeps", it);
      ck(v.max_size() >= cap, "reserve-cap", it);
      usize g = n + static_cast<usize>(rng.next_in(200));
      v.resize(g);
      r.resize(g);
      ck(mtest::vec_eq<E>(v, r), "resize-grow", it);
      usize s = static_cast<usize>(rng.next_in(g + 1));
      v.resize(s);
      r.resize(s);
      ck(mtest::vec_eq<E>(v, r), "resize-shrink", it);
    }
  }
  end_test_case();

  test_case("fv iteration + find vs oracle");
  {
    prng rng(0x4105u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      usize idx = 0;
      bool ok = true;
      for ( auto i = v.begin(); i != v.end(); ++i, ++idx )
        if ( mtest::elem<E>::key(*i) != r.buf[idx] ) {
          ok = false;
          break;
        }
      ck(ok && idx == n, "begin/end", it);
      ck(v.cbegin() == v.begin() && v.cend() == v.end(), "cbegin/cend", it);
      u64 fk = mtest::gen_raw(rng, band::small);
      usize oi = r.find(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
      auto p = v.find(mtest::elem<E>::make(fk));
      if ( oi == mtest::VREF_NPOS )
        ck(p == nullptr, "find-miss", it);
      else
        ck(p != nullptr && v.at_n(p) == oi, "find-hit", it);
    }
  }
  end_test_case();

  test_case("fv insert vs oracle");
  {
    prng rng(0x4106u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.insert(idx, e);
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-idx", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        usize cnt = static_cast<usize>(rng.next_in(6)) + 1;
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        v.insert(idx, e, cnt);
        r.insert(idx, mtest::elem<E>::key(e), cnt);
        ck(mtest::vec_eq<E>(v, r), "insert-idx-cnt", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.insert(idx, micron::move(e));
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-idx-move", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.insert(v.begin() + idx, e);
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-it", it);
      }
    }
  }
  end_test_case();

  test_case("fv erase variants vs oracle");
  {
    prng rng(0x4107u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 200, band::full);
      if ( n == 0 ) continue;
      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        v.erase(idx);
        r.erase(idx);
        ck(mtest::vec_eq<E>(v, r), "erase-idx", it);
      }
      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        v.erase(from, to);
        r.erase_range(from, to);
        ck(mtest::vec_eq<E>(v, r), "erase-range", it);
      }
      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        v.erase(v.begin() + idx);
        r.erase(idx);
        ck(mtest::vec_eq<E>(v, r), "erase-it", it);
      }
      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        v.erase(v.begin() + from, v.begin() + to);
        r.erase_range(from, to);
        ck(mtest::vec_eq<E>(v, r), "erase-it-range", it);
      }
    }
  }
  end_test_case();

  test_case("fv remove/assign/append/weld/swap/fill vs oracle");
  {
    prng rng(0x4108u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::small);
      gen_keys(rng, b, nb, MAXLEN, band::small);

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        u64 raw = mtest::gen_raw(rng, band::small);
        v.remove(mtest::elem<E>::make(raw));
        r.remove_val(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        ck(mtest::vec_eq<E>(v, r), "remove", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        usize cnt = static_cast<usize>(rng.next_in(200));
        u64 k = mtest::gen_raw(rng, band::small);
        v.assign(cnt, mtest::elem<E>::make(k));
        r.assign(cnt, mtest::elem<E>::key(mtest::elem<E>::make(k)));
        ck(mtest::vec_eq<E>(v, r), "assign", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> rv;
        from_keys<E>(v, rv, a, na);
        V w;
        mtest::ref_vec<CAP> rw;
        from_keys<E>(w, rw, b, nb);
        v.append(w);
        rv.append(rw.buf, rw.len);
        ck(mtest::vec_eq<E>(v, rv), "append", it);
        ck(mtest::vec_eq<E>(w, rw), "append-src-intact", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> rv;
        from_keys<E>(v, rv, a, na);
        V w;
        mtest::ref_vec<CAP> rw;
        from_keys<E>(w, rw, b, nb);
        v.weld(micron::move(w));
        rv.append(rw.buf, rw.len);
        ck(mtest::vec_eq<E>(v, rv), "weld", it);
        ck(w.size() == 0u, "weld-empties", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> rv;
        from_keys<E>(v, rv, a, na);
        V w;
        mtest::ref_vec<CAP> rw;
        from_keys<E>(w, rw, b, nb);
        v.swap(w);
        ck(mtest::vec_eq<E>(v, rw), "swap-a", it);
        ck(mtest::vec_eq<E>(w, rv), "swap-b", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        u64 k = mtest::gen_raw(rng, band::small);
        v.fill(mtest::elem<E>::make(k));
        r.fill_all(mtest::elem<E>::key(mtest::elem<E>::make(k)));
        ck(mtest::vec_eq<E>(v, r), "fill", it);
      }
    }
  }
  end_test_case();

  if constexpr ( orderable<E> ) {
    test_case("fv sort/insert_sort vs oracle");
    {
      prng rng(0x4109u);
      u64 ks[MAXLEN + 4];
      for ( usize it = 0; it < ITERS; ++it ) {
        usize n;
        gen_keys(rng, ks, n, 200, band::full);
        {
          V v;
          mtest::ref_vec<CAP> r;
          from_keys<E>(v, r, ks, n);
          v.sort();
          r.sort();
          ck(mtest::vec_eq<E>(v, r), "sort", it);
        }
        {
          V v;
          mtest::ref_vec<CAP> r;
          for ( usize i = 0; i < n; ++i ) {
            E e = mtest::elem<E>::make(ks[i]);
            r.push_back(mtest::elem<E>::key(e));
            v.insert_sort(micron::move(e));
          }
          r.sort();
          ck(mtest::vec_eq<E>(v, r), "insert_sort", it);
        }
      }
    }
    end_test_case();
  }
}

static u64
rowkey(const micron::vector<u32> &r)
{
  u64 h = 1469598103934665603ull;
  h = (h ^ static_cast<u64>(r.size())) * 1099511628211ull;
  for ( usize i = 0; i < r.size(); ++i ) h = (h ^ static_cast<u64>(r[i])) * 1099511628211ull;
  return h;
}

static micron::vector<u32>
mkrow(prng &rng)
{
  micron::vector<u32> r;
  usize len = static_cast<usize>(rng.next_in(8));
  u32 base = static_cast<u32>(rng.next());
  for ( usize i = 0; i < len; ++i ) r.push_back(base + static_cast<u32>(i) * 2654435761u);
  return r;
}

template<typename VV>
static bool
nested_eq(const VV &vv, const mtest::ref_vec<CAP> &r)
{
  if ( static_cast<usize>(vv.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( rowkey(vv[i]) != r.buf[i] ) return false;
  return true;
}

static void
run_nested(void)
{
  using V = micron::vector<u32>;
  using FVV = micron::fvector<V>;

  test_case("fvector<vector<u32>> nested ops vs oracle");
  {
    prng rng(0xAABBu);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(40));
      FVV vv;
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        r.push_back(rowkey(row));
        vv.move_back(micron::move(row));
      }
      ck(nested_eq(vv, r), "build", it);
      if ( n ) {
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        V row = mkrow(rng);
        u64 k = rowkey(row);
        vv.insert(idx, micron::move(row));
        r.insert(idx, k);
        ck(nested_eq(vv, r), "insert", it);
      }
      if ( vv.size() ) {
        usize idx = static_cast<usize>(rng.next_in(vv.size()));
        vv.erase(idx);
        r.erase(idx);
        ck(nested_eq(vv, r), "erase", it);
      }
      {
        usize s = vv.size() ? static_cast<usize>(rng.next_in(vv.size() + 1)) : 0;
        vv.resize(s);
        r.resize(s);
        ck(nested_eq(vv, r), "resize-shrink", it);
      }
    }
  }
  end_test_case();
}

static void
run_memory(void)
{
  using Tr = mtest::Tracked<9>;
  using TA = mtest::tracking_allocator<9>;
  using VA = micron::fvector<Tr, TA>;

  test_case("fv churn leak-free + Tracked balanced");
  {
    Tr::reset();
    expect_leak_free<TA>([] {
      prng rng(0xF00D01u);
      VA v;
      for ( usize it = 0; it < 20000; ++it ) {
        u64 op = rng.next() % 6;
        if ( op == 0 )
          v.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
        else if ( op == 1 )
          v.emplace_back(static_cast<int>(rng.next() & 0xffff));
        else if ( op == 2 ) {
          if ( !v.empty() ) v.pop_back();
        } else if ( op == 3 ) {
          if ( !v.empty() ) v.erase(static_cast<usize>(rng.next() % v.size()));
        } else if ( op == 4 ) {
          if ( !v.empty() ) v.insert(static_cast<usize>(rng.next() % (v.size() + 1)), Tr(static_cast<int>(rng.next() & 0xffff)));
        } else {
          if ( rng.next() & 1u )
            v.clear();
          else
            v.resize(static_cast<usize>(rng.next() % 64));
        }
      }
    });
    ck(Tr::live() == 0u, "tracked-balance", 0);
  }
  end_test_case();

  test_case("fv move-assign frees dest + empties donor (no leak)");
  {
    TA::reset();
    Tr::reset();
    {
      VA a;
      for ( int i = 0; i < 60; ++i ) a.push_back(Tr(i));
      VA b;
      for ( int i = 0; i < 60; ++i ) b.push_back(Tr(i + 1000));
      a = micron::move(b);
      ck(b.size() == 0u, "donor-empty", 0);
      ck(a.size() == 60u, "size", 0);
    }
    ck(TA::outstanding() == 0, "no-leak", 0);
    ck(Tr::live() == 0u, "balanced", 0);
  }
  end_test_case();

  test_case("fv throwing allocator mid-grow: throws + no leak");
  {
    using THA = mtest::throwing_allocator<10>;
    using VT = micron::fvector<u32, THA>;
    THA::reset();
    bool threw = false;
    try {
      VT v;
      THA::arm(2);
      for ( u32 i = 0; i < 100000u; ++i ) v.push_back(i);
    } catch ( const micron::except::memory_error & ) {
      threw = true;
    }
    THA::disarm();
    ck(threw, "grow-threw", 0);
    ck(THA::outstanding() == 0, "grow-no-leak", 0);
  }
  end_test_case();
}

template<typename E>
static void
run_all(const char *tyname)
{
  sb::print("--- fvector<", tyname, "> ---");
  run_props<E>();
}

int
main(int, char **)
{
  sb::print("=== FVECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- memory / lifetime / exception (Tracked + allocators) ---");
  run_memory();

  sb::print("--- nested containers (fvector<vector<...>>) ---");
  run_nested();

  sb::print("=== FVECTOR RIGOR DONE ===");
  return 1;
}
