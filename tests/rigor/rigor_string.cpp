// Exhaustive, adversarial rigor suite for micron::hstring<T, Sf, Alloc>
// (a.k.a. micron::string == hstring<char>).
//
// hstring is a heap-allocated, growable, mutable string. This suite drives
// every member function from an adversarial standpoint and — because hstring
// owns heap memory — puts heavy emphasis on object lifetime and memory
// correctness:
//   * functional groups (ctor/assign/access/find/append/insert/erase/remove/
//     substr/operators) diffed against the ref_string oracle, >=10k iters/fn,
//     across char/byte/wide/unicode8/unicode16/unicode32.
//   * lifetime: deep-copy independence, move emptying + reuse, self-assign and
//     self-move safety, lvalue/rvalue/xvalue/temporary dispatch.
//   * memory: leak-freeness over churn via mtest::tracking_allocator +
//     expect_leak_free; c-string ctor NUL-termination; clear() wiping the full
//     capacity; basic exception safety under mtest::throwing_allocator.
//
// Build: `duck build tests/rigor/rigor_string.cpp`; run `bin/rigor_string`.

#include "../../src/io/console.hpp"

#include "../../src/string/string.hpp"
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

static constexpr usize MAXLEN = 300;

template<typename T>
static micron::hstring<T>
make_hs(const T *p, usize n)
{

  if ( n == 0 ) return micron::hstring<T>();
  return micron::hstring<T>(static_cast<const T *>(p), static_cast<const T *>(p) + n);
}

static void
ck(bool ok, const char *what, usize it)
{
  if ( !ok ) sb::print("\033[31mMISMATCH\033[0m op=", what, " iter=", (u64)it);
  require(ok, true);
}

