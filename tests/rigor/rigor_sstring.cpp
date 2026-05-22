// Exhaustive, adversarial rigor suite for micron::sstring<N, T, Sf>.
//
// sstring is a fixed-capacity, stack-allocated, mutable string. This suite
// drives every member function from an adversarial standpoint: each group
// pairs a handful of deterministic edge cases with a >=10k-iteration randomized
// property loop that diffs sstring against the obviously-correct ref_string
// oracle (tests/support/string_rigor.hpp), plus explicit overflow / out-of-range
// / null cases that must throw library_error.
//
// Char-type coverage is exhaustive: run_props<T>() / run_edges<T>() are
// instantiated for char, byte(uint8_t), wide(wchar_t), unicode8(char8_t),
// unicode16(char16_t), unicode32(char32_t). Capacity behaviour that depends on
// N is exercised at several N via run_caps<N, T>().
//
// Memory correctness for a stack string == no OOB (ASan stack-buffer-overflow),
// and the Sf=true contract that bytes past length() are zero after
// clear/erase/pop. Lifetime: copy independence, move emptying, self-assign.
//
// Build: `duck build tests/rigor/rigor_sstring.cpp`; run `bin/rigor_sstring`.
// Exceptions are ON by default (src/defs.hpp).

#include "../../src/io/console.hpp"

#include "../../src/string/sstring.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"
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

static constexpr usize CAP = 2048;

template<usize N, typename T>
static micron::sstring<N, T>
make_ss(const T *p, usize n)
{
  micron::sstring<N, T> s;
  for ( usize i = 0; i < n; ++i ) s.push_back(p[i]);
  return s;
}

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

template<typename T>
static void
mk_charset(prng &rng, T *out, usize k, alpha a)
{
  for ( usize i = 0; i < k; ++i ) out[i] = mtest::gen_char<T>(rng, a);
  out[k] = static_cast<T>(0);
}

template<usize N>
static constexpr usize
usable(void)
{
  return N - 1;
}

template<typename T>
static void
run_edges(void)
{

  test_case("ss default ctor");
  {
    micron::sstring<64, T> s;
    require(s.empty(), true);
    require(s.size(), 0u);
    require(s.capacity(), 64u);
    require(s.max_size(), 64u);
    for ( usize i = 0; i < 64; ++i ) require(s[i], static_cast<T>(0));
  }
  end_test_case();

  test_case("ss boundary-length construction vs oracle");
  {
    prng rng(0xA5A5u);
    T buf[CAP];
    for ( usize li = 0; li < mtest::kStrLensCount; ++li ) {
      usize L = mtest::kStrLens[li];
      if ( L > usable<CAP>() ) continue;
      mtest::gen_string_n<T>(rng, buf, L, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, L);
      auto s = make_ss<CAP, T>(buf, L);
      ck(mtest::seq_eq(s, r), "ctor-boundary", L);
      ck(mtest::cstr_eq(s, r), "ctor-boundary-cstr", L);
    }
  }
  end_test_case();

  test_case("ss (iter,iter) ctor rejects empty range");
  {
    micron::sstring<16, T> s;
    T *b = s.data();
    expect_throw_type<micron::except::library_error>([&] { micron::sstring<16, T> bad(b, b); });
  }
  end_test_case();

  test_case("ss copy ctor independence");
  {
    auto s = make_ss<64, T>((const T *)nullptr, 0);
    for ( int i = 0; i < 5; ++i ) s.push_back(static_cast<T>('a' + i));
    micron::sstring<64, T> c(s);
    require(c.size(), s.size());
    c[0] = static_cast<T>('Z');
    require(s[0], static_cast<T>('a'));
    require(c[0], static_cast<T>('Z'));
  }
  end_test_case();

  test_case("ss move ctor empties donor");
  {
    micron::sstring<64, T> donor;
    for ( int i = 0; i < 6; ++i ) donor.push_back(static_cast<T>('x'));
    micron::sstring<64, T> m(micron::move(donor));
    require(m.size(), 6u);
    require(donor.size(), 0u);
  }
  end_test_case();
}

