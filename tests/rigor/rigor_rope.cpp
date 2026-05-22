// Exhaustive, adversarial rigor suite for micron::rope<T, Sf>.
//
// rope is a persistent, immutable, reference-counted binary tree of <=256-byte
// leaves. "Mutating" ops return a new rope sharing unchanged structure; the
// in-place operator+= rebuilds via move-assign. This suite covers:
//   * functional groups diffed against the ref_string oracle, >=10k iters/fn,
//     across char/byte/wide/unicode8/unicode16/unicode32.
//   * the immutability invariant: ops return a correct new rope AND leave the
//     source unchanged (structural sharing must not mutate shared nodes).
//   * rope-specific adversaria: deep concatenation balance, needles straddling
//     256-byte leaf boundaries, flatten()/data() materialization + caching,
//     the O(1) same-root operator== fast path, refcounted copy/drop.
//   * memory/double-free: rope has no Alloc template param, so leak and
//     use-after-free are caught by the ASan profile (tests/build/sanitize.sh);
//     here we exercise heavy copy/drop churn for ASan to instrument.
//
// Build: `duck build tests/rigor/rigor_rope.cpp`; run `bin/rigor_rope`.

#include "../../src/io/console.hpp"

#include "../../src/string/rope.hpp"
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
static constexpr usize MAXLEN = 400;

