// Exhaustive, adversarial rigor suite for micron::ivector<T, Alloc, Sf>.
//
// ivector is an immutable, heap-allocated vector: every "mutator" returns a
// fresh ivector and leaves the source unchanged. This suite asserts BOTH
// invariants every iteration (result matches the post-op oracle; source still
// matches the pre-op oracle), diffing against ref_vec
// (tests/support/vector_rigor.hpp). Out-of-range access throws library_error.
// Memory: each op allocates fresh, so churn is wrapped in expect_leak_free and
// Tracked ctor/dtor balance is asserted; nested ivector<vector<u32>> exercises
// resource-owning elements (operator& / deep-copy correctness).
//
// Element-type coverage: u8/u16/u32/u64, the 24-byte big, a non-trivial Tracked.
//
// Build: `duck build tests/rigor/rigor_ivector.cpp`; run `bin/rigor_ivector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/ivector.hpp"

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
static constexpr usize MAXLEN = 200;

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

static void
gen_keys(prng &rng, u64 *ks, usize &n, usize lo, usize hi, band b)
{
  n = lo + static_cast<usize>(rng.next() % (hi - lo + 1));
  for ( usize i = 0; i < n; ++i ) ks[i] = mtest::gen_raw(rng, b);
}

// build an ivector<E> from keys (via the converting ctor from a mutable vector)
// and populate the matching oracle.
template<typename E>
static micron::ivector<E>
build_iv(mtest::ref_vec<CAP> &r, const u64 *ks, usize n)
{
  micron::vector<E> tmp;
  for ( usize i = 0; i < n; ++i ) {
    E e = mtest::elem<E>::make(ks[i]);
    r.push_back(mtest::elem<E>::key(e));
    tmp.push_back(micron::move(e));
  }
  return micron::ivector<E>(tmp);
}