template<typename T>
static void
run_props(void)
{
  using S = micron::sstring<CAP, T>;

  test_case("ss ctor+copy+move fuzz");
  {
    prng rng(0x1111u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 300, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      ck(mtest::seq_eq(s, r), "ctor", it);

      S c(s);
      ck(mtest::seq_eq(c, r), "copy", it);

      if ( n ) {
        c[0] = static_cast<T>(c[0] ^ static_cast<T>(1));
        ck(static_cast<T>(s[0]) == r.buf[0], "copy-indep", it);
      }

      S fresh(s);
      S m(micron::move(fresh));
      ck(mtest::seq_eq(m, r), "move", it);
      ck(fresh.size() == 0u, "move-donor", it);
    }
  }
  end_test_case();

  test_case("ss copy/move assign fuzz");
  {
    prng rng(0x2222u);
    T a[CAP], b[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 300, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 300, alpha::ascii);
      ref_string<T> ra, rb;
      ra.assign(a, na);
      rb.assign(b, nb);
      S sa = make_ss<CAP, T>(a, na);
      S sb_ = make_ss<CAP, T>(b, nb);
      sa = sb_;
      ck(mtest::seq_eq(sa, rb), "copy-assign", it);
      ck(mtest::seq_eq(sb_, rb), "copy-assign-src", it);

      sa = sa;
      ck(mtest::seq_eq(sa, rb), "self-copy-assign", it);

      S sc = make_ss<CAP, T>(a, na);
      sa = micron::move(sc);
      ck(mtest::seq_eq(sa, ra), "move-assign", it);
    }
  }
  end_test_case();

  test_case("ss access [] / at / front / back fuzz");
  {
    prng rng(0x3333u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(static_cast<T>(s[idx]) == r.buf[idx], "op[]", it);
      ck(static_cast<T>(s.at(idx)) == r.buf[idx], "at", it);
      ck(static_cast<T>(s[0]) == r.buf[0], "first", it);
      ck(static_cast<T>(s[n - 1]) == r.buf[n - 1], "last", it);

      expect_throw_type<micron::except::library_error>([&] { (void)s.at(n); });
    }
  }
  end_test_case();

  test_case("ss find(char) fuzz vs oracle");
  {
    prng rng(0x4444u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      usize pos = n ? static_cast<usize>(rng.next_in(n + 1)) : 0;
      usize got = s.find(ch, pos);
      usize exp = r.find(ch, pos);
      ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-ch", it);
    }
  }
  end_test_case();

  test_case("ss find(needle) fuzz vs oracle");
  {
    prng rng(0x4545u);
    T buf[CAP], nd[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize m = static_cast<usize>(rng.next_in(5)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      micron::sstring<64, T> needle = make_ss<64, T>(nd, m);
      usize got = s.find(needle);
      usize exp = r.find_seq(nd, m, 0);
      ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-needle", it);
    }
  }
  end_test_case();

  test_case("ss rfind fuzz vs oracle");
  {
    prng rng(0x4646u);
    T buf[CAP], nd[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      usize got = s.rfind(ch);
      usize exp = r.rfind(ch);
      ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "rfind-ch", it);
      usize m = static_cast<usize>(rng.next_in(4)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      micron::sstring<64, T> needle = make_ss<64, T>(nd, m);
      usize g2 = s.rfind(needle);
      usize e2 = r.rfind_seq(nd, m);
      ck((g2 == micron::npos) == (e2 == REF_NPOS) && (e2 == REF_NPOS || g2 == e2), "rfind-needle", it);
    }
  }
  end_test_case();

  test_case("ss find_*_of fuzz vs oracle");
  {
    prng rng(0x4747u);
    T buf[CAP], set[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize k = static_cast<usize>(rng.next_in(3)) + 1;
      mk_charset<T>(rng, set, k, alpha::small);
      micron::sstring<16, T> cs = make_ss<16, T>(set, k);
      {
        usize got = s.find_first_of(cs);
        usize exp = r.find_first_of(set, k, 0);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "ffo", it);
      }
      {
        usize got = s.find_last_of(cs);
        usize exp = r.find_last_of(set, k);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "flo", it);
      }
      {
        usize got = s.find_first_not_of(cs);
        usize exp = r.find_first_not_of(set, k, 0);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "ffno", it);
      }
      {
        usize got = s.find_last_not_of(cs);
        usize exp = r.find_last_not_of(set, k);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "flno", it);
      }
    }
  }
  end_test_case();

  test_case("ss predicates fuzz vs oracle");
  {
    prng rng(0x4848u);
    T buf[CAP], nd[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize m = static_cast<usize>(rng.next_in(5)) + 1;

      if ( (rng.next() & 1u) && n >= m ) {
        for ( usize i = 0; i < m; ++i ) nd[i] = buf[i];
      } else {
        mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      }
      {
        micron::sstring<64, T> pre = make_ss<64, T>(nd, m);
        ck(s.starts_with(pre) == r.starts_with(nd, m), "starts_with", it);
        ck(s.contains(pre) == r.contains(nd, m), "contains", it);
      }

      if ( (rng.next() & 1u) && n >= m ) {
        for ( usize i = 0; i < m; ++i ) nd[i] = buf[n - m + i];
      } else {
        mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      }
      {
        micron::sstring<64, T> suf = make_ss<64, T>(nd, m);
        ck(s.ends_with(suf) == r.ends_with(nd, m), "ends_with", it);
      }
    }
  }
  end_test_case();

  test_case("ss count fuzz vs oracle");
  {
    prng rng(0x4949u);
    T buf[CAP], nd[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      ck(static_cast<usize>(s.count(ch)) == r.count_char(ch), "count-ch", it);
      usize m = static_cast<usize>(rng.next_in(3)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      micron::sstring<64, T> needle = make_ss<64, T>(nd, m);
      ck(static_cast<usize>(s.count(needle)) == r.count_seq(nd, m), "count-needle", it);
    }
  }
  end_test_case();

  test_case("ss compare fuzz vs oracle (sign)");
  {
    prng rng(0x5050u);
    T a[CAP], b[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 100, alpha::small);
      mtest::gen_string<T>(rng, b, nb, 100, alpha::small);
      ref_string<T> ra;
      ra.assign(a, na);
      S sa = make_ss<CAP, T>(a, na);
      S sb_ = make_ss<CAP, T>(b, nb);
      int got = sa.compare(sb_);
      int exp = ra.compare(b, nb);
      int gs = (got > 0) - (got < 0);
      int es = (exp > 0) - (exp < 0);
      ck(gs == es, "compare", it);
    }
  }
  end_test_case();

  test_case("ss append/push/pop fuzz vs oracle");
  {
    prng rng(0x6060u);
    T buf[CAP], add[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);

      usize k = static_cast<usize>(rng.next_in(16));
      for ( usize i = 0; i < k; ++i ) {
        T c = mtest::gen_char<T>(rng, alpha::ascii);
        s.push_back(c);
        r.push_back(c);
      }
      ck(mtest::seq_eq(s, r), "push_back", it);

      usize p = static_cast<usize>(rng.next_in(8));
      for ( usize i = 0; i < p; ++i ) {
        s.pop_back();
        r.pop_back();
      }
      ck(mtest::seq_eq(s, r), "pop_back", it);
      (void)add;
    }
  }
  end_test_case();

  test_case("ss insert(ind,ch,cnt) fuzz vs oracle");
  {
    prng rng(0x6161u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(8)) + 1;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      s.insert(ind, ch, cnt);
      r.insert_char(ind, ch, cnt);
      ck(mtest::seq_eq(s, r), "insert-ch", it);
    }
  }
  end_test_case();

  test_case("ss erase(ind,cnt) fuzz vs oracle");
  {
    prng rng(0x6262u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - ind)) + 1;
      s.erase(ind, cnt);
      r.erase(ind, cnt);
      ck(mtest::seq_eq(s, r), "erase", it);

      bool zp = true;
      for ( usize i = s.size(); i < CAP; ++i )
        if ( static_cast<T>(s[i]) != static_cast<T>(0) ) {
          zp = false;
          break;
        }
      ck(zp, "erase-zero-past-len", it);
    }
  }
  end_test_case();

  test_case("ss remove/remove_all fuzz vs oracle");
  {
    prng rng(0x6363u);
    T buf[CAP], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::small);
      ref_string<T> r1, r2;
      r1.assign(buf, n);
      r2.assign(buf, n);
      S s1 = make_ss<CAP, T>(buf, n);
      S s2 = make_ss<CAP, T>(buf, n);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      micron::sstring<8, T> needle = make_ss<8, T>(nd, m);
      s1.remove(needle);
      r1.remove(nd, m);
      ck(mtest::seq_eq(s1, r1), "remove", it);
      s2.remove_all(needle);
      r2.remove_all(nd, m);
      ck(mtest::seq_eq(s2, r2), "remove_all", it);
    }
  }
  end_test_case();

  test_case("ss replace/replace_all fuzz vs oracle");
  {
    prng rng(0x6464u);
    T buf[CAP], wbuf[16], nd[8], wd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::small);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - pos + 1));
      usize wl = static_cast<usize>(rng.next_in(6));
      mtest::gen_string_n<T>(rng, wbuf, wl, alpha::small);
      micron::sstring<16, T> wss = make_ss<16, T>(wbuf, wl);
      s.replace(pos, cnt, wss);
      r.replace(pos, cnt, wbuf, wl);
      ck(mtest::seq_eq(s, r), "replace", it);

      ref_string<T> r2;
      r2.assign(buf, n);
      S s2 = make_ss<CAP, T>(buf, n);
      usize ml = static_cast<usize>(rng.next_in(2)) + 1;
      usize kl = static_cast<usize>(rng.next_in(3));
      mtest::gen_string_n<T>(rng, nd, ml, alpha::small);
      mtest::gen_string_n<T>(rng, wd, kl, alpha::small);
      micron::sstring<8, T> nss = make_ss<8, T>(nd, ml);
      micron::sstring<8, T> wds = make_ss<8, T>(wd, kl);
      s2.replace_all(nss, wds);

      {
        usize p = 0;
        while ( (p = r2.find_seq(nd, ml, p)) != REF_NPOS ) {
          r2.replace(p, ml, wd, kl);
          p += kl;
        }
      }
      ck(mtest::seq_eq(s2, r2), "replace_all", it);
    }
  }
  end_test_case();

  test_case("ss substr/truncate fuzz vs oracle");
  {
    prng rng(0x6565u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(n - pos + 1));
      auto sub = s.template substr<CAP, T>(pos, cnt);
      ref_string<T> rsub;
      rsub.assign(r.buf + pos, cnt);
      ck(mtest::seq_eq(sub, rsub), "substr", it);

      usize tn = static_cast<usize>(rng.next_in(n + 1));
      s.truncate(tn);
      r.truncate(tn);
      ck(mtest::seq_eq(s, r), "truncate", it);
    }
  }
  end_test_case();

  test_case("ss fill/repeat fuzz vs oracle");
  {
    prng rng(0x6666u);
    for ( usize it = 0; it < ITERS; ++it ) {
      S s;
      ref_string<T> r;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      usize cnt = static_cast<usize>(rng.next_in(usable<CAP>()));
      s.fill(ch, cnt);
      r.fill(ch, cnt);
      ck(mtest::seq_eq(s, r), "fill", it);

      usize base = static_cast<usize>(rng.next_in(8)) + 1;
      T bb[16];
      mtest::gen_string_n<T>(rng, bb, base, alpha::ascii);
      S s2 = make_ss<CAP, T>(bb, base);
      ref_string<T> r2;
      r2.assign(bb, base);
      usize times = static_cast<usize>(rng.next_in(8)) + 1;

      if ( base * times < usable<CAP>() ) {
        s2.repeat(times);
        r2.repeat(times);
        ck(mtest::seq_eq(s2, r2), "repeat", it);
      }
    }
  }
  end_test_case();

  test_case("ss reverse fuzz vs oracle");
  {
    prng rng(0x6767u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      s.reverse();
      r.reverse();
      ck(mtest::seq_eq(s, r), "reverse", it);
    }
  }
  end_test_case();

  test_case("ss to_lower/to_upper fuzz vs oracle");
  {
    prng rng(0x6868u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 250, alpha::case_mix);
      ref_string<T> rl, ru;
      rl.assign(buf, n);
      ru.assign(buf, n);
      S sl = make_ss<CAP, T>(buf, n);
      S su = make_ss<CAP, T>(buf, n);
      sl.to_lower();
      rl.to_lower();
      ck(mtest::seq_eq(sl, rl), "to_lower", it);
      su.to_upper();
      ru.to_upper();
      ck(mtest::seq_eq(su, ru), "to_upper", it);
    }
  }
  end_test_case();

  test_case("ss trim fuzz vs oracle");
  {
    prng rng(0x6969u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ws_mix);
      ref_string<T> rl, rr, rt;
      rl.assign(buf, n);
      rr.assign(buf, n);
      rt.assign(buf, n);
      S sl = make_ss<CAP, T>(buf, n);
      S sr = make_ss<CAP, T>(buf, n);
      S st = make_ss<CAP, T>(buf, n);
      sl.trim_left();
      rl.trim_left();
      ck(mtest::seq_eq(sl, rl), "trim_left", it);
      sr.trim_right();
      rr.trim_right();
      ck(mtest::seq_eq(sr, rr), "trim_right", it);
      st.trim();
      rt.trim();
      ck(mtest::seq_eq(st, rt), "trim", it);
    }
  }
  end_test_case();

  test_case("ss operator+= fuzz vs oracle");
  {
    prng rng(0x7070u);
    T a[CAP], b[256];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 150, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 100, alpha::ascii);
      ref_string<T> r;
      r.assign(a, na);
      S s = make_ss<CAP, T>(a, na);
      S add = make_ss<CAP, T>(b, nb);
      s += add;
      r.append(b, nb);
      ck(mtest::seq_eq(s, r), "+=sstring", it);

      T c = mtest::gen_char<T>(rng, alpha::ascii);
      s += c;
      r.push_back(c);
      ck(mtest::seq_eq(s, r), "+=char", it);
    }
  }
  end_test_case();

  test_case("ss clear/fast_clear");
  {
    prng rng(0x7171u);
    T buf[CAP];
    for ( usize it = 0; it < 2000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 100, alpha::full);
      S s = make_ss<CAP, T>(buf, n);
      s.clear();
      ck(s.size() == 0u, "clear-size", it);
      bool zp = true;
      for ( usize i = 0; i < CAP; ++i )
        if ( static_cast<T>(s[i]) != static_cast<T>(0) ) {
          zp = false;
          break;
        }
      ck(zp, "clear-zero-buffer", it);
      S s2 = make_ss<CAP, T>(buf, n);
      s2.fast_clear();
      ck(s2.size() == 0u, "fast_clear-size", it);
    }
  }
  end_test_case();
}