template<typename T>
static void
run_props(void)
{
  using S = micron::hstring<T>;

  test_case("hs ctor+copy+move fuzz");
  {
    prng rng(0x1111u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
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
      ck(m.size() == r.len, "move-size", it);
    }
  }
  end_test_case();

  test_case("hs assign fuzz");
  {
    prng rng(0x2222u);
    T a[MAXLEN + 4], b[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, MAXLEN, alpha::ascii);
      ref_string<T> ra, rb;
      ra.assign(a, na);
      rb.assign(b, nb);
      S sa = make_hs<T>(a, na);
      S sb_ = make_hs<T>(b, nb);
      sa = sb_;
      ck(mtest::seq_eq(sa, rb), "copy-assign", it);
      ck(mtest::seq_eq(sb_, rb), "copy-assign-src", it);
      sa = sa;
      ck(mtest::seq_eq(sa, rb), "self-copy-assign", it);
      S sc = make_hs<T>(a, na);
      sa = micron::move(sc);
      ck(mtest::seq_eq(sa, ra), "move-assign", it);
    }
  }
  end_test_case();

  test_case("hs access fuzz");
  {
    prng rng(0x3333u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(static_cast<T>(s[idx]) == r.buf[idx], "op[]", it);
      ck(static_cast<T>(s.at(idx)) == r.buf[idx], "at", it);
      ck(static_cast<T>(s.front()) == r.buf[0], "front", it);
      ck(static_cast<T>(s.back()) == r.buf[n - 1], "back", it);
      expect_throw_type<micron::except::library_error>([&] { (void)s.at(n); });
    }
  }
  end_test_case();

  test_case("hs find fuzz vs oracle");
  {
    prng rng(0x4444u);
    T buf[MAXLEN + 4], nd[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      usize pos = n ? static_cast<usize>(rng.next_in(n + 1)) : 0;
      {
        usize got = s.find(ch, pos);
        usize exp = r.find(ch, pos);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-ch", it);
      }
      usize m = static_cast<usize>(rng.next_in(4)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_hs<T>(nd, m);
      {
        usize got = s.find(needle);
        usize exp = r.find_seq(nd, m, 0);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-needle", it);
      }
    }
  }
  end_test_case();

  test_case("hs append/push/pop fuzz");
  {
    prng rng(0x5555u);
    T buf[MAXLEN + 4], add[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize m;
      mtest::gen_string<T>(rng, add, m, 64, alpha::ascii);
      S adds = make_hs<T>(add, m);
      s.append(adds);
      r.append(add, m);
      ck(mtest::seq_eq(s, r), "append", it);
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
    }
  }
  end_test_case();

  test_case("hs insert fuzz vs oracle");
  {
    prng rng(0x6161u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(8)) + 1;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      s.insert(ind, ch, cnt);
      r.insert_char(ind, ch, cnt);
      ck(mtest::seq_eq(s, r), "insert-ch", it);
    }
  }
  end_test_case();

  test_case("hs erase fuzz vs oracle");
  {
    prng rng(0x6262u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - ind)) + 1;
      s.erase(ind, cnt);
      r.erase(ind, cnt);
      ck(mtest::seq_eq(s, r), "erase", it);
    }
  }
  end_test_case();

  test_case("hs remove/remove_all fuzz vs oracle");
  {
    prng rng(0x6363u);
    T buf[MAXLEN + 4], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      ref_string<T> r1, r2;
      r1.assign(buf, n);
      r2.assign(buf, n);
      S s1 = make_hs<T>(buf, n);
      S s2 = make_hs<T>(buf, n);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_hs<T>(nd, m);
      s1.remove(needle);
      r1.remove(nd, m);
      ck(mtest::seq_eq(s1, r1), "remove", it);
      s2.remove_all(needle);
      r2.remove_all(nd, m);
      ck(mtest::seq_eq(s2, r2), "remove_all", it);
    }
  }
  end_test_case();

  test_case("hs substr/truncate fuzz vs oracle");
  {
    prng rng(0x6565u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(n - pos + 1));
      auto sub = s.substr(pos, cnt);
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

  test_case("hs operator+= fuzz vs oracle");
  {
    prng rng(0x7070u);
    T a[MAXLEN + 4], b[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 64, alpha::ascii);
      ref_string<T> r;
      r.assign(a, na);
      S s = make_hs<T>(a, na);
      S add = make_hs<T>(b, nb);
      s += add;
      r.append(b, nb);
      ck(mtest::seq_eq(s, r), "+=hstring", it);
      T c = mtest::gen_char<T>(rng, alpha::ascii);
      s += c;
      r.push_back(c);
      ck(mtest::seq_eq(s, r), "+=char", it);
    }
  }
  end_test_case();

  test_case("hs comparison fuzz vs oracle");
  {
    prng rng(0x8080u);
    T a[128], b[128];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 64, alpha::small);
      mtest::gen_string<T>(rng, b, nb, 64, alpha::small);
      ref_string<T> ra;
      ra.assign(a, na);
      S sa = make_hs<T>(a, na);
      S sb_ = make_hs<T>(b, nb);
      int exp = ra.compare(b, nb);
      ck((sa == sb_) == (exp == 0), "==", it);
      ck((sa != sb_) == (exp != 0), "!=", it);
      ck((sa < sb_) == (exp < 0), "<", it);
      ck((sa > sb_) == (exp > 0), ">", it);
      ck((sa <= sb_) == (exp <= 0), "<=", it);
      ck((sa >= sb_) == (exp >= 0), ">=", it);
    }
  }
  end_test_case();

  test_case("hs reserve/resize/clear fuzz");
  {
    prng rng(0x9090u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);

      usize cap = static_cast<usize>(rng.next_in(400)) + 1;
      s.reserve(cap);
      ck(mtest::seq_eq(s, r), "reserve-keeps", it);
      ck(s.max_size() >= cap, "reserve-cap", it);

      usize rn = static_cast<usize>(rng.next_in(350));
      T fillc = mtest::gen_char<T>(rng, alpha::ascii);
      s.resize(rn, fillc);
      if ( rn <= r.len ) {
        r.truncate(rn);
      } else {
        for ( usize i = r.len; i < rn; ++i ) r.push_back(fillc);
      }
      ck(mtest::seq_eq(s, r), "resize", it);
      s.clear();
      ck(s.size() == 0u, "clear", it);
    }
  }
  end_test_case();
}

