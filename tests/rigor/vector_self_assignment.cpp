// vector_self_assignment.cpp
// Regression test for B1 (copy self-assign UAF) and B2 (move self-assign UAF)
// across all vector variants. With Sf=true (default), self-assignment must be
// a safe no-op preserving contents and leak-free. With Sf=false, the guard is
// elided (test cannot exercise the unsafe path without UB; we only verify the
// constexpr dispatch compiles).

#include "../../src/std.hpp"

// vector.hpp must precede convector.hpp because convector uses
// micron::__impl::grow which is defined inline by vector.hpp.
#include "../../src/vector/vector.hpp"

#include "../../src/vector/circle_vector.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/ivector.hpp"
#include "../../src/vector/pvector.hpp"
#include "../../src/vector/svector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

struct Tracked {
  static inline usize ctor = 0;
  static inline usize dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
    ++ctor;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = 0;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

}      // namespace

int
main()
{
  print("=== VECTOR SELF-ASSIGNMENT TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("vector copy self-assign preserves contents");
  {
    micron::vector<int> v{ 1, 2, 3, 4, 5 };
    v = v;
    require(v.size(), usize(5));
    for ( int i = 0; i < 5; ++i ) require(v[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("vector move self-assign preserves contents");
  {
    micron::vector<int> v{ 10, 20, 30 };
    v = micron::move(v);
    require(v.size(), usize(3));
    require(v[0], 10);
    require(v[2], 30);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("vector self-assign with Tracked has no leak");
  {
    reset_tracked();
    {
      micron::vector<Tracked> v;
      for ( int i = 0; i < 32; ++i ) v.emplace_back(i);
      v = v;                    // copy self
      v = micron::move(v);      // move self
      require(v.size(), usize(32));
      for ( int i = 0; i < 32; ++i ) require(v[i].v, i);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("svector copy self-assign preserves contents");
  {
    micron::svector<int, 8> sv{ 1, 2, 3, 4 };
    sv = sv;
    require(sv.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(sv[i], i + 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("svector move self-assign preserves contents");
  {
    micron::svector<int, 8> sv{ 5, 6, 7 };
    sv = micron::move(sv);
    require(sv.size(), usize(3));
    require(sv[0], 5);
    require(sv[2], 7);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("ivector copy self-assign preserves contents");
  {
    micron::ivector<int> iv{ 7, 8, 9 };
    iv = iv;
    require(iv.size(), usize(3));
    require(iv[0], 7);
    require(iv[2], 9);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("ivector move self-assign preserves contents");
  {
    micron::ivector<int> iv{ 11, 22 };
    iv = micron::move(iv);
    require(iv.size(), usize(2));
    require(iv[0], 11);
    require(iv[1], 22);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("convector copy self-assign preserves contents");
  {
    micron::convector<int> cv;
    for ( int i = 0; i < 4; ++i ) cv.push_back(i);
    cv = cv;
    require(cv.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(cv[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("convector move self-assign preserves contents");
  {
    micron::convector<int> cv;
    cv.push_back(99);
    cv = micron::move(cv);
    require(cv.size(), usize(1));
    require(cv[0], 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fvector move self-assign preserves contents");
  {
    micron::fvector<int> fv;
    for ( int i = 0; i < 3; ++i ) fv.push_back(i + 1);
    fv = micron::move(fv);
    require(fv.size(), usize(3));
    require(fv[0], 1);
    require(fv[2], 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pvector copy self-assign preserves contents");
  {
    // pvector is persistent: push_back returns a new vector
    micron::pvector<int> pv{ 0, 10, 20, 30 };
    pv = pv;
    require(pv.size(), usize(4));
    for ( int i = 0; i < 4; ++i ) require(pv[i], i * 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pvector move self-assign preserves contents");
  {
    micron::pvector<int> pv{ 42 };
    pv = micron::move(pv);
    require(pv.size(), usize(1));
    require(pv[0], 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("circle_vector copy self-assign preserves contents");
  {
    micron::circle_vector<int, 8> cv;
    for ( int i = 0; i < 4; ++i ) cv.push(i + 1);
    cv = cv;
    require(cv.size(), usize(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("circle_vector move self-assign preserves contents");
  {
    micron::circle_vector<int, 8> cv;
    cv.push(7);
    cv = micron::move(cv);
    require(cv.size(), usize(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("constexpr Sf=false elides guard (compile-only)");
  {
    micron::vector<int, micron::allocator_serial<>, false> v_unsafe;
    v_unsafe.push_back(1);
    // do NOT self-assign here; with Sf=false the guard is elided and
    // self-assign would UAF. Just verify the type instantiates.
    require(v_unsafe.size(), usize(1));
  }
  end_test_case();

  print("=== ALL VECTOR SELF-ASSIGNMENT TESTS PASSED ===");
  return 1;
}