template<typename T>
static void
run_accessors(void)
{
  using S = micron::sstring<CAP, T>;

  test_case("ss data/cdata/c_str/clone/spans");
  {
    prng rng(0xACC0u);
    T buf[CAP];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);

      const T *d = s.data();
      bool ok = true;
      for ( usize i = 0; i < n; ++i )
        if ( d[i] != r.buf[i] ) {
          ok = false;
          break;
        }
      ck(ok, "data", it);
      ck(mtest::cstr_eq(s, r), "c_str", it);

      auto cl = s.clone();
      ck(mtest::seq_eq(cl, r), "clone", it);
      if ( n ) {
        cl[0] = static_cast<T>(cl[0] ^ static_cast<T>(1));
        ck(static_cast<T>(s[0]) == r.buf[0], "clone-indep", it);
      }

      ck(s.capacity() == CAP && s.max_size() == CAP, "cap", it);
      ck(s.len() == s.size(), "len==size", it);
    }
  }
  end_test_case();

  test_case("ss conversion operators (! / bool / const char*)");
  {
    S empty;
    require(!empty, true);
    require(static_cast<bool>(empty), false);
    S full("data");
    require(!full, false);
    require(static_cast<bool>(full), true);
    const char *cp = static_cast<const char *>(full);
    require(cp[0], 'd');
    require(full.addr()->size() == 4u, true);
  }
  end_test_case();

  test_case("ss iteration begin/end/last/cbegin/cend");
  {
    prng rng(0xACC1u);
    T buf[CAP];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
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

  test_case("ss set_size / fill / pop_back / null_term");
  {
    prng rng(0xACC2u);
    for ( usize it = 0; it < 3000; ++it ) {
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      usize cnt = static_cast<usize>(rng.next_in(usable<CAP>()));
      S s;
      s.fill(ch, cnt);
      ref_string<T> r;
      r.fill(ch, cnt);
      ck(mtest::seq_eq(s, r), "fill", it);
      if ( cnt ) {
        s.pop_back();
        r.pop_back();
        ck(mtest::seq_eq(s, r), "pop_back", it);
      }
    }
  }
  end_test_case();
}