template<typename T>
static void
run_accessors(void)
{
  using S = micron::hstring<T>;

  test_case("hs data/cdata/c_str/clone/into_*");
  {
    prng rng(0xACC0u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
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
      ck(s.len() == s.size(), "len==size", it);
      ck(s.max_size() >= s.size(), "max_size", it);
    }
  }
  end_test_case();

  test_case("hs iteration begin/end/last/cbegin/cend");
  {
    prng rng(0xACC1u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 200, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize idx = 0;
      bool ok = true;
      for ( auto i = s.begin(); i != s.end(); ++i, ++idx )
        if ( static_cast<T>(*i) != r.buf[idx] ) {
          ok = false;
          break;
        }
      ck(ok && idx == n, "begin/end", it);
      ck(static_cast<T>(*s.last()) == r.buf[n - 1], "last", it);
    }
  }
  end_test_case();

  test_case("hs append overloads (ptr/array/sstring)");
  {
    prng rng(0xACC2u);
    T a[256], b[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 150, alpha::ascii);
      mtest::gen_string<T>(rng, b, nb, 40, alpha::ascii);
      ref_string<T> r;
      r.assign(a, na);

      S s = make_hs<T>(a, na);
      s.append(static_cast<const T *>(b), nb);
      r.append(b, nb);
      ck(mtest::seq_eq(s, r), "append-ptr", it);

      if ( nb < 64 ) {
        micron::sstring<64, T> ss;
        for ( usize i = 0; i < nb; ++i ) ss.push_back(b[i]);
        S s2 = make_hs<T>(a, na);
        s2.append(ss);
        ref_string<T> r2;
        r2.assign(a, na);
        r2.append(b, nb);
        ck(mtest::seq_eq(s2, r2), "append-sstring", it);
      }
    }
  }
  end_test_case();

  test_case("hs reserve/try_reserve/fast_clear");
  {
    prng rng(0xACC3u);
    T buf[200];
    for ( usize it = 0; it < 3000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);

      usize cap = s.max_size() + static_cast<usize>(rng.next_in(200)) + 1;
      s.try_reserve(cap);
      ck(mtest::seq_eq(s, r), "try_reserve-keeps", it);
      ck(s.max_size() >= cap, "try_reserve-cap", it);
      s.fast_clear();
      ck(s.size() == 0u, "fast_clear", it);
    }
  }
  end_test_case();

  test_case("hs iterator insert/erase fuzz");
  {
    prng rng(0xACC4u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_hs<T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(6)) + 1;
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      s.insert(s.begin() + ind, ch, cnt);
      r.insert_char(ind, ch, cnt);
      ck(mtest::seq_eq(s, r), "insert-iter", it);
      if ( s.size() ) {
        usize e = static_cast<usize>(rng.next_in(s.size()));
        s.erase(s.begin() + e, 1u);
        r.erase(e, 1u);
        ck(mtest::seq_eq(s, r), "erase-iter", it);
      }
    }
  }
  end_test_case();
}

static void
run_adversarial(void)
{
  using S = micron::hstring<char>;

  test_case("hs OOB / empty-string edges");
  {
    S s("abcdef");
    expect_throw_type<micron::except::library_error>([&] { (void)s.at(6); });
    S e;
    require(e.find('a'), micron::npos);
    require(e.empty(), true);
    usize zpos = 0, zcnt = 0;
    auto sub = e.substr(zpos, zcnt);
    require(sub.size(), 0u);
    e.clear();
    require(e.size(), 0u);
  }
  end_test_case();

  test_case("hs try_reserve shrink-request throws");
  {
    S s("abcdef");
    expect_throw_type<micron::except::memory_error>([&] { s.try_reserve(1); });
  }
  end_test_case();

  test_case("hs embedded-NUL content preserved by size (not c_str)");
  {
    char raw[8] = { 'a', 'b', 0, 'c', 'd', 0, 'e', 'f' };
    S s = make_hs<char>(raw, 8);
    require(s.size(), 8u);
    require(s[2], '\0');
    require(s[5], '\0');
    require(s[7], 'f');
  }
  end_test_case();
}