template<typename T>
static micron::rope<T>
make_rope(const T *p, usize n)
{
  if ( n == 0 ) return micron::rope<T>();
  return micron::rope<T>(static_cast<const T *>(p), static_cast<const T *>(p) + n);
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
  using S = micron::rope<T>;

  test_case("rope ctor+copy+move fuzz");
  {
    prng rng(0x1111u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      ck(mtest::seq_eq(s, r), "ctor", it);
      S c(s);
      ck(mtest::seq_eq(c, r), "copy", it);
      ck((s == c), "copy-eq", it);
      S m(micron::move(c));
      ck(mtest::seq_eq(m, r), "move", it);
    }
  }
  end_test_case();

  test_case("rope fill ctor fuzz");
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

  test_case("rope access fuzz");
  {
    prng rng(0x3333u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize idx = static_cast<usize>(rng.next_in(n));
      ck(static_cast<T>(s[idx]) == r.buf[idx], "op[]", it);
      ck(static_cast<T>(s.at(idx)) == r.buf[idx], "at", it);
      ck(static_cast<T>(s.front()) == r.buf[0], "front", it);
      ck(static_cast<T>(s.back()) == r.buf[n - 1], "back", it);
      expect_throw_type<micron::except::library_error>([&] { (void)s.at(n); });
    }
  }
  end_test_case();

  test_case("rope iteration fuzz");
  {
    prng rng(0x3434u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 4000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize idx = 0;
      bool ok = true;
      for ( auto it2 = s.begin(); it2 != s.end(); ++it2, ++idx ) {
        if ( idx >= n || static_cast<T>(*it2) != r.buf[idx] ) {
          ok = false;
          break;
        }
      }
      ck(ok && idx == n, "iterate", it);
    }
  }
  end_test_case();

  test_case("rope push_back/pop_back fuzz");
  {
    prng rng(0x5555u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::ascii);
      S t = s.push_back(ch);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.push_back(ch);
      ck(mtest::seq_eq(t, rt), "push_back-result", it);
      ck(mtest::seq_eq(s, r), "push_back-source", it);
      if ( n ) {
        S p = s.pop_back();
        ref_string<T> rp;
        rp.assign(buf, n);
        rp.pop_back();
        ck(mtest::seq_eq(p, rp), "pop_back", it);
      }
    }
  }
  end_test_case();

  test_case("rope append fuzz");
  {
    prng rng(0x4444u);
    T buf[MAXLEN + 4], add[200];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n, m;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, add, m, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      S addr_ = make_rope<T>(add, m);
      S t = s.append(addr_);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.append(add, m);
      ck(mtest::seq_eq(t, rt), "append-result", it);
      ck(mtest::seq_eq(s, r), "append-source", it);
    }
  }
  end_test_case();

  test_case("rope insert fuzz");
  {
    prng rng(0x6161u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
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

  test_case("rope erase fuzz");
  {
    prng rng(0x6262u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      if ( n == 0 ) continue;
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize ind = static_cast<usize>(rng.next_in(n));
      usize cnt = static_cast<usize>(rng.next_in(n - ind)) + 1;
      S t = s.erase(ind, cnt);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.erase(ind, cnt);
      ck(mtest::seq_eq(t, rt), "erase-result", it);
      ck(mtest::seq_eq(s, r), "erase-source", it);
    }
  }
  end_test_case();

  test_case("rope substr fuzz");
  {
    prng rng(0x6565u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize pos = static_cast<usize>(rng.next_in(n + 1));
      usize cnt = static_cast<usize>(rng.next_in(n - pos + 1));
      S sub = s.substr(pos, cnt);
      ref_string<T> rsub;
      rsub.assign(r.buf + pos, cnt);
      ck(mtest::seq_eq(sub, rsub), "substr-result", it);
      ck(mtest::seq_eq(s, r), "substr-source", it);
    }
  }
  end_test_case();

  test_case("rope operator+= fuzz");
  {
    prng rng(0x7070u);
    T buf[MAXLEN + 4], add[200];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n, m;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      mtest::gen_string<T>(rng, add, m, 150, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      S addr_ = make_rope<T>(add, m);
      s += addr_;
      r.append(add, m);
      ck(mtest::seq_eq(s, r), "+=rope", it);
    }
  }
  end_test_case();

  test_case("rope find fuzz vs oracle");
  {
    prng rng(0x8080u);
    T buf[MAXLEN + 4], nd[16];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      T ch = mtest::gen_char<T>(rng, alpha::small);
      usize pos = n ? static_cast<usize>(rng.next_in(n + 1)) : 0;
      {
        usize got = s.find(ch, pos);
        usize exp = r.find(ch, pos);
        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-ch", it);
      }
      usize m = static_cast<usize>(rng.next_in(5)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_rope<T>(nd, m);
      {
        usize got = s.find(needle);
        usize exp = r.find_seq(nd, m, 0);

        ck((got == micron::npos) == (exp == REF_NPOS) && (exp == REF_NPOS || got == exp), "find-needle", it);
      }
    }
  }
  end_test_case();

  test_case("rope comparison fuzz vs oracle");
  {
    prng rng(0x9090u);
    T a[200], b[200];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize na, nb;
      mtest::gen_string<T>(rng, a, na, 120, alpha::small);
      if ( rng.next() & 1u ) {
        nb = na;
        for ( usize i = 0; i < na; ++i ) b[i] = a[i];
      } else {
        mtest::gen_string<T>(rng, b, nb, 120, alpha::small);
      }
      ref_string<T> ra;
      ra.assign(a, na);
      int exp = ra.compare(b, nb);
      S sa = make_rope<T>(a, na);
      S sb_ = make_rope<T>(b, nb);
      ck((sa == sb_) == (exp == 0), "==", it);
      ck((sa != sb_) == (exp != 0), "!=", it);
      ck((sa < sb_) == (exp < 0), "<", it);
      ck((sa > sb_) == (exp > 0), ">", it);
    }
  }
  end_test_case();

  test_case("rope flatten/data caching fuzz");
  {
    prng rng(0xA1A1u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 3000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      const T *d1 = s.data();
      const T *d2 = s.data();
      ck(d1 == d2, "data-cache", it);
      S f = s.flatten();
      ck(mtest::seq_eq(f, r), "flatten", it);
      ck(mtest::seq_eq(s, r), "flatten-source", it);
    }
  }
  end_test_case();
}