template<typename T>
static void
run_iterops(void)
{
  using S = micron::sstring<CAP, T>;

  test_case("ss insert(iterator,ch,cnt) fuzz vs oracle");
  {
    prng rng(0x1701u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(8)) + 1;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      s.insert(s.begin() + ind, ch, cnt);
      r.insert_char(ind, ch, cnt);
      ck(mtest::seq_eq(s, r), "insert-iter", it);
    }
  }
  end_test_case();

  test_case("ss erase(iterator,cnt) fuzz vs oracle");
  {
    prng rng(0x1702u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - ind)) + 1;
      s.erase(s.begin() + ind, cnt);
      r.erase(ind, cnt);
      ck(mtest::seq_eq(s, r), "erase-iter", it);
    }
  }
  end_test_case();

  test_case("ss substr(iterator) / truncate(iterator) fuzz");
  {
    prng rng(0x1703u);
    T buf[CAP];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - pos)) + 1;
      auto sub = s.template substr<CAP, T>(s.cbegin() + pos, s.cbegin() + pos + cnt);
      ref_string<T> rsub;
      rsub.assign(r.buf + pos, cnt);
      ck(mtest::seq_eq(sub, rsub), "substr-iter", it);
      usize tn = static_cast<usize>(rng.next_in(n + 1));
      s.truncate(s.begin() + tn);
      r.truncate(tn);
      ck(mtest::seq_eq(s, r), "truncate-iter", it);
    }
  }
  end_test_case();

  test_case("ss replace(iterator-range) fuzz vs oracle");
  {
    prng rng(0x1704u);
    T buf[CAP], wbuf[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::small);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_ss<CAP, T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - pos + 1));
      usize wl = static_cast<usize>(rng.next_in(6));
      mtest::gen_string_n<T>(rng, wbuf, wl, alpha::small);
      micron::sstring<16, T> wss = make_ss<16, T>(wbuf, wl);
      s.replace(s.begin() + pos, s.begin() + pos + cnt, wss.data());

      r.replace(pos, cnt, wbuf, wl);

      ck(mtest::seq_eq(s, r), "replace-iter", it);
    }
  }
  end_test_case();
}