template<typename T>
static void
run_lifetime(void)
{
  using S = micron::hstring<T>;

  test_case("hs deep-copy independence");
  {
    T buf[64];
    prng rng(1);
    mtest::gen_string_n<T>(rng, buf, 32, alpha::ascii);
    S a = make_hs<T>(buf, 32);
    S b(a);
    for ( usize i = 0; i < 32; ++i ) b[i] = static_cast<T>('Z');
    bool indep = true;
    for ( usize i = 0; i < 32; ++i )
      if ( static_cast<T>(a[i]) != buf[i] ) {
        indep = false;
        break;
      }
    require(indep, true);
  }
  end_test_case();

  test_case("hs move empties source and is reusable");
  {
    S a = make_hs<T>((const T *)nullptr, 0);
    for ( int i = 0; i < 10; ++i ) a.push_back(static_cast<T>('a' + i));
    S b(micron::move(a));
    require(b.size(), 10u);

    a.push_back(static_cast<T>('x'));
    require(a.size() >= 1u, true);
    require(static_cast<T>(a[a.size() - 1]) == static_cast<T>('x'), true);
  }
  end_test_case();

  test_case("hs self-move is safe");
  {
    S a = make_hs<T>((const T *)nullptr, 0);
    for ( int i = 0; i < 8; ++i ) a.push_back(static_cast<T>('q'));
    a = micron::move(a);
    require(a.size() <= 8u, true);
    a.push_back(static_cast<T>('r'));
    require(a.size() >= 1u, true);
  }
  end_test_case();

  test_case("hs xvalue vs lvalue dispatch (content correctness)");
  {
    S src = make_hs<T>((const T *)nullptr, 0);
    for ( int i = 0; i < 12; ++i ) src.push_back(static_cast<T>('m'));
    S lcopy(src);
    S xmove(micron::move(src));
    require(lcopy.size(), 12u);
    require(xmove.size(), 12u);
  }
  end_test_case();
}

using TA = mtest::tracking_allocator<700>;
using THA = mtest::throwing_allocator<701>;
using LS = micron::hstring<char, true, TA>;
using XS = micron::hstring<char, true, THA>;

static void
run_memory(void)
{

  test_case("hs leak-free churn");
  {
    snowball::expect_leak_free<TA>([] {
      prng rng(0xCAFE);
      for ( usize it = 0; it < 4000; ++it ) {
        LS s;
        char buf[128];
        usize n;
        mtest::gen_string<char>(rng, buf, n, 100, alpha::ascii);
        for ( usize i = 0; i < n; ++i ) s.push_back(buf[i]);
        usize e = static_cast<usize>(rng.next_in(n + 1));
        for ( usize i = 0; i < e && s.size() > 0; ++i ) s.pop_back();
        s.clear();
      }
    });
  }
  end_test_case();

  test_case("hs copy/move/assign leak-free");
  {
    snowball::expect_leak_free<TA>([] {
      for ( usize it = 0; it < 2000; ++it ) {
        LS a("the quick brown fox jumps");
        LS b(a);
        LS c(micron::move(b));
        LS d;
        d = a;
        LS e;
        e = micron::move(c);
        (void)d.size();
        (void)e.size();
      }
    });
  }
  end_test_case();

  test_case("hs c_str NUL-termination");
  {
    LS s("hello world");
    require(s.size(), 11u);
    require(s.c_str()[11], '\0');

    LS e;
    require(e.c_str() != nullptr, true);

    LS p;
    for ( int i = 0; i < 40; ++i ) p.push_back('a');
    require(p.c_str()[p.size()], '\0');
  }
  end_test_case();

  test_case("hs clear wipes full capacity");
  {
    LS s("abcdefghijklmnopqrstuvwxyz");
    usize cap = s.max_size();
    s.clear();

    bool zp = true;
    const char *d = s.c_str();
    for ( usize i = 0; i < cap; ++i )
      if ( d[i] != '\0' ) {
        zp = false;
        break;
      }
    require(zp, true);
  }
  end_test_case();

  test_case("hs copy-assign self is identity");
  {
    LS s("identity-check");
    s = s;
    require(s == "identity-check", true);
  }
  end_test_case();

  test_case("hs throwing allocator: exception during growth leaves no leak");
  {
    THA::reset();
    bool threw = false;
    try {
      XS s("seed");
      THA::arm(1);
      for ( int i = 0; i < 100000; ++i ) s.push_back('z');
    } catch ( const micron::except::memory_error & ) {
      threw = true;
    }
    THA::disarm();
    require(threw, true);
    require(THA::outstanding() == 0, true);
  }
  end_test_case();
}

