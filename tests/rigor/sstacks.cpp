// micron::fsstack_snowball_tests.cpp
// Rigorous test suite for micron::fsstack<T, N> using snowball

#include "../snowball/snowball.hpp"

#include "../../src/stack.hpp"
#include "../../src/std.hpp"
#include "../../src/io/console.hpp"

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

using sb::print;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

using umax_t = std::size_t;

// ---------------- helpers ----------------

struct Trivial {
  int v;
  constexpr Trivial(int x = 0) : v(x) {}
  constexpr bool
  operator==(const Trivial &o) const
  {
    return v == o.v;
  }
};

struct NonTrivial {
  int v;
  static inline int ctor = 0;
  static inline int dtor = 0;
  static inline int copy = 0;
  static inline int move = 0;

  NonTrivial(int x = 0) : v(x) { ++ctor; }
  NonTrivial(const NonTrivial &o) : v(o.v) { ++copy; }
  NonTrivial(NonTrivial &&o) noexcept : v(o.v)
  {
    o.v = -1;
    ++move;
  }
  ~NonTrivial() { ++dtor; }

  bool
  operator==(const NonTrivial &o) const
  {
    return v == o.v;
  }

  static void
  reset()
  {
    ctor = dtor = copy = move = 0;
  }
};

// ---------------- tests ----------------

template <typename T, size_t N>
void
test_default_ctor()
{
  test_case("default ctor");
  micron::fsstack<T, N> s;
  require_true(s.empty());
  require(s.size(), size_t(0));
  require(s.max_size(), size_t(N));
  end_test_case();
}

template <typename T, size_t N>
void
test_size_ctor()
{
  test_case("size ctor");
  micron::fsstack<T, N> s(umax_t(N / 2));
  require(s.size(), size_t(N / 2));
  require_false(s.empty());
  end_test_case();
}

template <typename T, size_t N>
void
test_init_list()
{
  test_case("initializer_list ctor");
  micron::fsstack<T, N> s{ T(1), T(2), T(3) };
  require(s.size(), size_t(3));
  require(s[0], T(3));
  require(s[1], T(2));
  require(s[2], T(1));
  require(s.top(), T(3));
  end_test_case();
}

template <typename T, size_t N>
void
test_copy_ctor()
{
  test_case("copy ctor");
  micron::fsstack<T, N> a{ T(1), T(2) };
  micron::fsstack<T, N> b(a);
  require(b.size(), a.size());
  require(b[0], T(2));
  require(b[1], T(1));
  end_test_case();
}

template <typename T, size_t N>
void
test_move_ctor()
{
  test_case("move ctor");
  micron::fsstack<T, N> a{ T(1), T(2), T(3) };
  micron::fsstack<T, N> b(std::move(a));
  require(b.size(), size_t(3));
  require(b.top(), T(3));
  end_test_case();
}

template <typename T, size_t N>
void
test_copy_assign()
{
  test_case("copy assignment");
  micron::fsstack<T, N> a{ T(4), T(5) };
  micron::fsstack<T, N> b;
  b = a;
  require(b.size(), size_t(2));
  require(b[0], T(5));
  end_test_case();
}

template <typename T, size_t N>
void
test_move_assign()
{
  test_case("move assignment");
  micron::fsstack<T, N> a{ T(7), T(8) };
  micron::fsstack<T, N> b;
  b = std::move(a);
  require(b.size(), size_t(2));
  require(b.top(), T(8));
  end_test_case();
}

template <typename T, size_t N>
void
test_push_pop()
{
  test_case("push/pop");
  micron::fsstack<T, N> s;
  for ( size_t i = 0; i < N; ++i )
    s.push(T(int(i)));
  require(s.size(), size_t(N));
  for ( size_t i = N; i-- > 0; ) {
    require(s.top(), T(int(i)));
    s.pop();
  }
  require_true(s.empty());
  end_test_case();
}

template <typename T, size_t N>
void
test_emplace()
{
  test_case("emplace");
  micron::fsstack<T, N> s;
  s.emplace(42);
  require(s.size(), size_t(1));
  require(s.top(), T(42));
  end_test_case();
}

template <typename T, size_t N>
void
test_operator_call()
{
  test_case("operator()");
  micron::fsstack<T, N> s{ T(1), T(2) };
  T v = s();
  require(v, T(2));
  require(s.size(), size_t(1));
  end_test_case();
}

template <typename T, size_t N>
void
test_clear()
{
  test_case("clear");
  micron::fsstack<T, N> s{ T(1), T(2), T(3) };
  s.clear();
  require_true(s.empty());
  require(s.size(), size_t(0));
  end_test_case();
}

template <size_t N>
void
test_nontrivial_lifetime()
{
  test_case("non-trivial lifetime");
  NonTrivial::reset();
  {
    micron::fsstack<NonTrivial, N> s;
    s.emplace(1);
    s.emplace(2);
    require(NonTrivial::ctor, 2);
    s.pop();
    s.pop();
  }
  require(NonTrivial::dtor, NonTrivial::ctor + NonTrivial::copy + NonTrivial::move);
  end_test_case();
}

template <typename T, size_t N>
void
stress_lifo()
{
  test_case("stress LIFO");
  micron::fsstack<T, N> s;
  for ( size_t r = 0; r < 1000; ++r ) {
    for ( size_t i = 0; i < N; ++i )
      s.push(T(int(i)));
    for ( size_t i = N; i-- > 0; ) {
      require(s.top(), T(int(i)));
      s.pop();
    }
    require_true(s.empty());
  }
  end_test_case();
}

// ---------------- runner ----------------

int
main()
{
  print("=== SSTACK TESTS ===");
  test_default_ctor<int, 16>();
  test_size_ctor<int, 16>();
  test_init_list<int, 16>();
  test_copy_ctor<int, 16>();
  test_move_ctor<int, 16>();
  test_copy_assign<int, 16>();
  test_move_assign<int, 16>();
  test_push_pop<int, 16>();
  test_emplace<int, 16>();
  test_operator_call<int, 16>();
  test_clear<int, 16>();

  //test_nontrivial_lifetime<16>();
  stress_lifo<int, 32>();
  print("=== ALL STACK TESTS PASSED ===");

  return 1;
}