template<typename T>
static void
run_ctors(void)
{
  test_case("ss cross-size copy clamps to capacity");
  {
    prng rng(0x2c01u);
    T buf[64];
    for ( usize it = 0; it < 3000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 60, alpha::ascii);
      micron::sstring<64, T> big = make_ss<64, T>(buf, n);

      micron::sstring<16, T> small(big);
      ck(small.size() <= 16u, "cross-copy-clamp", it);
      usize lim = n < 16u ? n : 16u;
      bool ok = true;
      for ( usize i = 0; i < lim; ++i )
        if ( static_cast<T>(small[i]) != buf[i] ) {
          ok = false;
          break;
        }
      ck(ok, "cross-copy-prefix", it);
    }
  }
  end_test_case();

  test_case("ss assignment overloads (sstring / cross-size)");
  {
    prng rng(0x2c02u);
    T a[200], b[200];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 150, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 60, alpha::ascii);
      micron::sstring<CAP, T> sa = make_ss<CAP, T>(a, na);
      micron::sstring<64, T> sb_ = make_ss<64, T>(b, nb);
      sa = sb_;
      ref_string<T> rb;
      rb.assign(b, nb);
      ck(mtest::seq_eq(sa, rb), "cross-assign", it);
    }
  }
  end_test_case();

  test_case("ss construction from hstring (is_string)");
  {
    prng rng(0x2c03u);
    T buf[200];
    for ( usize it = 0; it < 2000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      if ( n >= CAP - 1 ) n = CAP - 2;
      micron::hstring<T> h = (n == 0) ? micron::hstring<T>() : micron::hstring<T>(buf, buf + n);
      micron::sstring<CAP, T> s(h);
      ref_string<T> r;
      r.assign(buf, n);
      ck(mtest::seq_eq(s, r), "from-hstring", it);
    }
  }
  end_test_case();
}