// ───────────────────────────────────────────────────────────────────────────
// property groups
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_props(void)
{
  using IV = micron::ivector<E>;

  // ── construction ────────────────────────────────────────────────────────────
  test_case("iv construction vs oracle");
  {
    prng rng(0x5101u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::full);
      // from mutable vector + copy ctor + move ctor
      mtest::ref_vec<CAP> r;
      IV iv = build_iv<E>(r, ks, n);
      ck(mtest::vec_eq<E>(iv, r), "from-vector", it);
      IV cp(iv);
      ck(mtest::vec_eq<E>(cp, r), "copy-ctor", it);
      ck(mtest::vec_eq<E>(iv, r), "copy-ctor-src", it);
      IV mv(micron::move(cp));
      ck(mtest::vec_eq<E>(mv, r), "move-ctor", it);
      // ivector(n, val)
      u64 k = mtest::gen_raw(rng, band::small);
      IV f(n, mtest::elem<E>::make(k));
      mtest::ref_vec<CAP> rf;
      rf.assign(n, mtest::elem<E>::key(mtest::elem<E>::make(k)));
      ck(mtest::vec_eq<E>(f, rf), "ctor-n-val", it);
    }
  }
  end_test_case();

  // ── push_back / push_front / emplace_back / pop_back (immutable) ────────────
  test_case("iv push/pop/emplace immutable vs oracle");
  {
    prng rng(0x5102u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, 150, band::small);
      mtest::ref_vec<CAP> rsrc;
      IV src = build_iv<E>(rsrc, ks, n);
      // push_back(const T&)
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(mtest::elem<E>::key(e));
        IV res = src.push_back(e);
        ck(mtest::vec_eq<E>(res, rr), "push_back-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "push_back-source", it);
      }
      // push_back(T&&)
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(mtest::elem<E>::key(e));
        IV res = src.push_back(micron::move(e));
        ck(mtest::vec_eq<E>(res, rr), "push_back-move-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "push_back-move-source", it);
      }
      // push_front
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_front(mtest::elem<E>::key(e));
        IV res = src.push_front(e);
        ck(mtest::vec_eq<E>(res, rr), "push_front-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "push_front-source", it);
      }
      // emplace_back
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        IV res = mtest::elem<E>::emplace_iv(src, raw);
        ck(mtest::vec_eq<E>(res, rr), "emplace-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "emplace-source", it);
      }
      // pop_back
      {
        mtest::ref_vec<CAP> rr = rsrc;
        rr.pop_back();
        IV res = src.pop_back();
        ck(mtest::vec_eq<E>(res, rr), "pop_back-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "pop_back-source", it);
      }
    }
  }
  end_test_case();

  // ── insert (immutable) — n in [0, length-1] (insert throws at n>=length) ────
  test_case("iv insert immutable vs oracle");
  {
    prng rng(0x5103u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, 150, band::small);
      mtest::ref_vec<CAP> rsrc;
      IV src = build_iv<E>(rsrc, ks, n);
      usize idx = static_cast<usize>(rng.next_in(n));      // [0, n-1]
      // insert(n, val)
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(idx, mtest::elem<E>::key(e));
        IV res = src.insert(idx, e);
        ck(mtest::vec_eq<E>(res, rr), "insert-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "insert-source", it);
      }
      // insert(n, val, cnt)
      {
        usize cnt = static_cast<usize>(rng.next_in(5)) + 1;
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(idx, mtest::elem<E>::key(e), cnt);
        IV res = src.insert(idx, e, cnt);
        ck(mtest::vec_eq<E>(res, rr), "insert-cnt-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "insert-cnt-source", it);
      }
      // insert(iterator, val)
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(idx, mtest::elem<E>::key(e));
        IV res = src.insert(src.begin() + idx, e);
        ck(mtest::vec_eq<E>(res, rr), "insert-it-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "insert-it-source", it);
      }
    }
  }
  end_test_case();

  // ── erase (immutable) ──────────────────────────────────────────────────────
  test_case("iv erase immutable vs oracle");
  {
    prng rng(0x5104u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, 200, band::full);
      mtest::ref_vec<CAP> rsrc;
      IV src = build_iv<E>(rsrc, ks, n);
      // erase(n)
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase(idx);
        IV res = src.erase(idx);
        ck(mtest::vec_eq<E>(res, rr), "erase-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "erase-source", it);
      }
      // erase(from, to)
      {
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase_range(from, to);
        IV res = src.erase(from, to);
        ck(mtest::vec_eq<E>(res, rr), "erase-range-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "erase-range-source", it);
      }
      // erase(iterator)
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase(idx);
        IV res = src.erase(src.begin() + idx);
        ck(mtest::vec_eq<E>(res, rr), "erase-it-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "erase-it-source", it);
      }
    }
  }
  end_test_case();

  // ── assign / append / operator+ / clear ────────────────────────────────────
  test_case("iv assign/append/operator+/clear vs oracle");
  {
    prng rng(0x5105u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, 1, MAXLEN, band::small);
      gen_keys(rng, b, nb, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> ra;
      IV ia = build_iv<E>(ra, a, na);
      mtest::ref_vec<CAP> rb;
      IV ib = build_iv<E>(rb, b, nb);
      // assign returns fresh
      {
        usize cnt = static_cast<usize>(rng.next_in(150)) + 1;
        u64 k = mtest::gen_raw(rng, band::small);
        mtest::ref_vec<CAP> rr;
        rr.assign(cnt, mtest::elem<E>::key(mtest::elem<E>::make(k)));
        IV res = ia.assign(cnt, mtest::elem<E>::make(k));
        ck(mtest::vec_eq<E>(res, rr), "assign-result", it);
        ck(mtest::vec_eq<E>(ia, ra), "assign-source", it);
      }
      // append + operator+
      {
        mtest::ref_vec<CAP> rr = ra;
        rr.append(rb.buf, rb.len);
        IV res = ia.append(ib);
        ck(mtest::vec_eq<E>(res, rr), "append-result", it);
        IV res2 = ia + ib;
        ck(mtest::vec_eq<E>(res2, rr), "operator+-result", it);
        ck(mtest::vec_eq<E>(ia, ra), "append-source-a", it);
        ck(mtest::vec_eq<E>(ib, rb), "append-source-b", it);
      }
      // clear returns empty
      {
        IV res = ia.clear();
        ck(res.size() == 0u, "clear-result", it);
        ck(mtest::vec_eq<E>(ia, ra), "clear-source", it);
      }
    }
  }
  end_test_case();

  // ── read-only access + find + iteration ─────────────────────────────────────
  test_case("iv access/find/iterate vs oracle");
  {
    prng rng(0x5106u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> r;
      IV iv = build_iv<E>(r, ks, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(mtest::elem<E>::key(iv[idx]) == r.buf[idx], "op[]", it);
      ck(mtest::elem<E>::key(iv.at(idx)) == r.buf[idx], "at", it);
      ck(mtest::elem<E>::key(iv.front()) == r.buf[0], "front", it);
      ck(mtest::elem<E>::key(iv.back()) == r.buf[n - 1], "back", it);
      ck(mtest::elem<E>::key(*iv.last()) == r.buf[n - 1], "last", it);
      ck(iv.size() == n && iv.capacity() >= n, "size/cap", it);
      // at() OOB throws
      expect_throw_type<micron::except::library_error>([&] { (void)iv.at(n); });
      // iterate
      usize j = 0;
      bool ok = true;
      for ( auto i = iv.begin(); i != iv.end(); ++i, ++j )
        if ( mtest::elem<E>::key(*i) != r.buf[j] ) {
          ok = false;
          break;
        }
      ck(ok && j == n, "begin/end", it);
      // find
      u64 fk = mtest::gen_raw(rng, band::small);
      usize oi = r.find(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
      auto p = iv.find(mtest::elem<E>::make(fk));
      if ( oi == mtest::VREF_NPOS )
        ck(p == nullptr, "find-miss", it);
      else
        ck(p != nullptr && iv.at_n(p) == oi, "find-hit", it);
    }
  }
  end_test_case();

  // ── to_persist(vector) ─────────────────────────────────────────────────────
  test_case("iv to_persist(vector) vs oracle");
  {
    prng rng(0x5107u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 0, MAXLEN, band::full);
      micron::vector<E> v;
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < n; ++i ) {
        E e = mtest::elem<E>::make(ks[i]);
        r.push_back(mtest::elem<E>::key(e));
        v.push_back(micron::move(e));
      }
      auto iv = micron::to_persist(v);
      ck(mtest::vec_eq<E>(iv, r), "to_persist", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// nested: ivector<vector<u32>>
// ───────────────────────────────────────────────────────────────────────────

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
  using IVV = micron::ivector<V>;

  test_case("ivector<vector<u32>> immutable nested ops vs oracle");
  {
    prng rng(0xCCDDu);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = 1 + static_cast<usize>(rng.next_in(30));
      // build via mutable vector of rows
      micron::vector<V> tmp;
      mtest::ref_vec<CAP> rsrc;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        rsrc.push_back(rowkey(row));
        tmp.push_back(micron::move(row));
      }
      IVV src(tmp);
      ck(nested_eq(src, rsrc), "build", it);
      // push_back a row (immutable)
      {
        V row = mkrow(rng);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(rowkey(row));
        IVV res = src.push_back(micron::move(row));
        ck(nested_eq(res, rr), "push_back-result", it);
        ck(nested_eq(src, rsrc), "push_back-source", it);
      }
      // insert a row
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        V row = mkrow(rng);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(idx, rowkey(row));
        IVV res = src.insert(idx, micron::move(row));
        ck(nested_eq(res, rr), "insert-result", it);
        ck(nested_eq(src, rsrc), "insert-source", it);
      }
      // erase a row
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase(idx);
        IVV res = src.erase(idx);
        ck(nested_eq(res, rr), "erase-result", it);
        ck(nested_eq(src, rsrc), "erase-source", it);
      }
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// memory: each op allocates fresh -> churn must be leak-free; Tracked balanced
// ───────────────────────────────────────────────────────────────────────────

static void
run_memory(void)
{
  using Tr = mtest::Tracked<11>;
  using TA = mtest::tracking_allocator<11>;
  using IV = micron::ivector<Tr, TA>;

  test_case("iv immutable churn leak-free + Tracked balanced");
  {
    Tr::reset();
    expect_leak_free<TA>([] {
      prng rng(0x1DEA01u);
      // seed from a mutable vector
      micron::vector<Tr> seedv;
      for ( int i = 0; i < 20; ++i ) seedv.push_back(Tr(i));
      IV cur(seedv);
      // Bounded random walk: each op allocates a fresh ivector and (after the
      // move-assign-frees-dest fix) releases the previous one, so this is the
      // leak/balance probe. Size is capped so the arena's peak stays modest.
      for ( usize it = 0; it < 8000; ++it ) {
        u64 op = rng.next() % 4;
        if ( op == 0 && cur.size() < 48 ) {
          cur = cur.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
        } else if ( op == 3 && cur.size() > 1 && cur.size() < 48 ) {
          cur = cur.insert(static_cast<usize>(rng.next() % cur.size()), Tr(7));
        } else if ( cur.size() > 1 ) {
          cur = cur.erase(static_cast<usize>(rng.next() % cur.size()));
        } else {
          cur = cur.push_back(Tr(1));
        }
      }
    });
    ck(Tr::live() == 0u, "tracked-balance", 0);
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// adversarial / edges
// ───────────────────────────────────────────────────────────────────────────

static void
run_adversarial(void)
{
  using IV = micron::ivector<u32>;

  test_case("iv OOB access throws library_error");
  {
    IV v(5u, 9u);
    expect_throw_type<micron::except::library_error>([&] { (void)v.at(5); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.insert(5u, 1u); });      // n>=length
    expect_throw_type<micron::except::library_error>([&] { (void)v.erase(5u); });
  }
  end_test_case();

  test_case("iv static_assert: not default constructible");
  {
    static_assert(!micron::is_default_constructible_v<IV>, "ivector() is deleted");
    require(true, true);
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// driver
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_all(const char *tyname)
{
  sb::print("--- ivector<", tyname, "> ---");
  run_props<E>();
}

int
main(int, char **)
{
  sb::print("=== IVECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- nested containers (ivector<vector<...>>) ---");
  run_nested();

  sb::print("--- memory / lifetime (Tracked + allocator) ---");
  run_memory();

  sb::print("--- adversarial / edges ---");
  run_adversarial();

  sb::print("=== IVECTOR RIGOR DONE ===");
  return 1;
}
