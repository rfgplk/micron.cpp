// Exhaustive, adversarial rigor suite for micron::svector<T, N, Sf>.
//
// svector is a fixed-capacity, stack-allocated, mutable vector (requires
// is_regular_object<T>, i.e. copyable). This suite drives every member function
// from an adversarial standpoint: each group pairs edge cases with a
// >=10k-iteration randomized property loop that diffs svector against the
// ref_vec oracle (tests/support/vector_rigor.hpp), plus overflow / out-of-range
// cases that must throw. Note svector's exception + miss conventions differ
// from the heap vector: at()/erase/insert/fill range errors throw
// runtime_error, a too-large ctor throws library_error, operator[] is
// unchecked, and find() returns end() (not nullptr) on a miss.
//
// Element-type coverage: run_props<E>() at a representative N is instantiated
// for u8/u16/u32/u64, the 24-byte big, and a non-trivial Tracked. Capacity /
// overflow behaviour is exercised at N in {8,16,64,256} via run_caps<N,E>().
// Memory correctness for a stack vector == no OOB (ASan stack-buffer-overflow)
// and Tracked ctor/dtor balance.
//
// Build: `duck build tests/rigor/rigor_svector.cpp`; run `bin/rigor_svector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/svector.hpp"

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
static constexpr usize SN = 256;          // representative capacity for run_props
static constexpr usize SCAP = 512;        // oracle capacity (> SN)
static constexpr usize MAXLEN = 200;      // < SN, leaves headroom for push/insert

// ───────────────────────────────────────────────────────────────────────────
// helpers
// ───────────────────────────────────────────────────────────────────────────

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

template<usize N, typename E>
static void
from_keys(micron::svector<E, N> &sv, mtest::ref_vec<SCAP> &r, const u64 *ks, usize n)
{
  for ( usize i = 0; i < n; ++i ) {
    E e = mtest::elem<E>::make(ks[i]);
    r.push_back(mtest::elem<E>::key(e));
    sv.push_back(e);
  }
}

// ───────────────────────────────────────────────────────────────────────────
// property groups at representative capacity SN, templated on element type
// ───────────────────────────────────────────────────────────────────────────

