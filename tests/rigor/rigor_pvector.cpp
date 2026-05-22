// Exhaustive, adversarial rigor suite for micron::pvector<T, K, H, Sf>.
//
// pvector is an immutable, persistent B-ary trie with path copying: every
// "mutator" returns a fresh pvector (O(log B) sharing the untouched subtrees),
// the source is unchanged, and copy/move are O(1) via refcounting. This suite
// asserts both the result and the source-unchanged invariant against ref_vec
// (tests/support/vector_rigor.hpp), checks identity()/structural-sharing, the
// functional layer (arith/reductions/predicates, gated to capable types), and
// — since pvector allocates through abc directly (no Alloc param) — relies on
// ASan + a refcount churn for memory correctness. Out-of-range ops throw
// except::runtime_error (operator[] is unchecked).
//
// Build: `duck build tests/rigor/rigor_pvector.cpp`; run `bin/rigor_pvector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/pvector.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
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
static constexpr usize CAP = 4096;        // oracle capacity (> any test size)
static constexpr usize MAXLEN = 300;      // crosses the B=32 leaf boundary repeatedly

template<typename A, typename B> inline constexpr bool same_t = false;
template<typename A> inline constexpr bool same_t<A, A> = true;
template<typename E> inline constexpr bool orderable = micron::is_integral_v<E> || same_t<E, mtest::big>;
template<typename E> inline constexpr bool arith = micron::is_integral_v<E>;

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

// build a pvector<E> from keys (via the pointer ctor) + populate the oracle.
template<typename E>
static micron::pvector<E>
build_pv(mtest::ref_vec<CAP> &r, const u64 *ks, usize n)
{
  E es[MAXLEN + 4];
  for ( usize i = 0; i < n; ++i ) {
    es[i] = mtest::elem<E>::make(ks[i]);
    r.push_back(mtest::elem<E>::key(es[i]));
  }
  return micron::pvector<E>(static_cast<const E *>(es), n);
}

// pvector::operator[] is const-only and returns const T&; vec_eq works as-is.