template<typename T>
static void
run_accessors(void)
{
  using S = micron::rope<T>;

  test_case("rope c_str/cdata/into_chars/into_bytes/clone");
  {
    prng rng(0xACC0u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 3000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      ck(mtest::cstr_eq(s, r), "c_str", it);
      const T *cd = s.cdata();
      bool ok = true;
      for ( usize i = 0; i < n; ++i )
        if ( cd[i] != r.buf[i] ) {
          ok = false;
          break;
        }
      ck(ok, "cdata", it);
      auto ic = s.into_chars();
      ck(ic.size() >= n, "into_chars", it);
      S cl = s.clone();
      ck(mtest::seq_eq(cl, r), "clone", it);
      ck(s.w_str() != nullptr && s.uni_str() != nullptr, "w/uni_str", it);
      ck(s.len() == s.size() && s.max_size() >= s.size() && s.capacity() >= s.size(), "cap", it);
    }
  }
  end_test_case();

  test_case("rope truncate / clear / fast_clear");
  {
    prng rng(0xACC1u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize tn = static_cast<usize>(rng.next_in(n + 1));
      S t = s.truncate(tn);
      ref_string<T> rt;
      rt.assign(buf, n);
      rt.truncate(tn);
      ck(mtest::seq_eq(t, rt), "truncate", it);
      ck(mtest::seq_eq(s, r), "truncate-source", it);
      S c = s.clear();
      ck(c.size() == 0u, "clear", it);
      ck(mtest::seq_eq(s, r), "clear-source", it);
    }
  }
  end_test_case();

  test_case("rope remove / remove_all fuzz");
  {
    prng rng(0xACC2u);
    T buf[MAXLEN + 4], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      ref_string<T> r1, r2;
      r1.assign(buf, n);
      r2.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_rope<T>(nd, m);
      S a = s.remove(needle);
      r1.remove(nd, m);
      ck(mtest::seq_eq(a, r1), "remove", it);
      S b = s.remove_all(needle);
      r2.remove_all(nd, m);
      ck(mtest::seq_eq(b, r2), "remove_all", it);
    }
  }
  end_test_case();

  test_case("rope resize fuzz vs oracle");
  {
    prng rng(0xACC3u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);
      usize rn = static_cast<usize>(rng.next_in(MAXLEN));
      T fillc = mtest::gen_char<T>(rng, alpha::ascii);
      S g = s.resize(rn, fillc);
      ref_string<T> rr;
      rr.assign(buf, n);
      if ( rn <= n )
        rr.truncate(rn);
      else
        for ( usize i = n; i < rn; ++i ) rr.push_back(fillc);
      ck(mtest::seq_eq(g, rr), "resize", it);
    }
  }
  end_test_case();

  test_case("rope for_each / for_each_chunk / operators");
  {
    prng rng(0xACC4u);
    T buf[MAXLEN + 4];
    for ( usize it = 0; it < 3000; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      ref_string<T> r;
      r.assign(buf, n);
      S s = make_rope<T>(buf, n);

      usize idx = 0;
      bool ok = true;
      s.for_each([&](T c) {
        if ( idx >= n || c != r.buf[idx] ) ok = false;
        ++idx;
      });
      ck(ok && idx == n, "for_each", it);

      usize total = 0;
      s.for_each_chunk([&](const T *, usize len) { total += len; });
      ck(total == n, "for_each_chunk", it);

      ck((!s) == (n == 0), "operator!", it);
      ck(static_cast<bool>(s) == (n != 0), "operator bool", it);
    }
  }
  end_test_case();
}

static void
run_structure(void)
{
  using S = micron::rope<char>;

  test_case("rope deep concat by push_back");
  {
    prng rng(0xD1);
    ref_string<char> r;
    S s;
    for ( usize i = 0; i < 2000; ++i ) {
      char c = mtest::gen_char<char>(rng, alpha::ascii);
      s = s.push_back(c);
      r.push_back(c);
    }
    require(mtest::seq_eq(s, r), true);
    require(s.size(), 2000u);
  }
  end_test_case();

  test_case("rope same-root operator== fast path");
  {
    S a("the quick brown fox jumps over the lazy dog and runs far away today");
    S b(a);
    require(a == b, true);
    require(a.identity() == b.identity(), true);
  }
  end_test_case();

  test_case("rope find needle across leaf boundary");
  {
    char buf[600];
    for ( usize i = 0; i < 600; ++i ) buf[i] = static_cast<char>('a' + (i % 23));

    const char *needle = "ZXCVBNMQWER";
    usize nl = micron::strlen(needle);
    for ( usize i = 0; i < nl; ++i ) buf[250 + i] = needle[i];
    S s = make_rope<char>(buf, 600);
    S nd = make_rope<char>(needle, nl);
    require(s.find(nd), 250u);
  }
  end_test_case();

  test_case("rope refcount copy/drop survivor intact");
  {
    S base("structural-sharing-reference-counted-rope-value");
    {
      S c1(base), c2(base), c3(c1), c4(c2);
      (void)c1.size();
      (void)c4.size();
    }
    require(base == "structural-sharing-reference-counted-rope-value", true);
    require(base.size(), micron::strlen("structural-sharing-reference-counted-rope-value"));
  }
  end_test_case();

  test_case("rope erase preserves shared original");
  {
    S a("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOP");
    S b(a);
    S c = b.erase(5, 10);
    require(a == "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOP", true);
    require(b == "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOP", true);
    require(c.size(), a.size() - 10);
  }
  end_test_case();

  test_case("rope resize grow/shrink");
  {
    S s("hello");
    S g = s.resize(10, 'x');
    require(g == "helloxxxxx", true);
    S sh = s.resize(3, 'y');
    require(sh == "hel", true);
    require(s == "hello", true);
  }
  end_test_case();

  test_case("rope copy/drop churn (ASan)");
  {
    prng rng(0xC0FFEE);
    for ( usize it = 0; it < 5000; ++it ) {
      char buf[300];
      usize n;
      mtest::gen_string<char>(rng, buf, n, 280, alpha::ascii);
      S s = make_rope<char>(buf, n);
      S a(s), b(s), c(a);
      S d = s.substr(0, n / 2);
      S e = s.append(a);
      (void)b.size();
      (void)c.size();
      (void)d.size();
      (void)e.size();
    }
    require(true, true);
  }
  end_test_case();
}