template<typename E>
static void
run_props(void)
{
  using S = micron::svector<E, SN>;

  // ── construction / fill / generator ────────────────────────────────────────
  test_case("sv ctor(n)/fill/generator vs oracle");
  {
    prng rng(0x3101u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_count(rng, n, MAXLEN);
      {
        S s(n);
        mtest::ref_vec<SCAP> r;
        r.resize(n);
        ck(mtest::vec_eq<E>(s, r), "ctor-n-zero", it);
        ck(s.size() == n, "ctor-n-size", it);
        ck(s.max_size() == SN, "max_size", it);
      }
      {
        u64 k = mtest::gen_raw(rng, band::small);
        E v = mtest::elem<E>::make(k);
        S s(n, v);
        mtest::ref_vec<SCAP> r;
        r.assign(n, mtest::elem<E>::key(v));
        ck(mtest::vec_eq<E>(s, r), "ctor-n-val", it);
      }
      {
        usize ctr = 0;
        auto fn = [&ctr]() { return mtest::elem<E>::make(static_cast<u64>(ctr++)); };
        S s(n, fn);
        mtest::ref_vec<SCAP> r;
        for ( usize i = 0; i < n; ++i ) r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(static_cast<u64>(i))));
        ck(mtest::vec_eq<E>(s, r), "ctor-generator", it);
      }
    }
  }
  end_test_case();

  // ── access [] / at / front / back / data ───────────────────────────────────
  test_case("sv access [] / at / front / back / data");
  {
    prng rng(0x3102u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      if ( n == 0 ) continue;
      S s;
      mtest::ref_vec<SCAP> r;
      from_keys<SN, E>(s, r, ks, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(mtest::elem<E>::key(s[idx]) == r.buf[idx], "op[]", it);
      ck(mtest::elem<E>::key(s.at(idx)) == r.buf[idx], "at", it);
      ck(mtest::elem<E>::key(s.front()) == r.buf[0], "front", it);
      ck(mtest::elem<E>::key(s.back()) == r.buf[n - 1], "back", it);
      const E *d = s.data();
      ck(mtest::elem<E>::key(d[idx]) == r.buf[idx], "data", it);
      // at() OOB throws runtime_error (operator[] is unchecked)
      expect_throw_type<micron::except::runtime_error>([&] { (void)s.at(n); });
    }
  }
  end_test_case();

  // ── push_back / emplace_back / move_back / append / pop_back / pop_front ────
  test_case("sv push/emplace/move_back/append/pop vs oracle");
  {
    prng rng(0x3103u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      S s;
      mtest::ref_vec<SCAP> r;
      from_keys<SN, E>(s, r, ks, n);
      // append a few, never exceeding capacity
      usize room = SN - s.size();
      usize k = static_cast<usize>(rng.next_in(8));
      if ( k > room ) k = room;
      for ( usize i = 0; i < k; ++i ) {
        u64 raw = mtest::gen_raw(rng, band::small);
        u64 op = rng.next() % 4;
        if ( op == 0 ) {
          E e = mtest::elem<E>::make(raw);
          s.push_back(e);
        } else if ( op == 1 ) {
          mtest::elem<E>::emplace(s, raw);      // svector emplace_back
        } else if ( op == 2 ) {
          E e = mtest::elem<E>::make(raw);
          s.move_back(micron::move(e));
        } else {
          E e = mtest::elem<E>::make(raw);
          s.append(e);      // append(const C&) == push_back
        }
        r.push_back(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
      }
      ck(mtest::vec_eq<E>(s, r), "push-family", it);
      // pop_back returns the removed value
      if ( !s.empty() ) {
        u64 ob = r.back();
        E got = s.pop_back();
        r.pop_back();
        ck(mtest::elem<E>::key(got) == ob, "pop_back-value", it);
        ck(mtest::vec_eq<E>(s, r), "pop_back", it);
      }
      // pop_front returns the front and shifts down
      if ( !s.empty() ) {
        u64 of = r.front();
        E got = s.pop_front();
        r.erase(0u);
        ck(mtest::elem<E>::key(got) == of, "pop_front-value", it);
        ck(mtest::vec_eq<E>(s, r), "pop_front", it);
      }
    }
  }
  end_test_case();

  // ── copy / move ctor + assign + self-assign + independence ──────────────────
  test_case("sv copy/move ctor+assign + independence");
  {
    prng rng(0x3104u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::full);
      gen_keys(rng, b, nb, MAXLEN, band::full);
      S sa;
      mtest::ref_vec<SCAP> ra;
      from_keys<SN, E>(sa, ra, a, na);
      // copy ctor + independence
      S c(sa);
      ck(mtest::vec_eq<E>(c, ra), "copy-ctor", it);
      if ( na ) {
        c[0] = mtest::elem<E>::make(~ra.buf[0]);
        ck(mtest::elem<E>::key(sa[0]) == ra.buf[0], "copy-indep", it);
      }
      // move ctor empties donor
      S fresh(sa);
      S m(micron::move(fresh));
      ck(mtest::vec_eq<E>(m, ra), "move-ctor", it);
      ck(fresh.size() == 0u, "move-ctor-donor", it);
      // copy-assign + self-assign
      S sb;
      mtest::ref_vec<SCAP> rb;
      from_keys<SN, E>(sb, rb, b, nb);
      sb = sa;
      ck(mtest::vec_eq<E>(sb, ra), "copy-assign", it);
      S &self = sb;
      sb = self;
      ck(mtest::vec_eq<E>(sb, ra), "self-copy-assign", it);
      // move-assign
      S src(sa);
      S dst;
      dst = micron::move(src);
      ck(mtest::vec_eq<E>(dst, ra), "move-assign", it);
      ck(src.size() == 0u, "move-assign-donor", it);
    }
  }
  end_test_case();

  // ── insert (index + iterator) / move_insert / emplace ──────────────────────
  test_case("sv insert/move_insert/emplace vs oracle");
  {
    prng rng(0x3105u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      // insert(ind, const C&) — ind in [0, n] (svector allows append position)
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        s.insert(idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(mtest::vec_eq<E>(s, r), "insert-idx", it);
      }
      // insert(iterator, const C&)
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        s.insert(s.begin() + idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(mtest::vec_eq<E>(s, r), "insert-it", it);
      }
      // move_insert(ind, T&&)
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        s.move_insert(idx, micron::move(e));
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(s, r), "move_insert-idx", it);
      }
      // emplace(ind, args)
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        u64 raw = mtest::gen_raw(rng, band::small);
        if constexpr ( same_t<E, mtest::big> )
          s.emplace(idx, raw, raw ^ 0x9e3779b97f4a7c15ull, ~raw);
        else if constexpr ( micron::is_integral_v<E> )
          s.emplace(idx, static_cast<E>(raw));
        else
          s.emplace(idx, static_cast<int>(raw & 0xffffffu));
        r.insert(idx, mtest::elem<E>::key(mtest::elem<E>::make(raw)));
        ck(mtest::vec_eq<E>(s, r), "emplace-idx", it);
      }
    }
  }
  end_test_case();

  // ── erase (index / range / iterator) ───────────────────────────────────────
  test_case("sv erase variants vs oracle");
  {
    prng rng(0x3106u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      if ( n == 0 ) continue;
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        s.erase(idx);
        r.erase(idx);
        ck(mtest::vec_eq<E>(s, r), "erase-idx", it);
      }
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n));
        s.erase(s.begin() + idx);
        r.erase(idx);
        ck(mtest::vec_eq<E>(s, r), "erase-it", it);
      }
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        s.erase(from, to);
        r.erase_range(from, to);
        ck(mtest::vec_eq<E>(s, r), "erase-range", it);
      }
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        usize from = static_cast<usize>(rng.next_in(n));
        usize to = from + 1 + static_cast<usize>(rng.next_in(n - from));
        s.erase(s.begin() + from, s.begin() + to);
        r.erase_range(from, to);
        ck(mtest::vec_eq<E>(s, r), "erase-it-range", it);
      }
    }
  }
  end_test_case();

  // ── fill / iteration / find / find_index / contains ────────────────────────
  test_case("sv fill/iterate/find/contains vs oracle");
  {
    prng rng(0x3107u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::small);
      S s;
      mtest::ref_vec<SCAP> r;
      from_keys<SN, E>(s, r, ks, n);
      // iteration
      usize idx = 0;
      bool ok = true;
      for ( auto i = s.begin(); i != s.end(); ++i, ++idx )
        if ( mtest::elem<E>::key(*i) != r.buf[idx] ) {
          ok = false;
          break;
        }
      ck(ok && idx == n, "begin/end", it);
      ck(s.cbegin() == s.begin() && s.cend() == s.end(), "cbegin/cend", it);
      // find returns end() on miss; find_index returns npos==N
      u64 fk = mtest::gen_raw(rng, band::small);
      usize oi = r.find(mtest::elem<E>::key(mtest::elem<E>::make(fk)));
      auto p = s.find(mtest::elem<E>::make(fk));
      usize fidx = s.find_index(mtest::elem<E>::make(fk));
      bool contains = s.contains(mtest::elem<E>::make(fk));
      if ( oi == mtest::VREF_NPOS ) {
        ck(p == s.end(), "find-miss", it);
        ck(fidx == S::npos, "find_index-miss", it);
        ck(!contains, "contains-miss", it);
      } else {
        ck(p != s.end() && static_cast<usize>(p - s.begin()) == oi, "find-hit", it);
        ck(fidx == oi, "find_index-hit", it);
        ck(contains, "contains-hit", it);
      }
      // fill sets every existing element (length unchanged)
      u64 vk = mtest::gen_raw(rng, band::small);
      s.fill(mtest::elem<E>::make(vk));
      r.fill_all(mtest::elem<E>::key(mtest::elem<E>::make(vk)));
      ck(mtest::vec_eq<E>(s, r), "fill", it);
    }
  }
  end_test_case();

  // ── append(svector) / operator+= / swap ────────────────────────────────────
  test_case("sv append(svector)/+=/swap vs oracle");
  {
    prng rng(0x3108u);
    u64 a[SN + 4], b[SN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      // bound combined length to SN so append() does not overflow
      usize na, nb;
      mtest::gen_count(rng, na, SN / 2);
      mtest::gen_count(rng, nb, SN - na);
      for ( usize i = 0; i < na; ++i ) a[i] = mtest::gen_raw(rng, band::small);
      for ( usize i = 0; i < nb; ++i ) b[i] = mtest::gen_raw(rng, band::small);
      {
        S sa;
        mtest::ref_vec<SCAP> ra;
        from_keys<SN, E>(sa, ra, a, na);
        S sb;
        mtest::ref_vec<SCAP> rb;
        from_keys<SN, E>(sb, rb, b, nb);
        sa.append(sb);
        ra.append(rb.buf, rb.len);
        ck(mtest::vec_eq<E>(sa, ra), "append", it);
        ck(mtest::vec_eq<E>(sb, rb), "append-src-intact", it);
      }
      {
        S sa;
        mtest::ref_vec<SCAP> ra;
        from_keys<SN, E>(sa, ra, a, na);
        S sb;
        mtest::ref_vec<SCAP> rb;
        from_keys<SN, E>(sb, rb, b, nb);
        sa += sb;
        ra.append(rb.buf, rb.len);
        ck(mtest::vec_eq<E>(sa, ra), "operator+=", it);
      }
      {
        S sa;
        mtest::ref_vec<SCAP> ra;
        from_keys<SN, E>(sa, ra, a, na);
        S sb;
        mtest::ref_vec<SCAP> rb;
        from_keys<SN, E>(sb, rb, b, nb);
        sa.swap(sb);
        ck(mtest::vec_eq<E>(sa, rb), "swap-a", it);
        ck(mtest::vec_eq<E>(sb, ra), "swap-b", it);
      }
    }
  }
  end_test_case();

  // ── clear / fast_clear ─────────────────────────────────────────────────────
  test_case("sv clear/fast_clear");
  {
    prng rng(0x3109u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        s.clear();
        ck(s.size() == 0u, "clear-size", it);
        ck(s.empty(), "clear-empty", it);
      }
      {
        S s;
        mtest::ref_vec<SCAP> r;
        from_keys<SN, E>(s, r, ks, n);
        s.fast_clear();
        ck(s.size() == 0u, "fast_clear-size", it);
      }
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// capacity / overflow behaviour at several N (per element type)
// ───────────────────────────────────────────────────────────────────────────

template<usize N, typename E>
static void
run_caps(void)
{
  using S = micron::svector<E, N>;

  test_case("sv full/overflow throws runtime_error");
  {
    S s;
    for ( usize i = 0; i < N; ++i ) s.push_back(mtest::elem<E>::make(i % 8u));
    require(s.size(), N);
    require(s.full(), true);
    require(s.full_or_overflowed(), true);
    require(s.overflowed(), false);
    // every grow op on a full svector throws runtime_error
    expect_throw_type<micron::except::runtime_error>([&] { s.push_back(mtest::elem<E>::make(1)); });
    expect_throw_type<micron::except::runtime_error>([&] { mtest::elem<E>::emplace(s, 1); });
    expect_throw_type<micron::except::runtime_error>([&] { (void)s.insert(static_cast<usize>(1), mtest::elem<E>::make(1)); });
    // at() OOB throws runtime_error
    expect_throw_type<micron::except::runtime_error>([&] { (void)s.at(N); });
  }
  end_test_case();

  test_case("sv ctor cnt too large throws library_error");
  {
    expect_throw_type<micron::except::library_error>([] { S bad(N + 1); });
  }
  end_test_case();

  test_case("sv erase / slice OOB throw");
  {
    S s;
    for ( usize i = 0; i < N / 2; ++i ) s.push_back(mtest::elem<E>::make(i % 8u));
    expect_throw_type<micron::except::runtime_error>([&] { (void)s.erase(s.size()); });
    expect_throw_type<micron::except::runtime_error>([&] { (void)s.erase(2u, 1u); });      // from>=to
    expect_throw_type<micron::except::runtime_error>([&] { (void)s.erase(0u, N); });       // to>length
    expect_throw_type<micron::except::library_error>([&] { (void)s.operator[](0u, N + 1); });
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// Tracked lifetime balance (stack vector: no allocator, just ctor/dtor balance)
// ───────────────────────────────────────────────────────────────────────────

static void
run_memory(void)
{
  using Tr = mtest::Tracked<8>;
  using S = micron::svector<Tr, 256>;

  test_case("sv Tracked lifetime balanced after churn");
  {
    Tr::reset();
    {
      prng rng(0x5CA1Eu);
      S s;
      for ( usize it = 0; it < 40000; ++it ) {
        u64 op = rng.next() % 6;
        if ( op == 0 ) {
          if ( !s.full() ) s.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
        } else if ( op == 1 ) {
          if ( !s.full() ) s.emplace_back(static_cast<int>(rng.next() & 0xffff));
        } else if ( op == 2 ) {
          if ( !s.empty() ) (void)s.pop_back();
        } else if ( op == 3 ) {
          if ( !s.empty() ) (void)s.erase(static_cast<usize>(rng.next() % s.size()));
        } else if ( op == 4 ) {
          if ( !s.full() ) s.insert(static_cast<usize>(rng.next() % (s.size() + 1)), Tr(static_cast<int>(rng.next() & 0xffff)));
        } else {
          s.clear();
        }
      }
    }
    ck(Tr::live() == 0u, "tracked-balance", 0);
  }
  end_test_case();

  test_case("sv copy deep + independent + balanced");
  {
    Tr::reset();
    {
      S a;
      for ( int i = 0; i < 40; ++i ) a.push_back(Tr(i));
      S c(a);
      c[0] = Tr(999);
      ck(a[0].v == 0, "copy-indep-src", 0);
      ck(c[0].v == 999, "copy-indep-dst", 0);
    }
    ck(Tr::live() == 0u, "copy-balance", 0);
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// nested: svector<vector<u32>, N> — a stack array of resource-owning elements
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

template<typename SVV>
static bool
nested_eq(const SVV &sv, const mtest::ref_vec<SCAP> &r)
{
  if ( static_cast<usize>(sv.size()) != r.len ) return false;
  for ( usize i = 0; i < r.len; ++i )
    if ( rowkey(sv[i]) != r.buf[i] ) return false;
  return true;
}

static void
run_nested(void)
{
  using V = micron::vector<u32>;
  using SVV = micron::svector<V, 32>;

  test_case("svector<vector<u32>,32> nested ops vs oracle");
  {
    prng rng(0x9999u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(20));      // <= 20 < 32
      SVV sv;
      mtest::ref_vec<SCAP> r;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        r.push_back(rowkey(row));
        sv.move_back(micron::move(row));
      }
      ck(nested_eq(sv, r), "build", it);
      {
        SVV cp(sv);
        ck(nested_eq(cp, r), "copy", it);
        if ( n ) {
          cp[0].push_back(7u);
          ck(rowkey(sv[0]) == r.buf[0], "copy-indep", it);
        }
      }
      if ( sv.size() < 31 ) {
        usize idx = static_cast<usize>(rng.next_in(sv.size() + 1));
        V row = mkrow(rng);
        u64 k = rowkey(row);
        sv.move_insert(idx, micron::move(row));
        r.insert(idx, k);
        ck(nested_eq(sv, r), "move_insert", it);
      }
      if ( sv.size() ) {
        usize idx = static_cast<usize>(rng.next_in(sv.size()));
        sv.erase(idx);
        r.erase(idx);
        ck(nested_eq(sv, r), "erase", it);
      }
      if ( sv.size() ) {
        u64 ob = r.back();
        V got = sv.pop_back();
        r.pop_back();
        ck(rowkey(got) == ob, "pop_back-value", it);
        ck(nested_eq(sv, r), "pop_back", it);
      }
    }
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
  sb::print("--- svector<", tyname, ", 256> ---");
  run_props<E>();
  run_caps<8, E>();
  run_caps<16, E>();
  run_caps<64, E>();
  run_caps<256, E>();
}

int
main(int, char **)
{
  sb::print("=== SVECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- memory / lifetime (Tracked) ---");
  run_memory();

  sb::print("--- nested containers (svector<vector<...>>) ---");
  run_nested();

  sb::print("=== SVECTOR RIGOR DONE ===");
  return 1;
}