// ───────────────────────────────────────────────────────────────────────────
// structural property groups (all element types)
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_props(void)
{
  using PV = micron::pvector<E>;

  // ── construction / O(1) copy sharing ───────────────────────────────────────
  test_case("pv construction + identity sharing vs oracle");
  {
    prng rng(0x6101u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::full);
      mtest::ref_vec<CAP> r;
      PV p = build_pv<E>(r, ks, n);
      ck(mtest::vec_eq<E>(p, r), "build", it);
      // O(1) copy shares the root
      PV c(p);
      ck(mtest::vec_eq<E>(c, r), "copy", it);
      ck(c.identity() == p.identity(), "copy-shares-root", it);
      // move transfers root
      PV m(micron::move(c));
      ck(mtest::vec_eq<E>(m, r), "move", it);
      ck(m.identity() == p.identity(), "move-same-root", it);
      // ctor(n, val)
      u64 k = mtest::gen_raw(rng, band::small);
      PV f(n, mtest::elem<E>::make(k));
      mtest::ref_vec<CAP> rf;
      rf.assign(n, mtest::elem<E>::key(mtest::elem<E>::make(k)));
      ck(mtest::vec_eq<E>(f, rf), "ctor-n-val", it);
    }
  }
  end_test_case();

  // ── push_back / emplace_back / pop_back / set / update (immutable) ──────────
  test_case("pv push/pop/set/update immutable + path-copy vs oracle");
  {
    prng rng(0x6102u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> rsrc;
      PV src = build_pv<E>(rsrc, ks, n);
      // push_back
      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(mtest::elem<E>::key(e));
        PV res = src.push_back(e);
        ck(mtest::vec_eq<E>(res, rr), "push_back-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "push_back-source", it);
        ck(res.identity() != src.identity(), "push_back-new-root", it);
      }
      // emplace_back
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        PV res = mtest::elem<E>::emplace_iv(src, raw);
        ck(mtest::vec_eq<E>(res, rr), "emplace-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "emplace-source", it);
      }
      // pop_back
      {
        mtest::ref_vec<CAP> rr = rsrc;
        rr.pop_back();
        PV res = src.pop_back();
        ck(mtest::vec_eq<E>(res, rr), "pop_back-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "pop_back-source", it);
      }
      // set
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.set(idx, mtest::elem<E>::key(e));
        PV res = src.set(idx, e);
        ck(mtest::vec_eq<E>(res, rr), "set-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "set-source", it);
      }
      // update(fn): add 1 to the key (width-wrapping)
      if constexpr ( micron::is_integral_v<E> ) {
        usize idx = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.set(idx, mtest::elem<E>::key(static_cast<E>(rsrc.buf[idx] + 1)));
        PV res = src.update(idx, [](const E &v) { return static_cast<E>(v + 1); });
        ck(mtest::vec_eq<E>(res, rr), "update-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "update-source", it);
      }
    }
  }
  end_test_case();

  // ── insert / erase (immutable) ─────────────────────────────────────────────
  test_case("pv insert/erase immutable vs oracle");
  {
    prng rng(0x6103u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> rsrc;
      PV src = build_pv<E>(rsrc, ks, n);
      // insert(pos, val) — pos in [0, n] (insert allows append position)
      {
        usize pos = static_cast<usize>(rng.next_in(n + 1));
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(pos, mtest::elem<E>::key(e));
        PV res = src.insert(pos, e);
        ck(mtest::vec_eq<E>(res, rr), "insert-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "insert-source", it);
      }
      // insert(pos, val, cnt)
      {
        usize pos = static_cast<usize>(rng.next_in(n + 1));
        usize cnt = static_cast<usize>(rng.next_in(5)) + 1;
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.insert(pos, mtest::elem<E>::key(e), cnt);
        PV res = src.insert(pos, e, cnt);
        ck(mtest::vec_eq<E>(res, rr), "insert-cnt-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "insert-cnt-source", it);
      }
      // erase(pos)
      {
        usize pos = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase(pos);
        PV res = src.erase(pos);
        ck(mtest::vec_eq<E>(res, rr), "erase-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "erase-source", it);
      }
      // erase(from, to)
      {
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase_range(from, to);
        PV res = src.erase(from, to);
        ck(mtest::vec_eq<E>(res, rr), "erase-range-result", it);
        ck(mtest::vec_eq<E>(src, rsrc), "erase-range-source", it);
      }
    }
  }
  end_test_case();

  // ── resize / fill / assign / append / remove / clear ───────────────────────
  test_case("pv resize/fill/assign/append/remove vs oracle");
  {
    prng rng(0x6104u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, 1, MAXLEN, band::small);
      gen_keys(rng, b, nb, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> ra;
      PV pa = build_pv<E>(ra, a, na);
      mtest::ref_vec<CAP> rb;
      PV pb = build_pv<E>(rb, b, nb);
      // resize grow + shrink
      {
        usize g = na + static_cast<usize>(rng.next_in(100));
        mtest::ref_vec<CAP> rr = ra;
        rr.resize(g);
        PV res = pa.resize(g);
        ck(mtest::vec_eq<E>(res, rr), "resize-grow", it);
        usize s = static_cast<usize>(rng.next_in(na + 1));
        mtest::ref_vec<CAP> rr2 = ra;
        rr2.resize(s);
        PV res2 = pa.resize(s);
        ck(mtest::vec_eq<E>(res2, rr2), "resize-shrink", it);
        ck(mtest::vec_eq<E>(pa, ra), "resize-source", it);
      }
      // assign / fill produce fresh
      {
        usize cnt = static_cast<usize>(rng.next_in(150)) + 1;
        u64 k = mtest::gen_raw(rng, band::small);
        mtest::ref_vec<CAP> rr;
        rr.assign(cnt, mtest::elem<E>::key(mtest::elem<E>::make(k)));
        PV res = pa.assign(cnt, mtest::elem<E>::make(k));
        ck(mtest::vec_eq<E>(res, rr), "assign", it);
      }
      // append + operator+
      {
        mtest::ref_vec<CAP> rr = ra;
        rr.append(rb.buf, rb.len);
        PV res = pa.append(pb);
        ck(mtest::vec_eq<E>(res, rr), "append-result", it);
        PV res2 = pa + pb;
        ck(mtest::vec_eq<E>(res2, rr), "operator+-result", it);
        ck(mtest::vec_eq<E>(pa, ra), "append-source", it);
      }
      // remove all occurrences of a small key
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        mtest::ref_vec<CAP> rr = ra;
        rr.remove_val(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        PV res = pa.remove(mtest::elem<E>::make(raw));
        ck(mtest::vec_eq<E>(res, rr), "remove-result", it);
        ck(mtest::vec_eq<E>(pa, ra), "remove-source", it);
      }
      // clear
      {
        PV res = pa.clear();
        ck(res.size() == 0u, "clear", it);
        ck(mtest::vec_eq<E>(pa, ra), "clear-source", it);
      }
    }
  }
  end_test_case();

  // ── access / iterate / find / contains ──────────────────────────────────────
  test_case("pv access/iterate/find/contains vs oracle");
  {
    prng rng(0x6105u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> r;
      PV p = build_pv<E>(r, ks, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(mtest::elem<E>::key(p[idx]) == r.buf[idx], "op[]", it);
      ck(mtest::elem<E>::key(p.at(idx)) == r.buf[idx], "at", it);
      ck(mtest::elem<E>::key(p.get(idx)) == r.buf[idx], "get", it);
      ck(mtest::elem<E>::key(p.front()) == r.buf[0], "front", it);
      ck(mtest::elem<E>::key(p.back()) == r.buf[n - 1], "back", it);
      ck(p.size() == n, "size", it);
      // at() OOB throws runtime_error
      expect_throw_type<micron::except::runtime_error>([&] { (void)p.at(n); });
      // iterate
      usize j = 0;
      bool ok = true;
      for ( auto i = p.begin(); i != p.end(); ++i, ++j )
        if ( mtest::elem<E>::key(*i) != r.buf[j] ) {
          ok = false;
          break;
        }
      ck(ok && j == n, "begin/end", it);
      // find + contains
      u64 fk = mtest::gen_raw(rng, band::small);
      usize oi = r.find(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
      auto fi = p.find(mtest::elem<E>::make(fk));
      bool contains = p.contains(mtest::elem<E>::make(fk));
      if ( oi == mtest::VREF_NPOS ) {
        ck(fi == p.end(), "find-miss", it);
        ck(!contains, "contains-miss", it);
      } else {
        ck(fi != p.end() && fi.index() == oi, "find-hit", it);
        ck(contains, "contains-hit", it);
      }
    }
  }
  end_test_case();

  // ── slice operator[](from,to) ───────────────────────────────────────────────
  test_case("pv slice operator[](from,to) vs oracle");
  {
    prng rng(0x6106u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < 6000; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, MAXLEN, band::small);
      mtest::ref_vec<CAP> r;
      PV p = build_pv<E>(r, ks, n);
      usize from = static_cast<usize>(rng.next_in(n));
      usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
      PV sub = p[from, to];
      mtest::ref_vec<CAP> rsub;
      for ( usize i = from; i < to; ++i ) rsub.push_back(r.buf[i]);
      ck(mtest::vec_eq<E>(sub, rsub), "slice", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// ordered ops (sort / min / max) — element types with operator<
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_ordered(void)
{
  using PV = micron::pvector<E>;
  test_case("pv sort/min/max vs oracle");
  {
    prng rng(0x6201u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, 200, band::full);
      mtest::ref_vec<CAP> r;
      PV p = build_pv<E>(r, ks, n);
      // sort
      {
        mtest::ref_vec<CAP> rr = r;
        rr.sort();
        PV res = p.sort();
        ck(mtest::vec_eq<E>(res, rr), "sort", it);
        ck(mtest::vec_eq<E>(p, r), "sort-source", it);
      }
      // min / max
      ck(mtest::elem<E>::key(p.min()) == r.vmin(), "min", it);
      ck(mtest::elem<E>::key(p.max()) == r.vmax(), "max", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// functional / arithmetic ops — integral element types
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_arith(void)
{
  using PV = micron::pvector<E>;
  test_case("pv arith/reduce/predicates vs oracle");
  {
    prng rng(0x6301u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 1, 200, band::small);
      mtest::ref_vec<CAP> r;
      PV p = build_pv<E>(r, ks, n);
      E s = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
      // operator+ scalar (element-wise, width-wrapping)
      {
        mtest::ref_vec<CAP> rr;
        for ( usize i = 0; i < n; ++i ) rr.push_back(mtest::elem<E>::key(static_cast<E>(static_cast<E>(r.buf[i]) + s)));
        PV res = p + s;
        ck(mtest::vec_eq<E>(res, rr), "op+scalar", it);
      }
      // operator* scalar
      {
        mtest::ref_vec<CAP> rr;
        for ( usize i = 0; i < n; ++i ) rr.push_back(mtest::elem<E>::key(static_cast<E>(static_cast<E>(r.buf[i]) * s)));
        PV res = p * s;
        ck(mtest::vec_eq<E>(res, rr), "op*scalar", it);
      }
      // operator- scalar
      {
        mtest::ref_vec<CAP> rr;
        for ( usize i = 0; i < n; ++i ) rr.push_back(mtest::elem<E>::key(static_cast<E>(static_cast<E>(r.buf[i]) - s)));
        PV res = p - s;
        ck(mtest::vec_eq<E>(res, rr), "op-scalar", it);
      }
      // sum (width-wrapping accumulation from T{})
      {
        E acc{};
        for ( usize i = 0; i < n; ++i ) acc = static_cast<E>(acc + static_cast<E>(r.buf[i]));
        ck(static_cast<E>(p.sum()) == acc, "sum", it);
      }
      // reduce(init, +)
      {
        E init = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        E acc = init;
        for ( usize i = 0; i < n; ++i ) acc = static_cast<E>(acc + static_cast<E>(r.buf[i]));
        E got = p.reduce(init, [](const E &x, const E &y) { return static_cast<E>(x + y); });
        ck(got == acc, "reduce", it);
      }
      // any(v) / all(v) / all_of / any_of
      {
        u64 vk = mtest::gen_raw(rng, band::small);
        E v = mtest::elem<E>::make(vk);
        bool oany = r.contains(mtest::elem<E>::key(v));
        ck(p.any(v) == oany, "any", it);
        bool oall = true;
        for ( usize i = 0; i < n; ++i )
          if ( r.buf[i] != mtest::elem<E>::key(v) ) {
            oall = false;
            break;
          }
        ck(p.all(v) == oall, "all", it);
        // all_of even, any_of zero
        bool oeven = true, ozero = false;
        for ( usize i = 0; i < n; ++i ) {
          if ( (r.buf[i] & 1u) != 0 ) oeven = false;
          if ( r.buf[i] == 0 ) ozero = true;
        }
        ck(p.all_of([](const E &x) { return (x & 1u) == 0; }) == oeven, "all_of", it);
        ck(p.any_of([](const E &x) { return x == static_cast<E>(0); }) == ozero, "any_of", it);
      }
      // map (+1) / filter (even)
      {
        mtest::ref_vec<CAP> rm;
        for ( usize i = 0; i < n; ++i ) rm.push_back(mtest::elem<E>::key(static_cast<E>(static_cast<E>(r.buf[i]) + 1)));
        PV pm = p.map([](const E &x) { return static_cast<E>(x + 1); });
        ck(mtest::vec_eq<E>(pm, rm), "map", it);
        mtest::ref_vec<CAP> rf;
        for ( usize i = 0; i < n; ++i )
          if ( (r.buf[i] & 1u) == 0 ) rf.push_back(r.buf[i]);
        PV pf = p.filter([](const E &x) { return (x & 1u) == 0; });
        ck(mtest::vec_eq<E>(pf, rf), "filter", it);
      }
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// nested: pvector<vector<u32>>
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

template<typename PVV>
static bool
nested_eq(const PVV &pv, const mtest::ref_vec<CAP> &r)
{
  if ( static_cast<usize>(pv.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( rowkey(pv[i]) != r.buf[i] ) return false;
  return true;
}

static void
run_nested(void)
{
  using V = micron::vector<u32>;
  using PVV = micron::pvector<V>;

  test_case("pvector<vector<u32>> immutable nested ops vs oracle");
  {
    prng rng(0xEEFFu);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = 1 + static_cast<usize>(rng.next_in(30));
      micron::vector<V> rows;
      mtest::ref_vec<CAP> rsrc;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        rsrc.push_back(rowkey(row));
        rows.push_back(micron::move(row));
      }
      PVV src(rows);
      ck(nested_eq(src, rsrc), "build", it);
      // push_back
      {
        V row = mkrow(rng);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.push_back(rowkey(row));
        PVV res = src.push_back(micron::move(row));
        ck(nested_eq(res, rr), "push_back-result", it);
        ck(nested_eq(src, rsrc), "push_back-source", it);
      }
      // set
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        V row = mkrow(rng);
        mtest::ref_vec<CAP> rr = rsrc;
        rr.set(idx, rowkey(row));
        PVV res = src.set(idx, micron::move(row));
        ck(nested_eq(res, rr), "set-result", it);
        ck(nested_eq(src, rsrc), "set-source", it);
      }
      // erase
      {
        usize idx = static_cast<usize>(rng.next_in(n));
        mtest::ref_vec<CAP> rr = rsrc;
        rr.erase(idx);
        PVV res = src.erase(idx);
        ck(nested_eq(res, rr), "erase-result", it);
        ck(nested_eq(src, rsrc), "erase-source", it);
      }
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// refcount / structural-sharing memory invariant (ASan catches double-free/UAF)
// ───────────────────────────────────────────────────────────────────────────

static void
run_refcount(void)
{
  using PV = micron::pvector<u32>;

  test_case("pv refcount: many copies destroyed in random order, survivors intact");
  {
    prng rng(0x9F1Fu);
    for ( usize it = 0; it < 4000; ++it ) {
      usize n = 1 + static_cast<usize>(rng.next_in(200));
      u32 es[256];
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < n; ++i ) {
        es[i] = static_cast<u32>(rng.next());
        r.push_back(es[i]);
      }
      PV base(static_cast<const u32 *>(es), n);
      // a pile of O(1) copies all sharing base's root
      micron::vector<PV> copies;
      for ( int i = 0; i < 16; ++i ) copies.push_back(base);
      // derive a few new versions (path-copying off shared root)
      PV d1 = base.push_back(123u);
      PV d2 = base.set(static_cast<usize>(rng.next_in(n)), 999u);
      // destroy copies in a shuffled order
      while ( copies.size() ) {
        usize k = static_cast<usize>(rng.next_in(copies.size()));
        copies.erase(k);
      }
      // base + derivations must remain valid/correct
      ck(mtest::vec_eq<u32>(base, r), "base-intact", it);
      ck(d1.size() == n + 1 && d2.size() == n, "deriv-sizes", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// adversarial / edges
// ───────────────────────────────────────────────────────────────────────────

static void
run_adversarial(void)
{
  using PV = micron::pvector<u32>;

  test_case("pv OOB ops throw runtime_error");
  {
    PV v(5u, 9u);
    expect_throw_type<micron::except::runtime_error>([&] { (void)v.at(5); });
    expect_throw_type<micron::except::runtime_error>([&] { (void)v.set(5u, 1u); });
    expect_throw_type<micron::except::runtime_error>([&] { (void)v.erase(5u); });
    expect_throw_type<micron::except::runtime_error>([&] { (void)v.insert(6u, 1u); });      // pos>size
  }
  end_test_case();

  test_case("pv empty + operator== structural");
  {
    PV e;
    require(e.empty(), true);
    require(e.size(), 0u);
    PV a(3u, 7u);
    PV b = a;      // shares root
    require((a == b), true);
    PV c = a.set(0u, 7u);         // same values, different root
    require((a == c), true);      // structural equality
    require(a.identity() == b.identity(), true);
    require(a.identity() != c.identity(), true);
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
  sb::print("--- pvector<", tyname, "> ---");
  run_props<E>();
  if constexpr ( orderable<E> ) run_ordered<E>();
  if constexpr ( arith<E> ) run_arith<E>();
}

int
main(int, char **)
{
  sb::print("=== PVECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- nested containers (pvector<vector<...>>) ---");
  run_nested();

  sb::print("--- refcount / structural sharing ---");
  run_refcount();

  sb::print("--- adversarial / edges ---");
  run_adversarial();

  sb::print("=== PVECTOR RIGOR DONE ===");
  return 1;
}
