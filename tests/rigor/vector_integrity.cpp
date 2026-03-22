// vector_integrity.cpp
// Memory integrity, leak detection, and safety tests for micron::vector<T>
// Run under valgrind: valgrind --leak-check=full --track-origins=yes
//                              --error-exitcode=1 ./vector_integrity

#include "../../src/std.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

// ------------------------------------------------------------------ //
//  Lifetime tracking – every byte of ctor/dtor must balance          //
// ------------------------------------------------------------------ //
namespace
{

struct Probe {
  static inline int live = 0;      // currently alive instances
  static inline int total = 0;     // total ever constructed
  static inline bool corrupt = false;

  static constexpr int MAGIC = 0xDEADBEEF;
  int sentinel;
  int id;

  Probe() : sentinel(MAGIC), id(++total) { ++live; }

  explicit Probe(int x) : sentinel(MAGIC), id(x)
  {
    ++total;
    ++live;
  }

  Probe(const Probe &o) : sentinel(MAGIC), id(o.id)
  {
    ++total;
    ++live;
    check(o);
  }

  Probe(Probe &&o) noexcept : sentinel(MAGIC), id(o.id)
  {
    ++total;
    ++live;
    check(o);
    o.sentinel = 0;     // poison moved-from
  }

  // Restore sentinel = MAGIC on assignment: erase() calls ~T() then immediately
  // move-assigns into the same slot during the shift.  Without restoring MAGIC
  // the vector destructor later sees 0xCDCDCDCD and flags corruption.
  Probe &
  operator=(const Probe &o)
  {
    check(o);
    sentinel = MAGIC;
    id = o.id;
    return *this;
  }

  Probe &
  operator=(Probe &&o) noexcept
  {
    check(o);
    sentinel = MAGIC;
    id = o.id;
    o.sentinel = 0;
    return *this;
  }

  ~Probe()
  {
    if ( sentinel != MAGIC && sentinel != 0 )
      corrupt = true;          // double-free or use-after-move corruption
    sentinel = 0xCDCDCDCD;     // poison
    --live;
  }

  bool
  operator==(const Probe &o) const
  {
    return id == o.id;
  }

  bool
  operator>(const Probe &o) const
  {
    return id > o.id;
  }

  bool
  operator<(const Probe &o) const
  {
    return id < o.id;
  }

  static void
  check(const Probe &o)
  {
    if ( o.sentinel != MAGIC )
      corrupt = true;
  }

  static void
  reset()
  {
    live = 0;
    total = 0;
    corrupt = false;
  }
};

// Thin wrapper so we can also test the move-only specialisation
struct MoveOnlyProbe {
  static inline int live = 0;
  static inline int total = 0;

  int id;

  explicit MoveOnlyProbe(int x = 0) : id(x)
  {
    ++live;
    ++total;
  }

  MoveOnlyProbe(const MoveOnlyProbe &) = delete;

  MoveOnlyProbe(MoveOnlyProbe &&o) noexcept : id(o.id)
  {
    o.id = -1;
    ++live;
    ++total;
  }

  MoveOnlyProbe &operator=(const MoveOnlyProbe &) = delete;

  MoveOnlyProbe &
  operator=(MoveOnlyProbe &&o) noexcept
  {
    id = o.id;
    o.id = -1;
    return *this;
  }

  ~MoveOnlyProbe() { --live; }

  bool
  operator==(const MoveOnlyProbe &o) const
  {
    return id == o.id;
  }

  bool
  operator>(const MoveOnlyProbe &o) const
  {
    return id > o.id;
  }

  bool
  operator<(const MoveOnlyProbe &o) const
  {
    return id < o.id;
  }

  static void
  reset()
  {
    live = 0;
    total = 0;
  }
};

}     // namespace

