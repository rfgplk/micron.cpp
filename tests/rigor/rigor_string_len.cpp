// rigor_string_len.cpp -- fixed_string's two lengths, and the generic string paths that read the
// wrong one.
//
// fixed_string<N> deliberately keeps TWO lengths and they are not the same thing:
//
//   size() == N-1, the CAPACITY. O(1), fixed by the type. regex.hpp sizes its constexpr VM arrays
//                  from it and the reflect suite asserts on it, so it does not move.
//   len()  == strnlen(buf, N), the CONTENT length.
//
// That was survivable while fixed_string was only an NTTP carrier. It stopped being survivable
// when the type gained c_str()/data()/cbegin()/cend() and the pointer/iterator typedefs, because
// that made it satisfy micron::is_string -- and every generic is_string consumer in the tree reads
// size() to get a byte count. micron::string(fixed_string<16>{"abc",3}) came out FIFTEEN bytes,
// "abc" followed by twelve embedded NULs, and compared unequal to "abc". format("{}", fs) did the
// same, as did every io write path.
//
// The fix is micron::string_len(t) in concepts.hpp: prefer len() where it exists. hstring,
// sstring, slice, span and rope all define len() == size(), so it is a no-op for them and exactly
// right for the one type where the two differ. This suite pins the invariant from both ends --
// that string_len is the content length for every string type, and that every generic consumer
// now agrees with c_str()/strlen.

#include "../../src/io/echo.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FUZZ = 3000;
#else
constexpr static const usize N_FUZZ = 40000;
#endif

// a sink that captures, so the echo path is compared byte for byte
struct capture_sink {
  micron::hstring<schar> out;

  max_t
  put(const char *p, usize n)
  {
    out.append(p, n);
    return static_cast<max_t>(n);
  }

  max_t
  put(char c)
  {
    out += c;
    return 1;
  }

  max_t
  flush(void)
  {
    return 0;
  }
};

// the oracle: strlen of c_str(), which is what a NUL-terminated string means by "its content"
static usize
oracle_len(const char *p) noexcept
{
  usize n = 0;
  while ( p[n] ) ++n;
  return n;
}

int
main()
{
  prng rng(0xfa11ed57a1c0de99ULL);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the split itself: size() is capacity, len() is content, and both still hold");
  {
    micron::fixed_string<16> fs("abc", 3);
    require_true(fs.size() == 15);      // regex.hpp and the reflect suite depend on this
    require_true(fs.len() == 3);
    require_true(fs.capacity() == 15);
    require_true(static_cast<usize>(fs.end() - fs.begin()) == 3);
    require_true(micron::string_len(fs) == 3);

    // a full buffer is the one case where they agree
    micron::fixed_string<4> full("abc");
    require_true(full.size() == 3 && full.len() == 3);
    require_true(micron::string_len(full) == 3);

    // and it still models the concept -- that is not what was wrong
    static_assert(micron::is_string<micron::fixed_string<4>>);
    static_assert(micron::is_string_v<micron::fixed_string<16>>);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("string_len is size() for every OTHER string type");
  {
    micron::hstring<char> h("hello");
    micron::sstring<32> s("hello");
    require_true(micron::string_len(h) == h.size() && h.size() == 5);
    require_true(micron::string_len(s) == s.size() && s.size() == 5);

    micron::hstring<char> empty;
    require_true(micron::string_len(empty) == 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the named conversions: no NUL padding, and equality holds");
  {
    micron::fixed_string<16> fs("abc", 3);

    micron::hstring<schar> f = micron::format::format("{}", fs);
    require_true(f.size() == 3);
    require_true(f[0] == 'a' && f[1] == 'b' && f[2] == 'c');

    micron::string str(fs);
    require_true(str.size() == 3);
    require_true(str == "abc");      // was FALSE: 15 bytes with 12 embedded NULs

    micron::sstr<32> ss(fs);
    require_true(ss.len() == 3);
    require_true(ss[0] == 'a' && ss[1] == 'b' && ss[2] == 'c');

    capture_sink cs;
    micron::io::__echo_impl::format_to_sink(cs, "{}", fs);
    require_true(cs.out.size() == 3);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: every generic path agrees with strlen(c_str()), at every fill level");
  {
    constexpr usize N = 24;
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      const usize fill = static_cast<usize>(rng.next() % N);      // 0 .. N-1 content bytes
      char raw[N];
      for ( usize k = 0; k < fill; ++k ) raw[k] = static_cast<char>('a' + (rng.next() % 26));
      for ( usize k = fill; k < N; ++k ) raw[k] = '\0';

      micron::fixed_string<N> fs(raw, fill);
      const usize want = oracle_len(fs.c_str());
      require_true(want == fill);

      require_true(micron::string_len(fs) == want);
      require_true(fs.len() == want);
      require_true(static_cast<usize>(fs.end() - fs.begin()) == want);
      require_true(fs.size() == N - 1);      // unchanged, always

      // format
      micron::hstring<schar> f = micron::format::format("{}", fs);
      require_true(f.size() == want);
      for ( usize k = 0; k < want; ++k ) require_true(f[k] == raw[k]);

      // hstring construction
      micron::string str(fs);
      require_true(str.size() == want);
      for ( usize k = 0; k < want; ++k ) require_true(str[k] == raw[k]);

      // sstring construction
      micron::sstr<64> ss(fs);
      require_true(ss.len() == want);
      for ( usize k = 0; k < want; ++k ) require_true(ss[k] == raw[k]);

      // the echo sink
      capture_sink cs;
      micron::io::__echo_impl::format_to_sink(cs, "{}", fs);
      require_true(cs.out.size() == want);
      for ( usize k = 0; k < want; ++k ) require_true(cs.out[k] == raw[k]);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: hstring and sstring round-trip unchanged through string_len");
  {
    for ( usize i = 0; i < N_FUZZ / 4; ++i ) {
      const usize n = static_cast<usize>(rng.next() % 40);
      micron::hstring<char> h;
      for ( usize k = 0; k < n; ++k ) h += static_cast<char>('a' + (rng.next() % 26));
      require_true(micron::string_len(h) == n);

      micron::string copy(h);
      require_true(copy.size() == n);
      micron::sstr<64> ss(h);
      require_true(ss.len() == n);
      require_true(micron::format::format("{}", h).size() == n);
    }
  }
  end_test_case();

  print("=== STRING LEN RIGOR SUITE PASSED ===");
  return 1;
}