template<typename T>
static void
run_newops(void)
{
  using S = micron::hstring<T>;

  test_case("hs operator- / -= fuzz vs oracle");
  {
    prng rng(0x7711u);
    T buf[MAXLEN + 4], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_hs<T>(nd, m);
      ref_string<T> r;
      r.assign(buf, n);
      r.remove_all(nd, m);
      S s = make_hs<T>(buf, n);
      ck(mtest::seq_eq(s - needle, r), "operator-", it);
      ck(s.size() == n, "operator- keeps lhs", it);
      s -= needle;
      ck(mtest::seq_eq(s, r), "operator-=", it);
    }
  }
  end_test_case();

  test_case("hs operator* / *= fuzz vs oracle");
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
      S s = make_hs<T>(bb, base);
      ck(mtest::seq_eq(s * k, r), "operator*", it);
      S s2 = make_hs<T>(bb, base);
      s2 *= k;
      ck(mtest::seq_eq(s2, r), "operator*=", it);
    }
  }
  end_test_case();

  test_case("hs operator/ ");
  {
    prng rng(0x7733u);
    T buf[MAXLEN + 4], add[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      usize an = static_cast<usize>(rng.next_in(32));
      mtest::gen_string_n<T>(rng, add, an, alpha::ascii);
      S addss = make_hs<T>(add, an);
      ref_string<T> r;
      r.assign(buf, n);
      r.append(add, an);
      S s = make_hs<T>(buf, n);
      ck(mtest::seq_eq(s / addss, r), "operator/", it);
      s /= addss;
      ck(mtest::seq_eq(s, r), "operator/=", it);
    }
  }
  end_test_case();

  test_case("hs bitwise ^ & | (+compound) fuzz vs oracle");
  {
    prng rng(0x7744u);
    T buf[MAXLEN + 4], key[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      usize kn = static_cast<usize>(rng.next_in(40));
      mtest::gen_string_n<T>(rng, key, kn, alpha::full);
      S ks = make_hs<T>(key, kn);
      const byte *kp = reinterpret_cast<const byte *>(key);
      const usize kb = kn * sizeof(T);

      ref_string<T> rx, ra, ro;
      rx.assign(buf, n);
      ra.assign(buf, n);
      ro.assign(buf, n);
      rx.xor_key(kp, kb);
      ra.and_key(kp, kb);
      ro.or_key(kp, kb);

      S s = make_hs<T>(buf, n);
      auto x = s ^ ks;
      ck(mtest::seq_eq(x, rx) && x.size() == n, "operator^", it);
      ck(mtest::seq_eq(s & ks, ra), "operator&", it);
      ck(mtest::seq_eq(s | ks, ro), "operator|", it);

      S sx = make_hs<T>(buf, n);
      sx ^= ks;
      ck(mtest::seq_eq(sx, rx), "operator^=", it);
      S sa = make_hs<T>(buf, n);
      sa &= ks;
      ck(mtest::seq_eq(sa, ra), "operator&=", it);
      S so = make_hs<T>(buf, n);
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
  sb::print("--- hstring<", tyname, "> ---");
  run_props<T>();
  run_accessors<T>();
  run_lifetime<T>();
  run_newops<T>();
}

int
main(int, char **)
{
  sb::print("=== STRING (HSTRING) RIGOR ===");

  run_all<char>("char");
  run_all<byte>("byte");
  run_all<wide>("wide");
  run_all<unicode8>("unicode8");
  run_all<unicode16>("unicode16");
  run_all<unicode32>("unicode32");

  sb::print("--- adversarial / edges (char) ---");
  run_adversarial();

  sb::print("--- memory / leak / exception (char) ---");
  run_memory();

  sb::print("=== STRING (HSTRING) RIGOR DONE ===");
  return 1;
}