static void
run_adversarial(void)
{
  using S = micron::sstring<64, char>;

  test_case("ss OOB index/insert/erase/replace/substr throw");
  {
    S s("abcdef");
    expect_throw_type<micron::except::library_error>([&] { (void)s.at(6); });
    expect_throw_type<micron::except::library_error>([&] { s.erase(6u, 1u); });
    expect_throw_type<micron::except::library_error>([&] { s.erase(0u, 100u); });
    expect_throw_type<micron::except::library_error>([&] { (void)s.substr(7, 1); });
    expect_throw_type<micron::except::library_error>([&] { s.replace(7u, 1u, "x"); });
  }
  end_test_case();

  test_case("ss null pointer args throw");
  {
    S s("abc");
    const char *np = nullptr;
    expect_throw_type<micron::except::library_error>([&] { (void)s.find(np); });
    expect_throw_type<micron::except::library_error>([&] { (void)s.rfind(np); });
    expect_throw_type<micron::except::library_error>([&] { (void)s.compare(np); });
    expect_throw_type<micron::except::library_error>([&] { (void)s.find_first_of(np); });
  }
  end_test_case();

  test_case("ss empty-string operations are well-defined");
  {
    S s;
    require(s.find('a'), micron::npos);
    require(s.rfind('a'), micron::npos);
    require(s.count('a'), 0u);
    require(s.starts_with("x"), false);
    require(s.contains("x"), false);
    usize zpos = 0, zcnt = 0;
    auto sub = s.substr(zpos, zcnt);
    require(sub.size(), 0u);
    s.to_lower();
    s.to_upper();
    s.trim();
    s.reverse();
    require(s.size(), 0u);
  }
  end_test_case();

  test_case("ss exact-capacity fill and access");
  {
    micron::sstring<8, char> s;
    s.fill('a', 7);
    require(s.size(), 7u);
    require(s.at(6), 'a');
    expect_throw_type<micron::except::library_error>([&] { (void)s.at(7); });
  }
  end_test_case();
}

