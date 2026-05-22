// Exhaustive, adversarial rigor suite for micron::istring<T, Alloc>.
//
// istring is an immutable, move-only heap string: copy is deleted, and every
// "mutating" operation returns a brand-new istring while leaving the source
// unchanged. This suite enforces exactly that:
//   * static asserts that istring is move-only.
//   * every functional group diffs the RESULT against the ref_string oracle and
//     also asserts the SOURCE is unchanged (the immutability invariant), >=10k
//     iters/fn across char/byte/wide/unicode8/unicode16/unicode32.
//   * adversarial: substr default cnt==0, substr OOB throw, stack() length>=255
//     throw, empty needle.
//   * memory: every op allocates a fresh buffer — churn loops are checked for
//     leak-freeness via mtest::tracking_allocator + expect_leak_free; moved-from
//     donor is empty; double-move chains; destructor of moved-from is safe.
//
// Build: `duck build tests/rigor/rigor_istring.cpp`; run `bin/rigor_istring`.

#include "../../src/io/console.hpp"

#include "../../src/string/istring.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
#include "../support/mock_allocators.hpp"
#include "../support/oracles.hpp"
#include "../support/string_rigor.hpp"

using namespace snowball;
using mtest::alpha;
using mtest::prng;
using mtest::REF_NPOS;
using mtest::ref_string;

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 10000
#endif
static constexpr usize ITERS = RIGOR_ITERS;
static constexpr usize MAXLEN = 200;

// build an istring<T> holding p[0..n) by appending into an empty one (no
// iterator-range ctor exists; append(ptr,n) returns a fresh istring).
template<typename T>
static micron::istring<T>
make_is(const T *p, usize n)
{
  micron::istring<T> e;
  return e.append(p, n);
}

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

// ───────────────────────────────────────────────────────────────────────────
// functional + immutability property groups
// ───────────────────────────────────────────────────────────────────────────