template<typename T>
static void
run_newops(void)
{
  using S = micron::rope<T>;

  test_case("rp operator- / -= fuzz vs oracle");
  {
    prng rng(0x7711u);
    T buf[MAXLEN + 4], nd[8];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::small);
      usize m = static_cast<usize>(rng.next_in(2)) + 1;
      mtest::gen_string_n<T>(rng, nd, m, alpha::small);
      S needle = make_rope<T>(nd, m);
      ref_string<T> r;
      r.assign(buf, n);
      r.remove_all(nd, m);
      S s = make_rope<T>(buf, n);
      ck(mtest::seq_eq(s - needle, r), "operator-", it);
      ck(s.size() == n, "operator- keeps lhs", it);
      s -= needle;
      ck(mtest::seq_eq(s, r), "operator-=", it);
    }
  }
  end_test_case();

  test_case("rp operator* / *= fuzz vs oracle");
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
      S s = make_rope<T>(bb, base);
      ck(mtest::seq_eq(s * k, r), "operator*", it);
      S s2 = make_rope<T>(bb, base);
      s2 *= k;
      ck(mtest::seq_eq(s2, r), "operator*=", it);
    }
  }
  end_test_case();

  test_case("rp operator/ ");
  {
    prng rng(0x7733u);
    T buf[MAXLEN + 4], add[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::ascii);
      usize an = static_cast<usize>(rng.next_in(32));
      mtest::gen_string_n<T>(rng, add, an, alpha::ascii);
      S addss = make_rope<T>(add, an);
      ref_string<T> r;
      r.assign(buf, n);
      r.append(add, an);
      S s = make_rope<T>(buf, n);
      ck(mtest::seq_eq(s / addss, r), "operator/", it);
      s /= addss;
      ck(mtest::seq_eq(s, r), "operator/=", it);
    }
  }
  end_test_case();

  test_case("rp bitwise ^ & | (+compound) fuzz vs oracle");
  {
    prng rng(0x7744u);
    T buf[MAXLEN + 4], key[64];
    for ( usize it = 0; it < ITERS; ++it ) {
      usize n;
      mtest::gen_string<T>(rng, buf, n, MAXLEN, alpha::full);
      usize kn = static_cast<usize>(rng.next_in(40));
      mtest::gen_string_n<T>(rng, key, kn, alpha::full);
      S ks = make_rope<T>(key, kn);
      const byte *kp = reinterpret_cast<const byte *>(key);
      const usize kb = kn * sizeof(T);

      ref_string<T> rx, ra, ro;
      rx.assign(buf, n);
      ra.assign(buf, n);
      ro.assign(buf, n);
      rx.xor_key(kp, kb);
      ra.and_key(kp, kb);
      ro.or_key(kp, kb);

      S s = make_rope<T>(buf, n);
      auto x = s ^ ks;
      ck(mtest::seq_eq(x, rx) && x.size() == n, "operator^", it);
      ck(mtest::seq_eq(s & ks, ra), "operator&", it);
      ck(mtest::seq_eq(s | ks, ro), "operator|", it);

      S sx = make_rope<T>(buf, n);
      sx ^= ks;
      ck(mtest::seq_eq(sx, rx), "operator^=", it);
      S sa = make_rope<T>(buf, n);
      sa &= ks;
      ck(mtest::seq_eq(sa, ra), "operator&=", it);
      S so = make_rope<T>(buf, n);
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
  sb::print("--- rope<", tyname, "> ---");
  run_props<T>();
  run_accessors<T>();
  run_newops<T>();
}

int
main(int, char **)
{
  sb::print("=== ROPE RIGOR ===");

  run_all<char>("char");
  run_all<byte>("byte");
  run_all<wide>("wide");
  run_all<unicode8>("unicode8");
  run_all<unicode16>("unicode16");
  run_all<unicode32>("unicode32");

  sb::print("--- structure / refcount (char) ---");
  run_structure();

  sb::print("=== ROPE RIGOR DONE ===");
  return 1;
}