template<usize N, typename T>
static void
run_caps(void)
{

  test_case("ss capacity boundary");
  {
    micron::sstring<N, T> s;
    for ( usize i = 0; i < usable<N>(); ++i ) s.push_back(static_cast<T>('a'));
    require(s.size(), usable<N>());

    usize before = s.size();
    s.push_back(static_cast<T>('z'));
    require(s.size(), before);
  }
  end_test_case();

  test_case("ss insert overflow throws");
  {
    micron::sstring<N, T> s;
    for ( usize i = 0; i < usable<N>(); ++i ) s.push_back(static_cast<T>('a'));
    expect_throw_type<micron::except::library_error>([&] { s.insert(0u, static_cast<T>('x'), N); });
  }
  end_test_case();

  test_case("ss repeat overflow throws");
  {
    micron::sstring<N, T> s;
    usize half = usable<N>() / 2 + 1;
    for ( usize i = 0; i < half; ++i ) s.push_back(static_cast<T>('a'));
    expect_throw_type<micron::except::library_error>([&] { s.repeat(4); });
  }
  end_test_case();

  test_case("ss substr too-large-target throws");
  {
    micron::sstring<N, T> s;
    for ( usize i = 0; i < usable<N>(); ++i ) s.push_back(static_cast<T>('a'));
    if ( usable<N>() >= 4 ) expect_throw_type<micron::except::library_error>([&] { (void)s.template substr<4, T>(0, usable<N>()); });
  }
  end_test_case();

  test_case("ss erase/at OOB throw");
  {
    micron::sstring<N, T> s;
    for ( usize i = 0; i < usable<N>(); ++i ) s.push_back(static_cast<T>('a'));
    usize len = s.size();
    expect_throw_type<micron::except::library_error>([&] { (void)s.at(len); });
    expect_throw_type<micron::except::library_error>([&] { s.erase(len, 1u); });
  }
  end_test_case();
}