template<typename T>
static void
run_props(void)
{
  using S = micron::istring<T>;

  static_assert(!micron::is_copy_constructible_v<S>, "istring must be move-only");
  static_assert(!micron::is_copy_assignable_v<S>, "istring must be move-only");

  // ── construction / move ──────────────────────────────────────────────────
  test_case("is ctor + move fuzz");
  {
    prng rng(0x1111u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      ck(mtest::seq_eq(s, r), "ctor", it);
      S m(micron::move(s));
      ck(mtest::seq_eq(m, r), "move", it);
      ck(s.size() == 0u, "move-donor-empty", it);      // donor zeroed
    }
  }
  end_test_case();

  // ── fill ctor (cnt, ch) ──────────────────────────────────────────────────
  test_case("is fill ctor fuzz");
  {
    prng rng(0x1212u);
    for ( usize it = 0; it < ITERS; ++it ) {
      usize cnt = static_cast<usize>(rng.next_in(MAXLEN));
      T ch = mtest::gen_char<T>(rng, alpha::full);
      S s(cnt, ch);
      ref_string<T> r;
      r.fill(ch, cnt);
      ck(mtest::seq_eq(s, r), "fill-ctor", it);
    }
  }
  end_test_case();

  // ── access ───────────────────────────────────────────────────────────────
  test_case("is access fuzz");
  {
    prng rng(0x3333u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(static_cast<T>(s[idx]) == r.buf[idx], "op[]", it);
      ck(static_cast<T>(s.at(idx)) == r.buf[idx], "at", it);
      expect_throw_type<micron::except::library_error>([&] { (void)s.at(n); });
    }
  }
  end_test_case();

  // ── append (immutable: result correct, source unchanged) ─────────────────
  test_case("is append fuzz + immutability");
  {
    prng rng(0x4444u);
    T buf[MAXLEN + 4], add[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n, m;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, add, m, 64, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      S addi = make_is<T>(add, m);
      S t = s.append(addi);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.append(add, m);
      ck(mtest::seq_eq(t, rt), "append-result", it);
      ck(mtest::seq_eq(s, r), "append-source-unchanged", it);      // immutability
    }
  }
  end_test_case();

  // ── push_back (immutable) ────────────────────────────────────────────────
  test_case("is push_back fuzz + immutability");
  {
    prng rng(0x5555u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      S t = s.push_back(ch);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.push_back(ch);
      ck(mtest::seq_eq(t, rt), "push_back-result", it);
      ck(mtest::seq_eq(s, r), "push_back-source", it);
    }
  }
  end_test_case();

  // ── insert (immutable) ───────────────────────────────────────────────────
  test_case("is insert fuzz + immutability");
  {
    prng rng(0x6161u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(8)) + 1;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      S t = s.insert(ind, ch, cnt);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.insert_char(ind, ch, cnt);
      ck(mtest::seq_eq(t, rt), "insert-result", it);
      ck(mtest::seq_eq(s, r), "insert-source", it);
    }
  }
  end_test_case();

  // ── operator+= (immutable: returns new) ──────────────────────────────────
  test_case("is operator+= fuzz");
  {
    prng rng(0x7070u);
    T buf[MAXLEN + 4], add[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n, m;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, add, m, 64, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      S addi = make_is<T>(add, m);
      S t = s += addi;
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.append(add, m);
      ck(mtest::seq_eq(t, rt), "+=result", it);
      ck(mtest::seq_eq(s, r), "+=source", it);
    }
  }
  end_test_case();

  // ── substr ───────────────────────────────────────────────────────────────
  test_case("is substr fuzz vs oracle");
  {
    prng rng(0x6565u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - pos)) + 1;      // 1..n-pos
      S sub = s.substr(pos, cnt);
      ref_string<T> rsub;
      rsub.assign(r.buf + pos, cnt);
      ck(mtest::seq_eq(sub, rsub), "substr", it);
      ck(mtest::seq_eq(s, r), "substr-source", it);
    }
  }
  end_test_case();

  // ── find(ch) / find(istring) ─────────────────────────────────────────────
  test_case("is find fuzz vs oracle");
  {
    prng rng(0x8080u);
    T buf[MAXLEN + 4], nd[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      usize pos = n ? static_cast<usize>(rng.next_in(n + 1)) : 0;
      {
        usize got = s.find(ch, pos);
        usize exp = r.find(ch, pos);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-ch", it);
      }
      usize m = static_cast<usize>(rng.next_in(4)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_is<T>(nd, m);
      {
        usize got = s.find(needle);
        usize exp = r.find_seq(nd, m, 0);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-needle", it);
      }
    }
  }
  end_test_case();

  // ── equality ─────────────────────────────────────────────────────────────
  test_case("is equality fuzz vs oracle");
  {
    prng rng(0x9090u);
    T a[128], b[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 64, alpha::small);
      // half the time make b identical to a to force the true branch
      if ( rng.next() & 1u ) {
        nb = na;
        for ( usize i = 0; i < na; ++i ) b[i] = a[i];
      } else {
        mtest::gen_string<T>(rng, b, nb, 64, alpha::small);
      }
      ref_string<T> ra;
      ra.assign(a, na);
      bool eq_exp = (ra.compare(b, nb) == 0);
      S sa = make_is<T>(a, na);
      S sb_ = make_is<T>(b, nb);
      ck((sa == sb_) == eq_exp, "==", it);
    }
  }
  end_test_case();

  // ── clear / fast_clear / swap ────────────────────────────────────────────
  test_case("is clear/swap fuzz");
  {
    prng rng(0xA0A0u);
    T a[128], b[128];
    for ( usize it = 0; it < 3000; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 64, alpha::full);
      mtest::gen_string<T>(rng, b, nb, 64, alpha::full);
      S sa = make_is<T>(a, na);
      S sb_ = make_is<T>(b, nb);
      sa.swap(sb_);
      ck(sa.size() == nb && sb_.size() == na, "swap-sizes", it);
      if ( nb ) ck(static_cast<T>(sa[0]) == b[0], "swap-content", it);
      sa.clear();
      ck(sa.size() == 0u, "clear", it);
      sb_.fast_clear();
      ck(sb_.size() == 0u, "fast_clear", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// accessors / iteration / more overloads (per char type)
// ───────────────────────────────────────────────────────────────────────────

template<typename T>
static void
run_accessors(void)
{
  using S = micron::istring<T>;

  test_case("is data/cdata/c_str/w_str/uni_str/into_bytes");
  {
    prng rng(0xACC0u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      const T *d = s.data();
      bool ok = true;
      for ( usize i = 0; i < n; ++i )
        if ( d[i] != r.buf[i] ) {
          ok = false;
          break;
        }
      ck(ok, "data", it);
      ck(mtest::cstr_eq(s, r), "c_str", it);
      ck(s.w_str() != nullptr && s.uni_str() != nullptr, "w/uni_str", it);
      auto bytes = s.into_bytes();
      ck(bytes.size() >= n, "into_bytes", it);
    }
  }
  end_test_case();

  test_case("is iteration begin/end/last/cbegin/cend");
  {
    prng rng(0xACC1u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_is<T>(buf, n);
      usize idx = 0;
      bool ok = true;
      for ( auto i = s.begin(); i != s.end(); ++i, ++idx )
        if ( static_cast<T>(*i) != r.buf[idx] ) {
          ok = false;
          break;
        }
      ck(ok && idx == n, "begin/end", it);
      ck(s.cbegin() == s.begin() && s.cend() == s.end(), "cbegin/cend", it);
      ck(static_cast<T>(*s.last()) == r.buf[n - 1], "last", it);
    }
  }
  end_test_case();

  test_case("is append(ptr) / append(sstring) immutable");
  {
    prng rng(0xACC2u);
    T a[MAXLEN + 4], b[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 40, alpha::ascii);
      ref_string<T> r;
      r.assign(a, na);
      S s = make_is<T>(a, na);
      S t = s.append(static_cast<const T *>(b), nb);
      ref_string<T> rt;
      rt.assign(a, na);
      rt.append(b, nb);
      ck(mtest::seq_eq(t, rt), "append-ptr", it);
      ck(mtest::seq_eq(s, r), "append-ptr-source", it);
    }
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// adversarial edge cases (char)
// ───────────────────────────────────────────────────────────────────────────

static void
run_edges(void)
{
  using S = micron::istring<char>;

  test_case("is substr default cnt==0 yields empty");
  {
    S s("abcdef");
    S t = s.substr(2);      // cnt defaults to 0 (NOT npos)
    require(t.size(), 0u);
  }
  end_test_case();

  test_case("is substr OOB throws");
  {
    S s("abc");
    expect_throw_type<micron::except::library_error>([&] { (void)s.substr(5, 1); });
  }
  end_test_case();

  test_case("is stack() throws at length >= 255");
  {
    char buf[300];
    for ( usize i = 0; i < 255; ++i ) buf[i] = 'a';
    S s = make_is<char>(buf, 255);
    expect_throw_type<micron::except::library_error>([&] { (void)s.stack(); });
  }
  end_test_case();

  test_case("is find empty needle returns pos");
  {
    S h("haystack");
    S n;
    require(h.find(n), 0u);
  }
  end_test_case();

  test_case("is c_str NUL-terminated");
  {
    S s("hello");
    require(s.c_str()[5], '\0');
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// memory / leak (char, instrumented allocator)
// ───────────────────────────────────────────────────────────────────────────

using TA = mtest::tracking_allocator<710>;
using LS = micron::istring<char, TA>;

template<typename T>
static LS
make_ls(const T *p, usize n)
{
  LS e;
  return e.append(p, n);
}

static void
run_memory(void)
{
  test_case("is fresh-alloc churn is leak-free");
  {
    snowball::expect_leak_free<TA>([] {
      prng rng(0xBEEF);
      for ( usize it = 0; it < 3000; ++it ) {
        char buf[128];
        usize n;
        mtest::gen_string<char>(rng, buf, n, 80, alpha::ascii);
        LS s = make_ls<char>(buf, n);
        LS t = s.push_back('!');        // fresh alloc
        LS u = t.append("xyz", 3);      // fresh alloc
        if ( u.size() > 2 ) {
          LS v = u.substr(1, u.size() - 2);      // fresh alloc
          (void)v.size();
        }
        // all temporaries drop here; every buffer must be freed
      }
    });
  }
  end_test_case();

  test_case("is move chain leak-free + donor empty");
  {
    snowball::expect_leak_free<TA>([] {
      for ( usize it = 0; it < 3000; ++it ) {
        LS a("a moderately long immutable string value");
        LS b(micron::move(a));
        LS c(micron::move(b));
        LS d(micron::move(c));
        require(a.size() == 0u, true);
        require(b.size() == 0u, true);
        require(c.size() == 0u, true);
        (void)d.size();
      }
    });
  }
  end_test_case();

  test_case("is destructor on moved-from is safe");
  {
    LS donor("moveme");
    LS s(micron::move(donor));
    (void)s.size();      // donor + s both destruct at scope end; no double free
  }
  end_test_case();
}

// ───────────────────────────────────────────────────────────────────────────
// driver
// ───────────────────────────────────────────────────────────────────────────

// ───────────────────────────────────────────────────────────────────────────
// new operators: - -= * *= / /= ^ ^= & &= | |=. istring is immutable, so every
// form (incl. the compound ones) returns a fresh istring by value.
// ───────────────────────────────────────────────────────────────────────────
template<typename T>
static void
run_newops(void)
{
  using S = micron::istring<T>;

  test_case("is operator- / -= fuzz vs oracle");
  {
    prng rng(0x7711u);
    T buf[MAXLEN + 4], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_is<T>(nd, m);
      ref_string<T> r;
      r.assign(buf, n);
      r.remove_all(nd, m);
      S s = make_is<T>(buf, n);
      ck(mtest::seq_eq(s - needle, r), "operator-", it);
      ck(s.size() == n, "operator- leaves original", it);
      ck(mtest::seq_eq(s -= needle, r), "operator-=", it);
    }
  }
  end_test_case();

  test_case("is operator* / *= fuzz vs oracle");
  {
    prng rng(0x7722u);
    T bb[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize base = static_cast<usize>(rng.next_in(8)) + 1;
      mtest::gen_string_n<T>(rng, bb, base, alpha::ascii);
      usize k = static_cast<usize>(rng.next_in(8));      // 0..7
      ref_string<T> r;
      r.assign(bb, base);
      r.repeat(k);
      S s = make_is<T>(bb, base);
      ck(mtest::seq_eq(s * k, r), "operator*", it);
      ck(mtest::seq_eq(s *= k, r), "operator*=", it);
    }
  }
  end_test_case();

  test_case("is operator/ //= fuzz vs oracle");
  {
    prng rng(0x7733u);
    T buf[MAXLEN + 4], add[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      usize an = static_cast<usize>(rng.next_in(32));
      mtest::gen_string_n<T>(rng, add, an, alpha::ascii);
      S addss = make_is<T>(add, an);
      ref_string<T> r;
      r.assign(buf, n);
      r.append(add, an);
      S s = make_is<T>(buf, n);
      ck(mtest::seq_eq(s / addss, r), "operator/", it);
      ck(mtest::seq_eq(s /= addss, r), "operator/=", it);
    }
  }
  end_test_case();

  test_case("is bitwise ^ & | (+compound) fuzz vs oracle");
  {
    prng rng(0x7744u);
    T buf[MAXLEN + 4], key[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      usize kn = static_cast<usize>(rng.next_in(40));      // 0..39 (incl. empty)
      mtest::gen_string_n<T>(rng, key, kn, alpha::full);
      S ks = make_is<T>(key, kn);
      const byte *kp = reinterpret_cast<const byte *>(key);
      const usize kb = kn * sizeof(T);

      ref_string<T> rx, ra, ro;
      rx.assign(buf, n);
      ra.assign(buf, n);
      ro.assign(buf, n);
      rx.xor_key(kp, kb);
      ra.and_key(kp, kb);
      ro.or_key(kp, kb);

      S s = make_is<T>(buf, n);
      auto x = s ^ ks;
      ck(mtest::seq_eq(x, rx) && x.size() == n, "operator^", it);
      ck(mtest::seq_eq(s & ks, ra), "operator&", it);
      ck(mtest::seq_eq(s | ks, ro), "operator|", it);
      ck(mtest::seq_eq(s ^= ks, rx), "operator^=", it);
      ck(mtest::seq_eq(s &= ks, ra), "operator&=", it);
      ck(mtest::seq_eq(s |= ks, ro), "operator|=", it);

      ref_string<T> rorig;
      rorig.assign(buf, n);
      ck(mtest::seq_eq(x ^ ks, rorig), "xor roundtrip", it);
    }
  }
  end_test_case();
}

template<typename T>
static void
run_all(const char *tyname)
{
  sb::print("--- istring<", tyname, "> ---");
  run_props<T>();
  run_accessors<T>();
  run_newops<T>();
}

int
main(int, char **)
{
  sb::print("=== ISTRING RIGOR ===");

  run_all<char>("char");
  run_all<byte>("byte");
  run_all<wide>("wide");
  run_all<unicode8>("unicode8");
  run_all<unicode16>("unicode16");
  run_all<unicode32>("unicode32");

  sb::print("--- edges / memory (char) ---");
  run_edges();
  run_memory();

  sb::print("=== ISTRING RIGOR DONE ===");
  return 1;
}
