

#include "../../src/io/console.hpp"

#include "../../src/string/__old_sstring.hpp"
#include "../../src/string/__old_string.hpp"

#include "../../src/string/sstring.hpp"
#include "../../src/string/string.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

using new_hstr = micron::hstring<char>;
using old_hstr = micron::__old::hstring<char>;
template <usize N> using new_sstr = micron::sstring<N, char>;
template <usize N> using old_sstr = micron::__old::sstring<N, char>;

static char
pat(usize i)
{
  static constexpr const char *src = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  return src[i % 62];
}

static void
fill_pat(char *dst, usize n)
{
  for ( usize i = 0; i < n; ++i ) dst[i] = pat(i);
  dst[n] = 0;
}

int
main(int, char **)
{
  sb::print("=== STRING SIMD TESTS ===");

  test_case("__lexcmp differential: lengths and diff positions");
  {
    constexpr usize lens[] = { 0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 1023 };
    char a[1024], b[1024];
    for ( usize L : lens ) {
      fill_pat(a, L);
      fill_pat(b, L);

      new_sstr<1024> na(a), nb(b);
      old_sstr<1024> oa(a), ob(b);
      require((na == nb), (oa == ob));

      const usize diffs[] = { 0, L / 2, L ? L - 1 : 0 };
      for ( usize k : diffs ) {
        if ( k >= L ) continue;
        b[k] = static_cast<char>(b[k] ^ 0x01);
        new_sstr<1024> nb2(b);
        old_sstr<1024> ob2(b);
        require((na == nb2), (oa == ob2));
        require((na < nb2), (oa < ob2));
        require((na > nb2), (oa > ob2));
        b[k] = static_cast<char>(b[k] ^ 0x01);
      }
    }
  }
  end_test_case();

  test_case("find(char) differential: hit positions across SIMD blocks");
  {
    char buf[1024];
    fill_pat(buf, 1023);
    new_sstr<1024> ns(buf);
    old_sstr<1024> os(buf);
    constexpr usize hits[] = { 0, 7, 15, 16, 17, 31, 32, 33, 63, 64, 100, 511, 1000, 1022 };
    for ( usize p : hits ) {
      char c = pat(p);
      require(ns.find(c), os.find(c));
    }

    require(ns.find('!'), os.find('!'));
    require(ns.find('#'), os.find('#'));
  }
  end_test_case();

  test_case("sstring find() does not over-scan past length");
  {
    new_sstr<128> s("hello");

    s.data()[s.size()] = 'Z';
    s.data()[s.size() + 1] = 0;

    require(s.find('Z'), micron::npos);
  }
  end_test_case();

  test_case("find_substr differential: needle lengths × hit positions");
  {
    char buf[1024];
    fill_pat(buf, 1023);
    new_sstr<1024> ns(buf);
    old_sstr<1024> os(buf);
    constexpr usize nlens[] = { 1, 2, 4, 8, 15, 16, 17, 31, 32, 33 };
    constexpr usize starts[] = { 0, 7, 15, 16, 17, 31, 32, 100, 511, 900 };
    for ( usize nl : nlens )
      for ( usize st : starts ) {
        if ( st + nl >= 1023 ) continue;
        char needle[64] = { 0 };
        for ( usize i = 0; i < nl; ++i ) needle[i] = pat(st + i);
        require(ns.find_substr(needle, nl), os.find_substr(needle, nl));
      }

    require(ns.find_substr("", 0), micron::npos);
    require(os.find_substr("", 0), micron::npos);

    require(ns.find_substr("!!!", 3), os.find_substr("!!!", 3));

    char big[2048];
    for ( usize i = 0; i < 2047; ++i ) big[i] = pat(i);
    big[2047] = 0;
    require(ns.find_substr(big, 2047), os.find_substr(big, 2047));
  }
  end_test_case();

  test_case("starts_with / ends_with differential");
  {
    char buf[256];
    fill_pat(buf, 255);
    new_sstr<256> ns(buf);
    old_sstr<256> os(buf);
    constexpr usize plens[] = { 0, 1, 8, 15, 16, 17, 31, 32, 33, 255 };
    for ( usize pl : plens ) {
      char pre[64] = { 0 };
      for ( usize i = 0; i < pl && i < 63; ++i ) pre[i] = pat(i);
      if ( pl <= 63 ) {
        require(ns.starts_with(pre), os.starts_with(pre));
      }
      char suf[64] = { 0 };
      if ( pl <= 63 ) {
        for ( usize i = 0; i < pl; ++i ) suf[i] = pat(255 - pl + i);
        require(ns.ends_with(suf), os.ends_with(suf));
      }
    }

    require(ns.starts_with("xyz"), os.starts_with("xyz"));
    require(ns.ends_with("xyz"), os.ends_with("xyz"));
  }
  end_test_case();

  test_case("to_lower / to_upper differential");
  {
    constexpr usize lens[] = { 0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 250 };
    for ( usize L : lens ) {
      char buf[256];
      fill_pat(buf, L);
      new_sstr<256> ns(buf);
      old_sstr<256> os(buf);
      ns.to_lower();
      os.to_lower();
      for ( usize i = 0; i < L; ++i ) require(ns.data()[i], os.data()[i]);
      ns.to_upper();
      os.to_upper();
      for ( usize i = 0; i < L; ++i ) require(ns.data()[i], os.data()[i]);
    }

    char buf[256];
    for ( usize i = 0; i < 200; ++i ) buf[i] = static_cast<char>(i + 1);
    buf[200] = 0;
    new_sstr<256> ns(buf);
    ns.to_lower();
    for ( usize i = 0; i < 200; ++i ) {
      char in = static_cast<char>(i + 1);
      char expected = (in >= 'A' && in <= 'Z') ? static_cast<char>(in + 32) : in;
      require(ns.data()[i], expected);
    }
  }
  end_test_case();

  test_case("find_first_of differential: small + large charsets");
  {
    char buf[256];
    fill_pat(buf, 255);
    new_sstr<256> ns(buf);
    old_sstr<256> os(buf);
    require(ns.find_first_of(" \t"), os.find_first_of(" \t"));
    require(ns.find_first_of("xyzA"), os.find_first_of("xyzA"));
    require(ns.find_first_of("aeiouAEIOU"), os.find_first_of("aeiouAEIOU"));

    char buf2[256];
    fill_pat(buf2, 255);
    buf2[32] = '!';
    new_sstr<256> ns2(buf2);
    old_sstr<256> os2(buf2);
    require(ns2.find_first_of("!@#"), os2.find_first_of("!@#"));
  }
  end_test_case();

  test_case("count(ch) differential across SIMD boundaries");
  {
    constexpr usize lens[] = { 0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 1023 };
    for ( usize L : lens ) {
      char buf[1024];
      fill_pat(buf, L);
      new_sstr<1024> ns(buf);
      old_sstr<1024> os(buf);
      require(ns.count('a'), os.count('a'));
      require(ns.count('Z'), os.count('Z'));
      require(ns.count('0'), os.count('0'));
    }
  }
  end_test_case();

  test_case("reverse: parity and known cases");
  {
    constexpr usize lens[] = { 0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127 };
    for ( usize L : lens ) {
      char buf[256];
      fill_pat(buf, L);
      new_sstr<256> ns(buf);
      new_sstr<256> orig(buf);
      ns.reverse().reverse();
      require(ns == orig, true);
    }
    new_sstr<16> r("abcdef");
    r.reverse();
    require(r == "fedcba", true);
  }
  end_test_case();

  test_case("trim_left / trim_right differential");
  {
    new_sstr<256> a("   \t\nhello   \r\n");
    old_sstr<256> b("   \t\nhello   \r\n");
    a.trim_left();
    b.trim_left();
    require(a.size(), b.size());
    for ( usize i = 0; i < a.size(); ++i ) require(a.data()[i], b.data()[i]);
    a.trim_right();
    b.trim_right();
    require(a.size(), b.size());
    require(a == "hello", true);
  }
  end_test_case();

  test_case("sstring insert(iterator,char,cnt) matches index-based");
  {
    new_sstr<64> a("abcdef");
    new_sstr<64> b("abcdef");
    a.insert(static_cast<usize>(3), 'X', 2);
    b.insert(b.begin() + 3, 'X', 2);
    require(a.size(), b.size());
    for ( usize i = 0; i < a.size(); ++i ) require(a.data()[i], b.data()[i]);
    require(a == "abcXXdef", true);
    require(b == "abcXXdef", true);
  }
  end_test_case();

  test_case("hstring find(hstring) returns real position");
  {
    new_hstr h("the quick brown fox jumps over the lazy dog");
    new_hstr needle("fox");
    require(h.find(needle), usize{ 16 });
    new_hstr missing("zebra");
    require(h.find(missing), micron::npos);
  }
  end_test_case();

  test_case("clear() zeroes whole capacity (invariant)");
  {
    new_hstr h;
    h.reserve(256);
    for ( usize i = 0; i < 200; ++i ) h.push_back(pat(i));
    require(h.size(), usize{ 200 });
    char *raw = h.data();
    h.clear();
    require(h.size(), usize{ 0 });

    for ( usize i = 0; i < 200; ++i ) require(raw[i], char{ 0 });
  }
  end_test_case();

  test_case("hstring push_back(sstring) advances length correctly");
  {
    new_hstr h("AB");
    new_sstr<32> s("CD");
    h.push_back(s);

    require(h.size() >= 4u, true);
    require(h.data()[0], 'A');
    require(h.data()[1], 'B');
    require(h.data()[2], 'C');
    require(h.data()[3], 'D');
  }
  end_test_case();

  test_case("hstring insert(iterator,hstring) preserves last byte");
  {
    new_hstr h("AB");
    h.reserve(64);
    new_hstr ins("CDE");
    h.insert(h.begin() + 1, ins);
    require(h.size() >= 5u, true);
    require(h.data()[0], 'A');
    require(h.data()[1], 'C');
    require(h.data()[2], 'D');
    require(h.data()[3], 'E');
    require(h.data()[4], 'B');
  }
  end_test_case();

  sb::print("=== ALL STRING SIMD TESTS PASSED ===");
  return 0;
}
