// Exhaustive, adversarial rigor suite for micron::convector<T, Alloc, Sf>.
//
// convector is a heap-allocated, growable, mutable vector guarded by an internal
// mutex (thread-safe). Single-threaded, it behaves like micron::vector, so this
// suite diffs every member function against the ref_vec oracle
// (tests/support/vector_rigor.hpp); it then adds a MULTITHREADED stress section
// (micron::auto_thread, per MEMORY.md feedback_no_stl_libc_in_tests — no
// <thread>/<atomic>) where several threads push concurrently and the produced
// multiset is verified order-independently. Out-of-range access throws
// library_error. Memory: Tracked balance + tracking/throwing allocators +
// nested convector<vector<u32>> + ASan/TSan.
//
// Build: `duck build tests/rigor/rigor_convector.cpp`; run `bin/rigor_convector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/convector.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

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

static constexpr usize ITERS = (RIGOR_ITERS > 2500) ? 2500 : RIGOR_ITERS;
static constexpr usize CAP = 2048;
static constexpr usize MAXLEN = 64;

template<typename A, typename B> inline constexpr bool same_t = false;
template<typename A> inline constexpr bool same_t<A, A> = true;
template<typename E> inline constexpr bool orderable = micron::is_integral_v<E> || same_t<E, mtest::big>;

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

static void
gen_keys(prng &rng, u64 *ks, usize &n, usize maxlen, band b)
{
  mtest::gen_count(rng, n, maxlen);
  for ( usize i = 0; i < n; ++i ) ks[i] = mtest::gen_raw(rng, b);
}

template<typename E>
static void
from_keys(micron::convector<E> &v, mtest::ref_vec<CAP> &r, const u64 *ks, usize n)
{
  for ( usize i = 0; i < n; ++i ) {
    E e = mtest::elem<E>::make(ks[i]);
    r.push_back(mtest::elem<E>::key(e));
    v.push_back(micron::move(e));
  }
}

template<typename E>
static bool
veq(micron::convector<E> &v, const mtest::ref_vec<CAP> &r)
{
  if ( static_cast<usize>(v.size()) != r.len ) return false;
  const E *d = v.data();
  for ( usize i = 0; i < r.len; ++i )
    if ( mtest::elem<E>::key(d[i]) != r.buf[i] ) return false;
  return true;
}

