// Exhaustive, adversarial rigor suite for micron::vector<T, Alloc, Sf>.
//
// vector is a heap-allocated, growable, mutable container with two
// specializations: the copyable one (regular T) and a move-only one
// (requires !is_regular_object<T> && movable<T> && !copyable<T>). This suite
// drives every member function from an adversarial standpoint: each group pairs
// a handful of edge cases with a >=10k-iteration randomized property loop that
// diffs the vector against the obviously-correct ref_vec oracle
// (tests/support/vector_rigor.hpp), plus out-of-range cases that must throw
// library_error and heavy object-lifetime / memory-correctness coverage.
//
// Element-type coverage is exhaustive: run_props<E>() is instantiated for the
// POD widths u8/u16/u32/u64, a 24-byte POD (big) and a non-trivial Tracked.
// The move-only specialization is exercised through vector<move_only>. Memory:
// deep-copy independence, move emptying, self-assign, allocator leak-freeness
// (tracking_allocator), exception safety mid-grow (throwing_allocator) and on a
// throwing element copy (Throwing), and Tracked ctor/dtor balance.
//
// Build: `duck build tests/rigor/rigor_vector.cpp`; run `bin/rigor_vector`.

#include "../../src/io/console.hpp"

#include "../../src/vector/vector.hpp"

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
template<typename E> inline constexpr bool orderable = micron::is_integral_v<E> || same_t<E, mtest::big> || same_t<E, mtest::move_only>;

static void
gen_keys(prng &rng, u64 *ks, usize &n, usize maxlen, band b)
{
  mtest::gen_count(rng, n, maxlen);
  for ( usize i = 0; i < n; ++i ) ks[i] = mtest::gen_raw(rng, b);
}

