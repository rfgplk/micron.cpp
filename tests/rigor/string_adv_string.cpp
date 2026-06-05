

#include "../../src/io/console.hpp"

#include "../../src/string/string.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

using namespace snowball;

using S = micron::hstring<char>;

static bool
terminated(const S &s)
{
  return micron::strlen(s.c_str()) == s.size();
}

static void
insert_index_bound(void)
{
  test_case("insert(ind>length) throws, no wild memmove");
  {

    S a("abc");
    expect_throw_type<micron::except::library_error>([&] { a.insert((usize)100, 'X', 1); });
    require(a == "abc");
    require(terminated(a));

    S b("abc");
    expect_throw_type<micron::except::library_error>([&] { b.insert((usize)100, "XY", 1); });
    require(b == "abc");
    require(terminated(b));

    S c("abc");
    micron::sstring<8, char> ss("zz");
    expect_throw_type<micron::except::library_error>([&] { c.insert((usize)100, ss); });
    require(c == "abc");
    require(terminated(c));

    S d("abc");
    d.insert((usize)3, 'X', 2);
    require(d == "abcXX");
    require(terminated(d));
    require(d.size() == 5u);
  }
  end_test_case();
}

static void
insert_index_array_oob(void)
{
  test_case("insert(ind, array, large cnt) no OOB write");
  {
    S s("start");
    s.insert((usize)2, "XYZ", 1000);
    require(s.size() == 5u + 3000u);
    require(terminated(s));

    require(s[0] == 's' && s[1] == 't');
    require(s[2] == 'X' && s[3] == 'Y' && s[4] == 'Z');
    require(s[2 + 3000] == 'a');
    require(s[s.size() - 1] == 't');
  }
  end_test_case();

  test_case("insert(ind, array, cnt) overflow count throws");
  {
    S s("hello");

    expect_throw_type<micron::except::library_error>([&] { s.insert((usize)1, "XY", micron::numeric_limits<usize>::max() - 1); });
    require(s == "hello");
    require(terminated(s));
  }
  end_test_case();
}

static void
insert_iter_array_oob(void)
{
  test_case("insert(itr, array, large cnt) no OOB write");
  {
    S s("start");
    s.insert(s.begin() + 2, "XYZ", 1000);
    require(s.size() == 5u + 3000u);
    require(terminated(s));
    require(s[2] == 'X' && s[3] == 'Y' && s[4] == 'Z');
    require(s[2 + 3000] == 'a');
    require(s[s.size() - 1] == 't');
  }
  end_test_case();
}

static void
pluseq_empty_slice(void)
{
  test_case("operator+=(empty slice) stays in-bounds");
  {
    S s("abc");
    micron::slice<char> empty(s.data(), s.data());
    require(empty.size() == 0u);
    s += empty;
    require(s == "abc");
    require(terminated(s));
    require(s.size() == 3u);
  }
  end_test_case();

  test_case("operator+=(non-empty slice) appends all chars (no off-by-one)");
  {
    S src("XYZW");
    micron::slice<char> sl(src.data(), src.data() + 4);
    require(sl.size() == 4u);
    S s("abc");
    s += sl;
    require(s.size() == 7u);
    require(s == "abcXYZW");
    require(terminated(s));
  }
  end_test_case();
}

static void
insert_iter_sstring_empty(void)
{
  test_case("insert(itr, empty sstring) stays in-bounds (lvalue)");
  {
    S s("abc");
    micron::sstring<8, char> e;
    require(e.size() == 0u);
    s.insert(s.begin() + 1, e);
    require(s == "abc");
    require(terminated(s));
    require(s.size() == 3u);
  }
  end_test_case();

  test_case("insert(itr, empty sstring) stays in-bounds (rvalue)");
  {
    S s("abc");
    micron::sstring<8, char> e;
    s.insert(s.begin() + 1, micron::move(e));
    require(s == "abc");
    require(terminated(s));
    require(s.size() == 3u);
  }
  end_test_case();

  test_case("insert(itr, non-empty sstring) inserts whole content");
  {
    S s("abc");
    micron::sstring<8, char> e("ZZ");
    s.insert(s.begin() + 1, e);
    require(s == "aZZbc");
    require(terminated(s));
    require(s.size() == 5u);

    S t("abc");
    micron::sstring<8, char> e2("QQ");
    t.insert(t.begin() + 1, micron::move(e2));
    require(t == "aQQbc");
    require(terminated(t));
  }
  end_test_case();
}

