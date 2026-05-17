// deep_move_safety.cpp
// Regression test for B4: __impl_container::deep_move now provides a
// dest-rollback basic guarantee when T's move constructor can throw. If the
// move-ctor throws at iteration i, dest[0..i-1] are destroyed and src[i..cnt-1]
// remain live; the exception is rethrown.

#include "../../src/std.hpp"

#include "../../src/bits/__container.hpp"
#include "../../src/except.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_throw;
using sb::test_case;

namespace
{

// Throws on the k-th throwing-move (controlled by static state).
// Tracks ctor/dtor balance.
struct Bomb {
  static inline usize ctor = 0;
  static inline usize dtor = 0;
  static inline int throw_at = -1;      // -1 = never throw
  static inline int move_count = 0;

  int v;

  Bomb() : v(0) { ++ctor; }

  explicit Bomb(int x) : v(x) { ++ctor; }

  Bomb(const Bomb &o) : v(o.v) { ++ctor; }

  Bomb(Bomb &&o) : v(o.v)
  {
    if ( move_count++ == throw_at ) {
      throw micron::runtime{ "Bomb: configured move throw" };
    }
    o.v = -1;
    ++ctor;
  }

  ~Bomb() { ++dtor; }

  static void
  reset(int trip = -1)
  {
    ctor = 0;
    dtor = 0;
    move_count = 0;
    throw_at = trip;
  }
};

}      // namespace

int
main()
{
  print("=== DEEP_MOVE SAFETY TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("noexcept fast path: trivial type leaves no garbage");
  {
    int src[5] = { 10, 20, 30, 40, 50 };
    alignas(int) byte dest_buf[sizeof(int) * 5];
    int *dest = reinterpret_cast<int *>(dest_buf);

    micron::__impl_container::deep_move<int>(dest, src, 5);
    for ( int i = 0; i < 5; ++i ) require(dest[i], (i + 1) * 10);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("throwing-move path: no throw configured, ctor==dtor");
  {
    Bomb::reset(-1);      // never throw
    alignas(Bomb) byte src_buf[sizeof(Bomb) * 4];
    alignas(Bomb) byte dest_buf[sizeof(Bomb) * 4];
    Bomb *src = reinterpret_cast<Bomb *>(src_buf);
    Bomb *dest = reinterpret_cast<Bomb *>(dest_buf);
    for ( int i = 0; i < 4; ++i ) new (src + i) Bomb(i + 1);

    micron::__impl_container::deep_move<Bomb>(dest, src, 4);
    // dest[0..3] live, src[0..3] destroyed
    for ( int i = 0; i < 4; ++i ) require(dest[i].v, i + 1);
    for ( int i = 0; i < 4; ++i ) dest[i].~Bomb();
    require(Bomb::ctor, Bomb::dtor);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("throwing-move at index 2 of 5: rollback + rethrow");
  {
    Bomb::reset(2);      // throw on the 3rd move (0-indexed: i=2)
    alignas(Bomb) byte src_buf[sizeof(Bomb) * 5];
    alignas(Bomb) byte dest_buf[sizeof(Bomb) * 5];
    Bomb *src = reinterpret_cast<Bomb *>(src_buf);
    Bomb *dest = reinterpret_cast<Bomb *>(dest_buf);
    for ( int i = 0; i < 5; ++i ) new (src + i) Bomb(i + 1);

    bool caught = false;
    try {
      micron::__impl_container::deep_move<Bomb>(dest, src, 5);
    } catch ( const micron::runtime & ) {
      caught = true;
    }
    require(caught, true);
    // src[2..4] should still be live (never moved-from); destroy them now.
    require(src[2].v, 3);
    require(src[3].v, 4);
    require(src[4].v, 5);
    for ( int i = 2; i < 5; ++i ) src[i].~Bomb();
    // ctor count: 5 initial + 2 successful move-ctors (i=0, i=1) = 7
    // dtor count: 2 src destroys during move + 2 dest rollback + 3 src cleanup = 7
    require(Bomb::ctor, Bomb::dtor);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("throwing-move at index 0 of 3: no dest constructed, full src live");
  {
    Bomb::reset(0);      // throw immediately on first move
    alignas(Bomb) byte src_buf[sizeof(Bomb) * 3];
    alignas(Bomb) byte dest_buf[sizeof(Bomb) * 3];
    Bomb *src = reinterpret_cast<Bomb *>(src_buf);
    Bomb *dest = reinterpret_cast<Bomb *>(dest_buf);
    (void)dest;
    for ( int i = 0; i < 3; ++i ) new (src + i) Bomb(i + 1);

    bool caught = false;
    try {
      micron::__impl_container::deep_move<Bomb>(dest, src, 3);
    } catch ( const micron::runtime & ) {
      caught = true;
    }
    require(caught, true);
    // src[0..2] all still live (the throw happens DURING the first move ctor,
    // before src[0] is destroyed)
    require(src[0].v, 1);
    require(src[1].v, 2);
    require(src[2].v, 3);
    for ( int i = 0; i < 3; ++i ) src[i].~Bomb();
    require(Bomb::ctor, Bomb::dtor);
  }
  end_test_case();

  print("=== ALL DEEP_MOVE SAFETY TESTS PASSED ===");
  return 1;
}