template<typename E, typename V>
static void
from_keys(V &v, mtest::ref_vec<CAP> &r, const u64 *ks, usize n)
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
  using V = micron::vector<E>;

  test_case("vec ctor(n)/fill/initlist/generator vs oracle");
  {
    prng rng(0x1001u);
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
    test_case("vec initializer_list ctor");
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

  test_case("vec push/emplace/move_back/pop_back fuzz vs oracle");
  {
    prng rng(0x1002u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      ck(mtest::vec_eq<E>(v, r), "build", it);

      {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::small));
        u64 k = mtest::elem<E>::key(e);
        v.push_back(e);
        r.push_back(k);
        ck(mtest::vec_eq<E>(v, r), "push_back-copy", it);
      }

      {
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.push_back(micron::move(e));
        r.push_back(k);
        ck(mtest::vec_eq<E>(v, r), "push_back-move", it);
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

      usize p = static_cast<usize>(rng.next_in(8));
      for ( usize i = 0; i < p && !v.empty(); ++i ) {
        v.pop_back();
        r.pop_back();
      }
      ck(mtest::vec_eq<E>(v, r), "pop_back", it);
    }
  }
  end_test_case();

  test_case("vec copy/move ctor + independence");
  {
    prng rng(0x1003u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);

      V c(v);
      ck(mtest::vec_eq<E>(c, r), "copy-ctor", it);
      if ( n ) {
        c[0] = mtest::elem<E>::make(~r.buf[0]);
        ck(mtest::elem<E>::key(v[0]) == r.buf[0], "copy-indep", it);
      }

      V fresh(v);
      V m(micron::move(fresh));
      ck(mtest::vec_eq<E>(m, r), "move-ctor", it);
      ck(fresh.size() == 0u, "move-ctor-donor", it);
    }
  }
  end_test_case();

  test_case("vec copy/move assign + self-assign vs oracle");
  {
    prng rng(0x1004u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::full);
      gen_keys(rng, b, nb, MAXLEN, band::full);
      V va;
      mtest::ref_vec<CAP> ra;
      from_keys<E>(va, ra, a, na);
      V vb;
      mtest::ref_vec<CAP> rb;
      from_keys<E>(vb, rb, b, nb);

      va = vb;
      ck(mtest::vec_eq<E>(va, rb), "copy-assign", it);
      ck(mtest::vec_eq<E>(vb, rb), "copy-assign-src", it);

      V &self = va;
      va = self;
      ck(mtest::vec_eq<E>(va, rb), "self-copy-assign", it);

      V vc;
      mtest::ref_vec<CAP> rc;
      from_keys<E>(vc, rc, a, na);
      va = micron::move(vc);
      ck(mtest::vec_eq<E>(va, ra), "move-assign", it);
      ck(vc.size() == 0u, "move-assign-donor", it);
    }
  }
  end_test_case();

  test_case("vec access [] / at / front / back / data");
  {
    prng rng(0x1005u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      if ( n == 0 ) continue;
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(mtest::elem<E>::key(v[idx]) == r.buf[idx], "op[]", it);
      ck(mtest::elem<E>::key(v.at(idx)) == r.buf[idx], "at", it);
      ck(mtest::elem<E>::key(v.front()) == r.buf[0], "front", it);
      ck(mtest::elem<E>::key(v.back()) == r.buf[n - 1], "back", it);
      const E *d = v.data();
      ck(mtest::elem<E>::key(d[idx]) == r.buf[idx], "data", it);

      expect_throw_type<micron::except::library_error>([&] { (void)v.at(n); });
    }
  }
  end_test_case();

  test_case("vec capacity reserve/resize vs oracle");
  {
    prng rng(0x1006u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      ck(v.size() == n, "size", it);
      ck(v.empty() == (n == 0), "empty", it);
      ck(v.max_size() >= n, "max_size>=size", it);

      usize cap = n + static_cast<usize>(rng.next_in(600)) + 1;
      v.reserve(cap);
      ck(v.size() == n, "reserve-keeps-size", it);
      ck(mtest::vec_eq<E>(v, r), "reserve-keeps-content", it);
      ck(v.max_size() >= cap, "reserve-cap", it);

      usize g = n + static_cast<usize>(rng.next_in(200));
      v.resize(g);
      r.resize(g);
      ck(mtest::vec_eq<E>(v, r), "resize-grow", it);

      usize s = static_cast<usize>(rng.next_in(g + 1));
      v.resize(s);
      r.resize(s);
      ck(mtest::vec_eq<E>(v, r), "resize-shrink", it);

      u64 fk = mtest::gen_raw(rng, band::small);
      usize g2 = v.size() + static_cast<usize>(rng.next_in(100));
      v.resize(g2, mtest::elem<E>::make(fk));
      r.resize(g2, mtest::elem<E>::key(mtest::elem<E>::make(fk)));
      ck(mtest::vec_eq<E>(v, r), "resize-val", it);
    }
  }
  end_test_case();

  test_case("vec iteration + find vs oracle");
  {
    prng rng(0x1007u);
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
      if ( n ) ck(mtest::elem<E>::key(*v.last()) == r.buf[n - 1], "last", it);

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

  test_case("vec insert(index, ...) vs oracle");
  {
    prng rng(0x1008u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = n ? static_cast<usize>(rng.next_in(n)) : 0;
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        v.insert(idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(mtest::vec_eq<E>(v, r), "insert-idx", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = n ? static_cast<usize>(rng.next_in(n)) : 0;
        usize cnt = static_cast<usize>(rng.next_in(6)) + 1;
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        v.insert(idx, e, cnt);
        r.insert(idx, mtest::elem<E>::key(e), cnt);
        ck(mtest::vec_eq<E>(v, r), "insert-idx-cnt", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = n ? static_cast<usize>(rng.next_in(n)) : 0;
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.insert(idx, micron::move(e));
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-idx-move", it);
      }
    }
  }
  end_test_case();

  test_case("vec insert(iterator, ...) vs oracle");
  {
    prng rng(0x1009u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        v.insert(v.begin() + idx, e);
        r.insert(idx, mtest::elem<E>::key(e));
        ck(mtest::vec_eq<E>(v, r), "insert-it", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        usize cnt = static_cast<usize>(rng.next_in(6)) + 1;
        u64 k = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(k);
        v.insert(v.begin() + idx, e, cnt);
        r.insert(idx, mtest::elem<E>::key(e), cnt);
        ck(mtest::vec_eq<E>(v, r), "insert-it-cnt", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, ks, n);
        usize m = static_cast<usize>(rng.next_in(6));
        E src[8];
        u64 sk[8];
        for ( usize i = 0; i < m; ++i ) {
          u64 raw = mtest::gen_raw(rng, band::small);
          src[i] = mtest::elem<E>::make(raw);
          sk[i] = mtest::elem<E>::key(src[i]);
        }
        usize idx = static_cast<usize>(rng.next_in(n + 1));
        v.insert(v.begin() + idx, src, src + m);
        r.insert_seq(idx, sk, m);
        ck(mtest::vec_eq<E>(v, r), "insert-it-range", it);
      }
    }
  }
  end_test_case();

  test_case("vec erase variants vs oracle");
  {
    prng rng(0x100Au);
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

  test_case("vec remove vs oracle");
  {
    prng rng(0x100Bu);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 200, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      u64 raw = mtest::gen_raw(rng, band::small);
      v.remove(mtest::elem<E>::make(raw));
      r.remove_val(mtest::elem<E>::key(mtest::elem<E>::make(raw)));
      ck(mtest::vec_eq<E>(v, r), "remove", it);
    }
  }
  end_test_case();

  test_case("vec assign/append/weld/swap/clone vs oracle");
  {
    prng rng(0x100Cu);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, MAXLEN, band::small);
      gen_keys(rng, b, nb, MAXLEN, band::small);

      {
        usize cnt = static_cast<usize>(rng.next_in(200));
        u64 k = mtest::gen_raw(rng, band::small);
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
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
        mtest::ref_vec<CAP> rv;
        from_keys<E>(v, rv, a, na);
        V cl = v.clone();
        ck(mtest::vec_eq<E>(cl, rv), "clone", it);
        if ( na ) {
          cl[0] = mtest::elem<E>::make(~rv.buf[0]);
          ck(mtest::elem<E>::key(v[0]) == rv.buf[0], "clone-indep", it);
        }
      }
    }
  }
  end_test_case();

  if constexpr ( orderable<E> ) {
    test_case("vec sort/insert_sort vs oracle");
    {
      prng rng(0x100Du);
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

  test_case("vec fill/into_bytes");
  {
    prng rng(0x100Eu);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 200, band::full);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);

      u64 k = mtest::gen_raw(rng, band::small);
      v.fill(mtest::elem<E>::make(k));
      r.fill_all(mtest::elem<E>::key(mtest::elem<E>::make(k)));
      ck(mtest::vec_eq<E>(v, r), "fill", it);
      auto bs = v.into_bytes();
      (void)bs;
    }
  }
  end_test_case();
}