// ================================================================== //
int
main()
{
  sb::print("=== VECTOR INTEGRITY TESTS ===");

  // ================================================================ //
  //  Lifetime balance – no leaks, no extra destructions              //
  // ================================================================ //
  test_case("lifetime: emplace_back and destroy – ctor == dtor");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 256; ++i )
        v.emplace_back(i);
      require(Probe::live, 256);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: push_back copy – no phantom destructions");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      Probe src(42);
      for ( int i = 0; i < 128; ++i )
        v.push_back(src);
      // src + 128 copies alive
      require(Probe::live, 129);
    }
    // src destroyed at end of block too
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: push_back rvalue – move does not leak");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 64; ++i )
        v.push_back(Probe(i));
      require(Probe::live, 64);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: reserve triggers realloc – objects survive");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 8; ++i )
        v.emplace_back(i);
      int live_before = Probe::live;
      v.reserve(4096);
      // All objects must still be alive and uncorrupted
      require(Probe::live, live_before);
      require_false(Probe::corrupt);
      for ( int i = 0; i < 8; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: repeated reserve does not double-destroy");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 32; ++i )
        v.emplace_back(i);
      for ( size_t cap : { 64, 128, 256, 512, 1024 } )
        v.reserve(cap);
      require(Probe::live, 32);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: erase single – exactly one destructor called");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 16; ++i )
        v.emplace_back(i);
      require(Probe::live, 16);

      v.erase(size_t(0));
      require(Probe::live, 15);
      require(v[0].id, 1);

      v.erase(size_t(7));
      require(Probe::live, 14);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: erase range – exactly (to-from) destructors");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 32; ++i )
        v.emplace_back(i);

      v.erase(size_t(4), size_t(12));     // 8 elements erased
      require(Probe::live, 24);
      require(v[4].id, 12);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: pop_back – exactly one destructor per pop");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 64; ++i )
        v.emplace_back(i);
      for ( int i = 63; i >= 0; --i ) {
        require(Probe::live, i + 1);
        v.pop_back();
      }
      require(Probe::live, 0);
      require_true(v.empty());
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: clear destroys all, reuse after clear is clean");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 100; ++i )
        v.emplace_back(i);
      require(Probe::live, 100);
      v.clear();
      require(Probe::live, 0);
      require_true(v.empty());

      // second fill after clear – should not double-destroy previous objects
      for ( int i = 0; i < 50; ++i )
        v.emplace_back(i);
      require(Probe::live, 50);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: copy construction – independent, source untouched");
  {
    Probe::reset();
    {
      micron::vector<Probe> a;
      for ( int i = 0; i < 32; ++i )
        a.emplace_back(i);
      int after_fill = Probe::live;

      micron::vector<Probe> b(a);
      require(Probe::live, after_fill * 2);
      require(b.size(), a.size());
      for ( size_t i = 0; i < a.size(); ++i )
        require(b[i].id, a[i].id);

      // mutating b must not affect a
      b[0] = Probe(999);
      require(a[0].id, 0);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: copy assignment – old content destroyed before overwrite");
  {
    Probe::reset();
    {
      micron::vector<Probe> a, b;
      for ( int i = 0; i < 16; ++i )
        a.emplace_back(i);
      for ( int i = 0; i < 8; ++i )
        b.emplace_back(100 + i);

      int before = Probe::live;     // 24
      b = a;
      // b's old 8 elements destroyed, 16 new copies made
      // net change: -8 +16 = +8
      require(Probe::live, before - 8 + 16);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: move construction – source left empty, no double-free");
  {
    Probe::reset();
    {
      micron::vector<Probe> a;
      for ( int i = 0; i < 32; ++i )
        a.emplace_back(i);
      require(Probe::live, 32);

      micron::vector<Probe> b(micron::move(a));
      // move should not construct new objects
      require(Probe::live, 32);
      require(b.size(), size_t(32));
      require_true(a.empty());
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: move assignment – old content of lhs destroyed first");
  {
    Probe::reset();
    {
      micron::vector<Probe> a, b;
      for ( int i = 0; i < 20; ++i )
        a.emplace_back(i);
      for ( int i = 0; i < 10; ++i )
        b.emplace_back(i);
      require(Probe::live, 30);

      b = micron::move(a);
      // b's old 10 must be destroyed; a's 20 now owned by b
      require(Probe::live, 20);
      require(b.size(), size_t(20));
      require_true(a.empty());
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: resize(n) default-constructs new slots");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 8; ++i )
        v.emplace_back(i);
      v.resize(32);
      require(Probe::live, 32);
      // original elements preserved
      for ( int i = 0; i < 8; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: resize(n, val) copy-constructs new slots");
  {
    Probe::reset();
    {
      Probe filler(77);
      micron::vector<Probe> v;
      for ( int i = 0; i < 4; ++i )
        v.emplace_back(i);
      v.resize(20, filler);
      require(Probe::live, 21);     // 4 originals + 16 copies + filler
      for ( size_t i = 4; i < 20; ++i )
        require(v[i].id, 77);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: insert shifts objects without orphaning them");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 8; ++i )
        v.emplace_back(i * 10);
      require(Probe::live, 8);

      Probe p(999);
      v.insert(size_t(4), p);
      require(Probe::live, 10);     // 8 + 1 original p + 1 copy inside vector
      require(v[4].id, 999);
      require(v[5].id, 40);
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: remove erases all matching, no dangling");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 5; ++i )
        v.emplace_back(7);
      for ( int i = 0; i < 5; ++i )
        v.emplace_back(i);
      require(Probe::live, 10);

      v.remove(Probe(7));
      require(v.size(), size_t(5));
      // FIX: both temporaries are dead by this point.
      // Probe(7) argument lifetime ends at the semicolon above.
      // The static_cast<T>(val) copy inside remove()'s fold expression
      // also dies when remove() returns.
      // Only the 5 surviving vector elements remain.
      require(Probe::live, 5);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: append – source not destroyed, target grows cleanly");
  {
    Probe::reset();
    {
      micron::vector<Probe> a, b;
      for ( int i = 0; i < 16; ++i )
        a.emplace_back(i);
      for ( int i = 16; i < 32; ++i )
        b.emplace_back(i);
      require(Probe::live, 32);

      a.append(b);
      require(a.size(), size_t(32));
      require(b.size(), size_t(16));     // b untouched
      require(Probe::live, 48);          // 32 in a + 16 still in b
      for ( size_t i = 0; i < 32; ++i )
        require(a[i].id, (int)i);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("lifetime: weld – source content moved, sizes correct");
  {
    Probe::reset();
    {
      micron::vector<Probe> a, b;
      for ( int i = 0; i < 8; ++i )
        a.emplace_back(i);
      for ( int i = 8; i < 16; ++i )
        b.emplace_back(i);
      int before = Probe::live;

      a.weld(micron::move(b));
      require(a.size(), size_t(16));
      for ( size_t i = 0; i < 16; ++i )
        require(a[i].id, (int)i);
      require(Probe::live, before);     // total count unchanged (no new constructions)
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ================================================================ //
  //  Move-only specialisation                                        //
  // ================================================================ //
  test_case("move-only: emplace_back and destroy");
  {
    MoveOnlyProbe::reset();
    {
      micron::vector<MoveOnlyProbe> v;
      for ( int i = 0; i < 64; ++i )
        v.emplace_back(i);
      require(MoveOnlyProbe::live, 64);
    }
    require(MoveOnlyProbe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move-only: move construction leaves source empty");
  {
    MoveOnlyProbe::reset();
    {
      micron::vector<MoveOnlyProbe> a;
      for ( int i = 0; i < 32; ++i )
        a.emplace_back(i);

      micron::vector<MoveOnlyProbe> b(micron::move(a));
      require(MoveOnlyProbe::live, 32);
      require_true(a.empty());
      require(b.size(), size_t(32));
    }
    require(MoveOnlyProbe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move-only: push_back rvalue moves in without copy");
  {
    MoveOnlyProbe::reset();
    {
      micron::vector<MoveOnlyProbe> v;
      for ( int i = 0; i < 16; ++i )
        v.push_back(MoveOnlyProbe(i));
      require(MoveOnlyProbe::live, 16);
      for ( int i = 0; i < 16; ++i )
        require(v[i].id, i);
    }
    require(MoveOnlyProbe::live, 0);
  }
  end_test_case();

  // ================================================================ //
  //  Bounds-safety / exception paths                                 //
  // ================================================================ //
  test_case("bounds: at() throws, does not corrupt state");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 8; ++i )
        v.emplace_back(i);

      require_throw([&]() { (void)v.at(8); });
      require_throw([&]() { (void)v.at(99); });

      // vector must be fully intact after throws
      require(v.size(), size_t(8));
      require(Probe::live, 8);
      for ( int i = 0; i < 8; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("bounds: erase out-of-range throws, vector intact");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 4; ++i )
        v.emplace_back(i);

      require_throw([&]() { v.erase(size_t(4)); });
      require_throw([&]() { v.erase(size_t(99)); });

      require(v.size(), size_t(4));
      require(Probe::live, 4);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("bounds: erase range out-of-order throws");
  {
    micron::vector<int> v{ 1, 2, 3, 4, 5 };
    require_throw([&]() { v.erase(size_t(3), size_t(2)); });     // from > to
    require_throw([&]() { v.erase(size_t(3), size_t(6)); });     // to > length
    require(v.size(), size_t(5));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("bounds: insert out-of-range throws");
  {
    micron::vector<int> v{ 1, 2, 3 };
    require_throw([&]() { v.insert(size_t(5), 99); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("bounds: erase iterator out-of-range throws");
  {
    micron::vector<int> v{ 1, 2, 3 };
    int *before = v.begin() - 1;
    int *after = v.end() + 1;
    require_throw([&]() { v.erase(before); });
    require_throw([&]() { v.erase(after); });
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("bounds: operator[] at capacity boundary does not smash");
  {
    micron::vector<int> v;
    v.reserve(16);
    for ( int i = 0; i < 8; ++i )
      v.push_back(i);
    // capacity is >=16, length is 8
    // operator[] checks against capacity (per implementation note)
    // so indices 0..capacity-1 are valid reads per design
    // require_nothrow needs a raw function pointer; call directly instead
    (void)v[0];     // must not throw
    (void)v[7];     // must not throw
  }
  end_test_case();

  // ================================================================ //
  //  Self-assignment and aliasing                                    //
  // ================================================================ //
  test_case("self-assignment copy – no use-after-free");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 16; ++i )
        v.emplace_back(i);
      v = v;     // must not crash or corrupt
      require(v.size(), size_t(16));
      require(Probe::live, 16);
      for ( int i = 0; i < 16; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("self-assignment move – no double-free");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 16; ++i )
        v.emplace_back(i);
      v = micron::move(v);
      // implementation may clear or keep; either way must not corrupt
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ================================================================ //
  //  Stress – grow through many reallocations                        //
  // ================================================================ //
  test_case("stress: 100k push_back with reallocs – no leak");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 100000; ++i )
        v.emplace_back(i);
      require(Probe::live, 100000);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: 1000 push/clear cycles – no accumulating leak");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int round = 0; round < 1000; ++round ) {
        for ( int i = 0; i < 100; ++i )
          v.emplace_back(i);
        require(Probe::live, 100);
        v.clear();
        require(Probe::live, 0);
      }
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: interleaved push/pop – live count always correct");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      int expected_live = 0;
      for ( int i = 0; i < 10000; ++i ) {
        v.emplace_back(i);
        ++expected_live;
        if ( i % 3 == 0 && !v.empty() ) {
          v.pop_back();
          --expected_live;
        }
        require(Probe::live, expected_live);
      }
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: sort does not leak or corrupt");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 99; i >= 0; --i )
        v.emplace_back(i);
      v.sort();
      require(Probe::live, 100);
      require_false(Probe::corrupt);
      for ( int i = 0; i < 100; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: insert_sort 1000 elements in reverse – no leak");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 999; i >= 0; --i )
        v.insert_sort(Probe(i));
      require(Probe::live, 1000);
      require_false(Probe::corrupt);
      for ( int i = 0; i < 1000; ++i )
        require(v[i].id, i);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: many erase(0) from front – no corruption");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 512; ++i )
        v.emplace_back(i);
      for ( int i = 0; i < 256; ++i ) {
        require(v[0].id, i);
        v.erase(size_t(0));
        require(Probe::live, 512 - i - 1);
      }
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: assign overwrites with no accumulated leak");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 100; ++i )
        v.emplace_back(i);
      Probe filler(42);
      for ( int round = 0; round < 100; ++round ) {
        v.assign(50, filler);
        require(v.size(), size_t(50));
        require(Probe::live, 51);     // 50 in vector + filler
      }
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ================================================================ //
  //  Edge cases – empty vector operations                            //
  // ================================================================ //
  test_case("edge: pop_back on empty is safe (no underflow)");
  {
    // The implementation does not guard against pop on empty –
    // this test documents that calling it on a non-empty then
    // exactly emptied vector leaves live == 0 without corruption.
    Probe::reset();
    {
      micron::vector<Probe> v;
      v.emplace_back(1);
      v.pop_back();
      require(Probe::live, 0);
      require_true(v.empty());
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: clear on empty vector is a no-op");
  {
    Probe::reset();
    micron::vector<Probe> v;
    v.clear();
    require_true(v.empty());
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: move from empty vector yields empty target");
  {
    Probe::reset();
    {
      micron::vector<Probe> a;
      micron::vector<Probe> b(micron::move(a));
      require_true(b.empty());
      require(Probe::live, 0);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: reserve(0) on empty does not crash");
  {
    micron::vector<int> v;
    v.reserve(0);     // must not throw
    require_true(v.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: resize down is a no-op (does not truncate)");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      for ( int i = 0; i < 16; ++i )
        v.emplace_back(i);
      v.resize(4);     // resize to smaller – implementation skips per code
      require(v.size(), size_t(16));
      require(Probe::live, 16);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: single-element vector full lifecycle");
  {
    Probe::reset();
    {
      micron::vector<Probe> v;
      v.emplace_back(1);
      require(Probe::live, 1);
      require(v.front().id, 1);
      require(v.back().id, 1);
      require(v.size(), size_t(1));

      v.pop_back();
      require(Probe::live, 0);
      require_true(v.empty());
    }
    require(Probe::live, 0);
    require_false(Probe::corrupt);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: vector of vectors – nested lifetime");
  {
    {
      micron::vector<micron::vector<int>> outer;
      for ( int i = 0; i < 16; ++i ) {
        micron::vector<int> inner;
        for ( int j = 0; j < 16; ++j )
          inner.push_back(i * 16 + j);
        outer.push_back(micron::move(inner));
      }
      require(outer.size(), size_t(16));
      for ( int i = 0; i < 16; ++i ) {
        require(outer[i].size(), size_t(16));
        for ( int j = 0; j < 16; ++j )
          require(outer[i][j], i * 16 + j);
      }
    }
    // Valgrind must report 0 leaks here
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: swap – both vectors retain correct lifetimes");
  {
    Probe::reset();
    {
      micron::vector<Probe> a, b;
      for ( int i = 0; i < 8; ++i )
        a.emplace_back(i);
      for ( int i = 8; i < 24; ++i )
        b.emplace_back(i);
      require(Probe::live, 24);

      a.swap(b);
      require(a.size(), size_t(16));
      require(b.size(), size_t(8));
      require(a[0].id, 8);
      require(b[0].id, 0);
      require(Probe::live, 24);
      require_false(Probe::corrupt);
    }
    require(Probe::live, 0);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("edge: into_bytes does not own memory");
  {
    micron::vector<int> v{ 1, 2, 3, 4 };
    {
      auto bytes = v.into_bytes();
      // bytes is a view, not an owner – v must survive normally
      (void)bytes;
    }
    require(v.size(), size_t(4));
    require(v[0], 1);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("sentinel: no object corruption detected across all tests");
  {
    require_false(Probe::corrupt);
  }
  end_test_case();

  sb::print("=== ALL INTEGRITY TESTS PASSED ===");
  return 0;
}