template<typename E>
static void
run_props(void)
{
  using V = micron::convector<E>;

  test_case("cv ctor/push/emplace/move_back/pop + access vs oracle");
  {
    prng rng(0x7101u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);

      {
        V z(n);
        mtest::ref_vec<CAP> rz;
        rz.resize(n);
        ck(veq<E>(z, rz), "ctor-n-zero", it);
        u64 k = mtest::gen_raw(rng, band::small);
        V f(n, mtest::elem<E>::make(k));
        mtest::ref_vec<CAP> rf;
        rf.assign(n, mtest::elem<E>::key(mtest::elem<E>::make(k)));
        ck(veq<E>(f, rf), "ctor-n-val", it);
      }
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.push_back(e);
        r.push_back(k);
        ck(veq<E>(v, r), "push_back-copy", it);
      }
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        mtest::elem<E>::emplace(v, raw);
        r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        ck(veq<E>(v, r), "emplace_back", it);
      }
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.move_back(micron::move(e));
        r.push_back(k);
        ck(veq<E>(v, r), "move_back", it);
      }
      if ( !v.empty() ) {
        usize idx = static_cast<usize>(rng.next_in(v.size()));
        ck(mtest::elem<E>::key(v[idx]) == r.buf[idx], "op[]", it);
        ck(mtest::elem<E>::key(v.at(idx)) == r.buf[idx], "at", it);
        ck(mtest::elem<E>::key(v.front()) == r.buf[0], "front", it);
        ck(mtest::elem<E>::key(v.back()) == r.buf[r.len - 1], "back", it);
        expect_throw_type<micron::except::library_error>([&] { (void)v.at(v.size()); });
      }
      usize p = static_cast<usize>(rng.next_in(8));
      for ( usize i = 0; i < p && !v.empty(); ++i ) {
        v.pop_back();
        r.pop_back();
      }
      ck(veq<E>(v, r), "pop_back", it);
    }
  }
  end_test_case();

  test_case("cv copy/move ctor+assign + independence");
  {
    prng rng(0x7102u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::full);
      gen_keys(rng, b, nb, MAXLEN, band::full);
      V va;
      mtest::ref_vec<CAP> ra;
      from_keys<E>(va, ra, a, na);
      V c(va);
      ck(veq<E>(c, ra), "copy-ctor", it);
      if ( na ) {
        c[0] = mtest::elem<E>::make(~ra.buf[0]);
        ck(mtest::elem<E>::key(va[0]) == ra.buf[0], "copy-indep", it);
      }
      V fresh(va);
      V m(micron::move(fresh));
      ck(veq<E>(m, ra), "move-ctor", it);
      ck(fresh.size() == 0u, "move-ctor-donor", it);
      V vb;
      mtest::ref_vec<CAP> rb;
      from_keys<E>(vb, rb, b, nb);
      vb = va;
      ck(veq<E>(vb, ra), "copy-assign", it);
      V vc;
      mtest::ref_vec<CAP> rc;
      from_keys<E>(vc, rc, a, na);
      V dst;
      dst = micron::move(vc);
      ck(veq<E>(dst, ra), "move-assign", it);
      ck(vc.size() == 0u, "move-assign-donor", it);
    }
  }
  end_test_case();

  test_case("cv capacity/resize + iteration + find vs oracle");
  {
    prng rng(0x7103u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      usize cap = n + static_cast<usize>(rng.next_in(400)) + 1;
      v.reserve(cap);
      ck(veq<E>(v, r), "reserve-keeps", it);
      ck(v.max_size() >= cap, "reserve-cap", it);
      usize g = n + static_cast<usize>(rng.next_in(200));
      v.resize(g);
      r.resize(g);
      ck(veq<E>(v, r), "resize-grow", it);
      usize s = static_cast<usize>(rng.next_in(g + 1));
      v.resize(s);
      r.resize(s);
      ck(veq<E>(v, r), "resize-shrink", it);

      usize j = 0;
      bool ok = true;
      for ( auto i = v.begin(); i != v.end(); ++i, ++j )
        if ( mtest::elem<E>::key(*i) != r.buf[j] ) {
          ok = false;
          break;
        }
      ck(ok && j == v.size(), "begin/end", it);

      {
        V fv;
        mtest::ref_vec<CAP> fr;
        from_keys<E>(fv, fr, ks, n);
        u64 fk = mtest::gen_raw(rng, band::small);
        usize oi = fr.find(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
        auto pp = fv.find(mtest::elem<E>::make(fk));
        if ( oi == mtest::VREF_NPOS )
          ck(pp == nullptr, "find-miss", it);
        else
          ck(pp != nullptr && fv.at_n(pp) == oi, "find-hit", it);
      }
    }
  }
  end_test_case();

  test_case("cv insert/erase vs oracle");
  {
    prng rng(0x7104u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      if ( n == 0 ) continue;

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        v.insert(idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(veq<E>(v, r), "insert-idx", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        v.insert(v.begin() + idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(veq<E>(v, r), "insert-it", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        v.erase(idx);
        r.erase(idx);
        ck(veq<E>(v, r), "erase-idx", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        v.erase(from, to);
        r.erase_range(from, to);
        ck(veq<E>(v, r), "erase-range", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        v.erase(v.begin() + from, v.begin() + to);
        r.erase_range(from, to);
        ck(veq<E>(v, r), "erase-it-range", it);
      }
    }
  }
  end_test_case();

  test_case("cv assign/append/weld/swap/clone/fill vs oracle");
  {
    prng rng(0x7105u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::small);
      gen_keys(rng, b, nb, MAXLEN, band::small);
      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        usize cnt = static_cast<usize>(rng.next_in(200));
        u64 k = mtest::gen_raw(rng, band::small);
        v.assign(cnt, mtest::elem<E>::make(k));
        r.assign(cnt, mtest::elem<E>::key(mtest::elem<E>::make(k)));
        ck(veq<E>(v, r), "assign", it);
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
        ck(veq<E>(v, rv), "append", it);
        ck(veq<E>(w, rw), "append-src", it);
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
        ck(veq<E>(v, rv), "weld", it);
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
        ck(veq<E>(v, rw), "swap-a", it);
        ck(veq<E>(w, rv), "swap-b", it);
      }
      {
        V v;
        mtest::ref_vec<CAP> rv;
        from_keys<E>(v, rv, a, na);
        V cl = v.clone();
        ck(veq<E>(cl, rv), "clone", it);
        u64 fk = mtest::gen_raw(rng, band::small);
        v.fill(mtest::elem<E>::make(fk));
        rv.fill_all(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
        ck(veq<E>(v, rv), "fill", it);
      }
    }
  }
  end_test_case();

  if constexpr ( orderable<E> ) {
    test_case("cv sort vs oracle");
    {
      prng rng(0x7106u);
      u64 ks[MAXLEN + 4];
      for ( usize it = 0; it < ITERS; ++it ) {
        usize n;
        gen_keys(rng, ks, n, MAXLEN, band::full);
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        v.sort();
        r.sort();
        ck(veq<E>(v, r), "sort", it);
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
  using CVV = micron::convector<V>;

  test_case("convector<vector<u32>> nested ops vs oracle");
  {
    prng rng(0xBE11u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(40));
      CVV vv;
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        r.push_back(rowkey(row));
        vv.move_back(micron::move(row));
      }
      ck(nested_eq(vv, r), "build", it);
      {
        CVV cp(vv);
        ck(nested_eq(cp, r), "copy", it);
        if ( n ) {
          cp[0].push_back(5u);
          ck(rowkey(vv[0]) == r.buf[0], "copy-indep", it);
        }
      }
      if ( n ) {
        usize idx = static_cast<usize>(rng.next_in(n));
        vv.erase(idx);
        r.erase(idx);
        ck(nested_eq(vv, r), "erase", it);
      }
      {
        usize s = vv.size() ? static_cast<usize>(rng.next_in(vv.size() + 1)) : 0;
        vv.resize(s);
        r.resize(s);
        ck(nested_eq(vv, r), "resize", it);
      }
    }
  }
  end_test_case();
}

static void
run_memory(void)
{
  using Tr = mtest::Tracked<13>;
  using TA = mtest::tracking_allocator<13>;
  using VA = micron::convector<Tr, TA>;

  test_case("cv churn leak-free + Tracked balanced");
  {
    Tr::reset();
    expect_leak_free<TA>([] {
      prng rng(0xC0DE01u);
      VA v;
      for ( usize it = 0; it < 20000; ++it ) {
        u64 op = rng.next() % 5;
        if ( op == 0 )
          v.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
        else if ( op == 1 )
          v.emplace_back(static_cast<int>(rng.next() & 0xffff));
        else if ( op == 2 ) {
          if ( !v.empty() ) v.pop_back();
        } else if ( op == 3 ) {
          if ( !v.empty() ) v.erase(static_cast<usize>(rng.next() % v.size()));
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

  test_case("cv move-assign frees dest + empties donor (no leak)");
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
    }
    ck(TA::outstanding() == 0, "no-leak", 0);
    ck(Tr::live() == 0u, "balanced", 0);
  }
  end_test_case();

  test_case("cv throwing allocator mid-grow: throws + no leak");
  {
    using THA = mtest::throwing_allocator<14>;
    using VT = micron::convector<u32, THA>;
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

static void
mt_push(micron::convector<u64> *v, u64 base, u64 cnt)
{
  for ( u64 i = 0; i < cnt; ++i ) v->push_back(base + i);
}

static void
mt_readwrite(micron::convector<u64> *v, u64 base, u64 cnt)
{
  for ( u64 i = 0; i < cnt; ++i ) {
    v->push_back(base + i);

    volatile usize sz = v->size();
    if ( sz > 0 ) {
      volatile u64 f = v->front();
      (void)f;
    }
    (void)sz;
  }
}

static void
run_concurrency(void)
{
  static constexpr u64 PER = 2500;
  static constexpr u64 BASES[4] = { 0ull, 1000000ull, 2000000ull, 3000000ull };

  test_case("cv concurrent producers: no lost updates (multiset)");
  {
    micron::convector<u64> shared;
    {
      micron::auto_thread<> t0(mt_push, micron::addr(shared), BASES[0], PER);
      micron::auto_thread<> t1(mt_push, micron::addr(shared), BASES[1], PER);
      micron::auto_thread<> t2(mt_push, micron::addr(shared), BASES[2], PER);
      micron::auto_thread<> t3(mt_push, micron::addr(shared), BASES[3], PER);
    }
    require(shared.size(), static_cast<usize>(4 * PER));

    shared.sort();
    bool ok = true;
    for ( u64 t = 0; t < 4 && ok; ++t )
      for ( u64 i = 0; i < PER; ++i ) {
        usize idx = static_cast<usize>(t * PER + i);
        if ( shared[idx] != BASES[t] + i ) {
          ok = false;
          break;
        }
      }
    require(ok, true);
  }
  end_test_case();

  test_case("cv concurrent readers+writers: no crash / no torn size");
  {
    micron::convector<u64> shared;
    {
      micron::auto_thread<> t0(mt_readwrite, micron::addr(shared), BASES[0], PER);
      micron::auto_thread<> t1(mt_readwrite, micron::addr(shared), BASES[1], PER);
      micron::auto_thread<> t2(mt_readwrite, micron::addr(shared), BASES[2], PER);
    }
    require(shared.size(), static_cast<usize>(3 * PER));
  }
  end_test_case();
}

static void
run_adversarial(void)
{
  using V = micron::convector<u32>;

  test_case("cv OOB at()/insert/erase + empty throw library_error");
  {
    V v;
    for ( u32 i = 0; i < 6u; ++i ) v.push_back(i);
    expect_throw_type<micron::except::library_error>([&] { (void)v.at(6); });
    expect_throw_type<micron::except::library_error>([&] { v.erase(6u); });
    expect_throw_type<micron::except::library_error>([&] { v.erase(2u, 2u); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.insert(6u, 99u); });
    V e;
    expect_throw_type<micron::except::library_error>([&] { (void)e.front(); });
    expect_throw_type<micron::except::library_error>([&] { e.pop_back(); });
  }
  end_test_case();
}

template<typename E>
static void
run_all(const char *tyname)
{
  sb::print("--- convector<", tyname, "> ---");
  run_props<E>();
}

int
main(int, char **)
{
  sb::print("=== CONVECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- nested containers (convector<vector<...>>) ---");
  run_nested();

  sb::print("--- memory / lifetime / exception (Tracked + allocators) ---");
  run_memory();

  sb::print("--- concurrency (auto_thread) ---");
  run_concurrency();

  sb::print("--- adversarial / edges ---");
  run_adversarial();

  sb::print("=== CONVECTOR RIGOR DONE ===");
  return 1;
}