template<typename T>
static void
run_newops(void)
{
  using S = micron::sstring<CAP, T>;

  test_case("ss operator- / -= fuzz vs oracle");
  {
    prng rng(0x7711u);
    T buf[CAP], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::small);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      micron::sstring<8, T> needle = make_ss<8, T>(nd, m);
      ref_string<T> r;
      r.assign(buf, n);
      r.remove_all(nd, m);
      S s = make_ss<CAP, T>(buf, n);
      auto sub = s - needle;
      ck(mtest::seq_eq(sub, r), "operator-", it);
      ck(s.size() == n, "operator- keeps lhs", it);
      s -= needle;
      ck(mtest::seq_eq(s, r), "operator-=", it);
    }
  }
  end_test_case();

  test_case("ss operator* / *= / times fuzz vs oracle");
  {
    prng rng(0x7722u);
    T bb[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize base = static_cast<usize>(rng.next_in(8)) + 1;
      mtest::gen_string_n<T>(rng, bb, base, alpha::ascii);
      usize k = static_cast<usize>(rng.next_in(8));
      ref_string<T> r;
      r.assign(bb, base);
      r.repeat(k);
      S s = make_ss<CAP, T>(bb, base);
      auto p = s * k;
      ck(mtest::seq_eq(p, r), "operator*", it);
      S s2 = make_ss<CAP, T>(bb, base);
      s2 *= k;
      ck(mtest::seq_eq(s2, r), "operator*=", it);
    }

    prng rng2(0x7723u);
    T bb2[16];
    for ( usize it = 0; it < 512; ++it ) {
      usize base = static_cast<usize>(rng2.next_in(8)) + 1;
      mtest::gen_string_n<T>(rng2, bb2, base, alpha::ascii);
      micron::sstring<64, T> b = make_ss<64, T>(bb2, base);
      auto t = b.template times<3>();
      ref_string<T> rr;
      rr.assign(bb2, base);
      rr.repeat(3);
      ck(mtest::seq_eq(t, rr) && t.size() == base * 3, "times<3>", it);
    }
  }
  end_test_case();

  test_case("ss operator/ ");
  {
    prng rng(0x7733u);
    T buf[CAP], add[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n = static_cast<usize>(rng.next_in(400));
      mtest::gen_string_n<T>(rng, buf, n, alpha::ascii);
      usize an = static_cast<usize>(rng.next_in(32));
      mtest::gen_string_n<T>(rng, add, an, alpha::ascii);
      micron::sstring<64, T> addss = make_ss<64, T>(add, an);
      ref_string<T> r;
      r.assign(buf, n);
      r.append(add, an);
      S s = make_ss<CAP, T>(buf, n);
      auto cc = s / addss;
      ck(mtest::seq_eq(cc, r), "operator/", it);
      s /= addss;
      ck(mtest::seq_eq(s, r), "operator/=", it);
    }
  }
  end_test_case();

  test_case("ss bitwise ^ & | (+compound) fuzz vs oracle");
  {
    prng rng(0x7744u);
    T buf[CAP], key[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 300, alpha::full);
      usize kn = static_cast<usize>(rng.next_in(40));
      mtest::gen_string_n<T>(rng, key, kn, alpha::full);
      micron::sstring<64, T> ks = make_ss<64, T>(key, kn);
      const byte *kp = reinterpret_cast<const byte *>(key);
      const usize kb = kn * sizeof(T);

      ref_string<T> rx, ra, ro;
      rx.assign(buf, n);
      ra.assign(buf, n);
      ro.assign(buf, n);
      rx.xor_key(kp, kb);
      ra.and_key(kp, kb);
      ro.or_key(kp, kb);

      S s = make_ss<CAP, T>(buf, n);
      auto x = s ^ ks;
      ck(mtest::seq_eq(x, rx) && x.size() == n, "operator^", it);
      ck(mtest::seq_eq(s & ks, ra), "operator&", it);
      ck(mtest::seq_eq(s | ks, ro), "operator|", it);

      S sx = make_ss<CAP, T>(buf, n);
      sx ^= ks;
      ck(mtest::seq_eq(sx, rx), "operator^=", it);
      S sa = make_ss<CAP, T>(buf, n);
      sa &= ks;
      ck(mtest::seq_eq(sa, ra), "operator&=", it);
      S so = make_ss<CAP, T>(buf, n);
      so |= ks;
      ck(mtest::seq_eq(so, ro), "operator|=", it);

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
  sb::print("--- sstring<", tyname, "> ---");
  run_edges<T>();
  run_props<T>();
  run_accessors<T>();
  run_iterops<T>();
  run_ctors<T>();
  run_newops<T>();
  run_caps<8, T>();
  run_caps<16, T>();
  run_caps<64, T>();
  run_caps<256, T>();
}

int
main(int, char **)
{
  sb::print("=== SSTRING RIGOR ===");

  run_all<char>("char");
  run_all<byte>("byte");
  run_all<wide>("wide");
  run_all<unicode8>("unicode8");
  run_all<unicode16>("unicode16");
  run_all<unicode32>("unicode32");

  sb::print("--- adversarial / edges (char) ---");
  run_adversarial();

  sb::print("=== SSTRING RIGOR DONE ===");
  return 1;
}