template<typename E>
static void
run_boundaries(void)
{
  using V = micron::vector<E>;
  test_case("vec boundary-length build/copy vs oracle");
  {
    prng rng(0xB0D5u);
    for ( usize li = 0; li < mtest::kVecLensCount; ++li ) {
      usize L = mtest::kVecLens[li];
      if ( L > CAP - 1 ) continue;
      V v;
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < L; ++i ) {
        E e = mtest::elem<E>::make(mtest::gen_raw(rng, band::full));
        r.push_back(mtest::elem<E>::key(e));
        v.push_back(micron::move(e));
      }
      ck(mtest::vec_eq<E>(v, r), "boundary-build", L);
      V c(v);
      ck(mtest::vec_eq<E>(c, r), "boundary-copy", L);
    }
  }
  end_test_case();
}

static void
run_move_only(void)
{
  using E = mtest::move_only;
  using V = micron::vector<E>;

  static_assert(!micron::is_copy_constructible_v<V>, "vector<move_only> must be move-only");

  test_case("vec(move-only) build/move-ctor/move-assign vs oracle");
  {
    prng rng(0x2001u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, MAXLEN, band::full);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      ck(mtest::vec_eq<E>(v, r), "build", it);

      V m(micron::move(v));
      ck(mtest::vec_eq<E>(m, r), "move-ctor", it);
      ck(v.size() == 0u, "move-ctor-donor", it);

      V t;
      mtest::ref_vec<CAP> rt;
      from_keys<E>(t, rt, ks, n ? n / 2 : 0);
      t = micron::move(m);
      ck(mtest::vec_eq<E>(t, r), "move-assign", it);
      ck(m.size() == 0u, "move-assign-donor", it);
    }
  }
  end_test_case();

  test_case("vec(move-only) push/emplace/move_back/pop/access vs oracle");
  {
    prng rng(0x2002u);
    u64 ks[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      gen_keys(rng, ks, n, 150, band::small);
      V v;
      mtest::ref_vec<CAP> r;
      from_keys<E>(v, r, ks, n);
      {
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.push_back(micron::move(e));
        r.push_back(k);
        ck(mtest::vec_eq<E>(v, r), "push_back-move", it);
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

  test_case("vec(move-only) reserve/resize grow+shrink vs oracle");
  {
    prng rng(0x2004u);
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

  test_case("vec(move-only) insert/erase/weld/swap vs oracle");
  {
    prng rng(0x2003u);
    u64 a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      gen_keys(rng, a, na, 150, band::small);
      gen_keys(rng, b, nb, 150, band::small);

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        usize idx = na ? static_cast<usize>(rng.next_in(na)) : 0;
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.insert(idx, micron::move(e));
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-idx-move", it);
      }

      {
        V v;
        mtest::ref_vec<CAP> r;
        from_keys<E>(v, r, a, na);
        usize idx = static_cast<usize>(rng.next_in(na + 1));
        u64 raw = mtest::gen_raw(rng, band::small);
        E e = mtest::elem<E>::make(raw);
        u64 k = mtest::elem<E>::key(e);
        v.insert(v.begin() + idx, micron::move(e));
        r.insert(idx, k);
        ck(mtest::vec_eq<E>(v, r), "insert-it-move", it);
      }

      if ( na ) {
        {
          V v;
          mtest::ref_vec<CAP> r;
          from_keys<E>(v, r, a, na);
          usize idx = static_cast<usize>(rng.next_in(na));
          v.erase(idx);
          r.erase(idx);
          ck(mtest::vec_eq<E>(v, r), "erase-idx", it);
        }
        {
          V v;
          mtest::ref_vec<CAP> r;
          from_keys<E>(v, r, a, na);
          usize from = static_cast<usize>(rng.next_in(na));
          usize to = from + 1 + static_cast<usize>(rng.next_in(na - from));
          v.erase(v.begin() + from, v.begin() + to);
          r.erase_range(from, to);
          ck(mtest::vec_eq<E>(v, r), "erase-it-range", it);
        }
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
    }
  }
  end_test_case();
}

template<typename VA, typename TrkA>
static void
churn(prng &rng, VA &v, usize iters)
{
  using Tr = typename VA::value_type;
  for ( usize it = 0; it < iters; ++it ) {
    u64 op = rng.next() % 6;
    if ( op == 0 )
      v.push_back(Tr(static_cast<int>(rng.next() & 0xffff)));
    else if ( op == 1 )
      v.emplace_back(static_cast<int>(rng.next() & 0xffff));
    else if ( op == 2 ) {
      if ( !v.empty() ) v.pop_back();
    } else if ( op == 3 ) {
      if ( v.size() > 2 ) v.erase(static_cast<usize>(rng.next() % v.size()));
    } else if ( op == 4 ) {
      if ( rng.next() & 1u )
        v.clear();
      else
        v.resize(static_cast<usize>(rng.next() % 64));
    } else {
      if ( v.size() > 1 ) {
        usize idx = static_cast<usize>(rng.next() % v.size());
        v.insert(idx, Tr(static_cast<int>(rng.next() & 0xffff)));
      }
    }
  }
  (void)sizeof(TrkA);
}

static void
run_memory(void)
{
  using Tr = mtest::Tracked<5>;
  using TA = mtest::tracking_allocator<5>;
  using VA = micron::vector<Tr, TA>;

  test_case("vec churn leak-free (tracking allocator)");
  {
    expect_leak_free<TA>([] {
      prng rng(0xBEEF01u);
      VA v;
      churn<VA, TA>(rng, v, 20000);
    });
  }
  end_test_case();

  test_case("vec Tracked lifetime balanced after churn");
  {
    Tr::reset();
    {
      prng rng(0xBEEF02u);
      VA v;
      churn<VA, TA>(rng, v, 20000);
    }
    ck(Tr::live() == 0u, "tracked-balance", 0);
  }
  end_test_case();

  test_case("vec move-assign frees dest + empties donor (no leak)");
  {
    TA::reset();
    Tr::reset();
    {
      VA a;
      for ( int i = 0; i < 60; ++i ) a.push_back(Tr(i));
      VA b;
      for ( int i = 0; i < 60; ++i ) b.push_back(Tr(i + 1000));
      a = micron::move(b);
      ck(b.size() == 0u, "move-assign-donor-empty", 0);
      ck(a.size() == 60u, "move-assign-size", 0);
    }
    ck(TA::outstanding() == 0, "move-assign-no-leak", 0);
    ck(Tr::live() == 0u, "move-assign-tracked-balance", 0);
  }
  end_test_case();

  test_case("vec copy deep + independent + balanced");
  {
    Tr::reset();
    {
      VA a;
      for ( int i = 0; i < 40; ++i ) a.push_back(Tr(i));
      VA c(a);
      c[0] = Tr(999);
      ck(a[0].v == 0, "copy-indep-src", 0);
      ck(c[0].v == 999, "copy-indep-dst", 0);
      ck(a.size() == c.size(), "copy-size", 0);
    }
    ck(Tr::live() == 0u, "copy-balance", 0);
  }
  end_test_case();

  test_case("vec throwing allocator mid-grow: throws + no leak");
  {
    using THA = mtest::throwing_allocator<6>;
    using VT = micron::vector<u32, THA>;
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

  test_case("vec element copy-ctor throw: source intact, balanced");
  {
    using Th = mtest::Throwing<mtest::throw_on::copy_ctor, 3>;
    Th::reset();
    {
      micron::vector<Th> v;
      for ( int i = 0; i < 10; ++i ) v.move_back(Th(i));
      Th::arm(3);
      bool threw = false;
      try {
        micron::vector<Th> c(v);
        (void)c;
      } catch ( ... ) {
        threw = true;
      }
      Th::disarm();
      ck(threw, "copy-threw", 0);
      ck(v.size() == 10u, "src-intact", 0);
    }
    ck(Th::ctor == Th::dtor, "throwing-balance", 0);
  }
  end_test_case();
}

static void
run_adversarial(void)
{
  using V = micron::vector<u32>;

  test_case("vec OOB at()/insert/erase throw library_error");
  {
    V v;
    for ( u32 i = 0; i < 6u; ++i ) v.push_back(i);
    expect_throw_type<micron::except::library_error>([&] { (void)v.at(6); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.at(100); });
    expect_throw_type<micron::except::library_error>([&] { v.erase(6u); });
    expect_throw_type<micron::except::library_error>([&] { v.erase(2u, 2u); });
    expect_throw_type<micron::except::library_error>([&] { v.erase(0u, 100u); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.insert(6u, 99u); });
  }
  end_test_case();

  test_case("vec front/back/last/pop on empty throw");
  {
    V v;
    expect_throw_type<micron::except::library_error>([&] { (void)v.front(); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.back(); });
    expect_throw_type<micron::except::library_error>([&] { (void)v.last(); });
    expect_throw_type<micron::except::library_error>([&] { v.pop_back(); });
  }
  end_test_case();

  test_case("vec empty-vector operations are well-defined");
  {
    V v;
    require(v.size(), 0u);
    require(v.empty(), true);
    require(v.find(7u) == nullptr, true);
    require(v.begin() == v.end(), true);
    V z(0);
    require(z.size(), 0u);
  }
  end_test_case();
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
  using VV = micron::vector<V>;

  test_case("vector<vector<u32>> full ops vs oracle");
  {
    prng rng(0x7777u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(40));
      VV vv;
      mtest::ref_vec<CAP> r;
      for ( usize i = 0; i < n; ++i ) {
        V row = mkrow(rng);
        r.push_back(rowkey(row));
        vv.push_back(micron::move(row));
      }
      ck(nested_eq(vv, r), "build", it);

      {
        VV cp(vv);
        ck(nested_eq(cp, r), "copy", it);
        if ( n ) {
          cp[0].push_back(0xABCDu);
          ck(rowkey(vv[0]) == r.buf[0], "copy-indep", it);
        }
      }

      if ( n ) {
        usize idx = static_cast<usize>(rng.next_in(n));
        V row = mkrow(rng);
        u64 k = rowkey(row);
        vv.insert(idx, micron::move(row));
        r.insert(idx, k);
        ck(nested_eq(vv, r), "insert-idx", it);
      }
      {
        V row = mkrow(rng);
        u64 k = rowkey(row);
        vv.insert(vv.end(), micron::move(row));
        r.insert(r.len, k);
        ck(nested_eq(vv, r), "insert-end", it);
      }

      if ( vv.size() ) {
        usize idx = static_cast<usize>(rng.next_in(vv.size()));
        vv.erase(idx);
        r.erase(idx);
        ck(nested_eq(vv, r), "erase-idx", it);
      }
      if ( vv.size() >= 2 ) {
        usize sz = vv.size();
        usize from = static_cast<usize>(rng.next_in(sz));
        usize to = from + 1 + static_cast<usize>(rng.next_in(sz - from));
        vv.erase(from, to);
        r.erase_range(from, to);
        ck(nested_eq(vv, r), "erase-range", it);
      }

      {
        usize s = vv.size() ? static_cast<usize>(rng.next_in(vv.size() + 1)) : 0;
        vv.resize(s);
        r.resize(s);
        ck(nested_eq(vv, r), "resize-shrink", it);
      }

      if ( vv.size() ) {
        V row = mkrow(rng);
        u64 k = rowkey(row);
        vv.fill(row);
        r.fill_all(k);
        ck(nested_eq(vv, r), "fill", it);
      }
    }
  }
  end_test_case();

  test_case("vector<vector<vector<u32>>> triple-nest copy independence");
  {
    using VVV = micron::vector<VV>;
    VVV t;
    for ( u32 i = 0; i < 20u; ++i ) {
      VV mid;
      for ( u32 j = 0; j <= i % 5u; ++j ) {
        V leaf;
        for ( u32 k = 0; k <= j; ++k ) leaf.push_back(i * 100u + j * 10u + k);
        mid.push_back(micron::move(leaf));
      }
      t.push_back(micron::move(mid));
    }
    require(t.size(), 20u);
    VVV t2(t);
    require(t2.size(), 20u);

    t2[5].push_back(V{});
    t2[10][0].push_back(99999u);

    require(t[10][0].size(), 1u);
    require(t[10][0][0], 1000u);
    require(t[19].size(), 5u);
    sb::print("triple-nest OK");
  }
  end_test_case();
}

template<typename E>
static void
run_all(const char *tyname)
{
  sb::print("--- vector<", tyname, "> ---");
  run_props<E>();
  run_boundaries<E>();
}

int
main(int, char **)
{
  sb::print("=== VECTOR RIGOR ===");

  run_all<u8>("u8");
  run_all<u16>("u16");
  run_all<u32>("u32");
  run_all<u64>("u64");
  run_all<mtest::big>("big");
  run_all<mtest::Tracked<0>>("Tracked");

  sb::print("--- vector<move_only> (move-only specialization) ---");
  run_move_only();

  sb::print("--- memory / lifetime / exception (Tracked + allocators) ---");
  run_memory();

  sb::print("--- adversarial / edges ---");
  run_adversarial();

  sb::print("--- nested containers (vector<vector<...>>) ---");
  run_nested();

  sb::print("=== VECTOR RIGOR DONE ===");
  return 1;
}