static void
nul_termination(void)
{
  test_case("substr(0,5).c_str() is NUL-terminated");
  {
    S s("abcdefghij");
    auto sub = s.substr(0, 5);
    require(sub.size() == 5u);
    require(terminated(sub));
    require(sub == "abcde");
  }
  end_test_case();

  test_case("substr(iter,iter).c_str() is NUL-terminated");
  {
    S s("abcdefghij");
    auto sub = s.substr(s.begin() + 2, s.begin() + 7);
    require(sub.size() == 5u);
    require(terminated(sub));
    require(sub == "cdefg");
  }
  end_test_case();

  test_case("append after fast_clear is NUL-terminated (no residual)");
  {
    S s("AAAAAAAAAAAAAAAAAAAA");
    s.fast_clear();
    s.append("bc", 2);
    require(s.size() == 2u);
    require(terminated(s));
    require(s == "bc");
  }
  end_test_case();

  test_case("end-insert leaves a terminator");
  {
    S s("abc");
    s.insert((usize)3, 'X', 2);
    require(s.size() == 5u);
    require(terminated(s));
    require(s == "abcXX");
  }
  end_test_case();

  test_case("converting ctor from sstring is NUL-terminated");
  {
    micron::sstring<16, char> ss("abcdefg");
    S h(ss);
    require(h.size() == 7u);
    require(terminated(h));
    require(h == "abcdefg");
  }
  end_test_case();

  test_case("cross-width copy ctor is NUL-terminated");
  {
    S a("hello world");
    micron::hstring<char> b(a);
    require(b.size() == 11u);
    require(terminated(b));
    require(b == "hello world");
  }
  end_test_case();

  test_case("append/operator+= chains keep terminator");
  {
    S s;
    for ( int i = 0; i < 200; ++i ) {
      s.append("xy", 2);
      require(terminated(s));
    }
    require(s.size() == 400u);
    S t("seed");
    micron::sstring<8, char> add("AB");
    for ( int i = 0; i < 50; ++i ) {
      t += add;
      require(terminated(t));
    }
    require(t.size() == 4u + 100u);
  }
  end_test_case();
}

static void
substr_overflow(void)
{
  test_case("substr overflow args throw (no wrap bypass)");
  {
    S s("abcdef");

    expect_throw_type<micron::except::library_error>([&] { (void)s.substr((usize)3, micron::numeric_limits<usize>::max() - 1); });

    auto ok = s.substr(2, 3);
    require(ok == "cde");
    require(terminated(ok));

    expect_throw_type<micron::except::library_error>([&] { (void)s.substr((usize)100, (usize)1); });

    auto empty = s.substr((usize)6, (usize)0);
    require(empty.size() == 0u);
    require(terminated(empty));
  }
  end_test_case();
}

static void
reserve_honors_n(void)
{
  test_case("reserve(n) on moved-from string honors n");
  {
    S a("seed");
    S b(micron::move(a));
    require(a.size() == 0u);
    a.reserve(10000);
    require(a.max_size() >= 10000u);

    a.append("z", 1);
    require(a == "z");
    require(terminated(a));
  }
  end_test_case();

  test_case("try_reserve(n) on moved-from string honors n");
  {
    S a("seed");
    S b(micron::move(a));
    require(a.size() == 0u);
    a.try_reserve(8192);
    require(a.max_size() >= 8192u);
  }
  end_test_case();
}

static void
pop_back(void)
{
  test_case("pop_back drops the last char from c_str");
  {
    S s("abc");
    s.pop_back();
    require(s.size() == 2u);
    require(terminated(s));
    require(s == "ab");
    s.pop_back();
    require(s == "a");
    require(terminated(s));
    s.pop_back();
    require(s.size() == 0u);
    require(terminated(s));
    s.pop_back();
    require(s.size() == 0u);
  }
  end_test_case();
}

static void
sanity_roundtrip(void)
{
  test_case("sanity insert/erase/substr roundtrip");
  {
    S s("Hello");
    s.insert((usize)5, ", world", 1);
    require(s == "Hello, world");
    require(terminated(s));
    auto sub = s.substr(7, 5);
    require(sub == "world");
    require(terminated(sub));
    s.insert(s.begin(), ">>", 1);
    require(s == ">>Hello, world");
    require(terminated(s));
  }
  end_test_case();
}

int
main(int, char **)
{
  sb::print("=== STRING ADV (string.hpp) ===");

  insert_index_bound();
  insert_index_array_oob();
  insert_iter_array_oob();
  pluseq_empty_slice();
  insert_iter_sstring_empty();
  nul_termination();
  substr_overflow();
  reserve_honors_n();
  pop_back();
  sanity_roundtrip();

  sb::print("ALL STRING ADV PASSED");
  return 1;
}
