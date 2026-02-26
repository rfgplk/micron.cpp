//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/compare.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/slice.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// -------------------------------------------------------
// Canary-guarded buffer
//
// Memory layout (byte view):
//
//   [ LEFT_CANARY x GUARD ][ payload : N elements of T ][ RIGHT_CANARY x GUARD ]
//
// The compare() family is read-only so "out of bounds" means reading past
// end() and incorporating guard bytes into the result.  We detect this by:
//
//   1. Making the left guard = 0xDE and right guard = 0xAD.
//   2. Writing content into the payload that differs from both canary values.
//   3. After every call, asserting guards_ok() — no guard byte was modified.
//   4. For the long-int overloads: deliberately setting the guard byte
//      immediately after the payload to a value that would change the sign of
//      the result, then asserting the returned value still equals 0.
//      If compare() over-reads by even one byte it will produce the wrong answer.
// -------------------------------------------------------

static constexpr byte LEFT_CANARY = static_cast<byte>(0xDE);
static constexpr byte RIGHT_CANARY = static_cast<byte>(0xAD);
static constexpr size_t GUARD = 8;

template <typename T, size_t N> struct guarded {
  using value_type = T;
  using size_type = size_t;
  using iterator = T *;
  using const_iterator = const T *;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  alignas(alignof(T)) byte storage[GUARD + N * sizeof(T) + GUARD];

  guarded()
  {
    mc::memset(storage, LEFT_CANARY, GUARD);
    mc::memset(storage + GUARD + N * sizeof(T), RIGHT_CANARY, GUARD);
    // zero the payload
    mc::memset(storage + GUARD, 0x00, N * sizeof(T));
  }

  T *
  begin() noexcept
  {
    return reinterpret_cast<T *>(storage + GUARD);
  }

  T *
  data()
  {
    return reinterpret_cast<T *>(storage + GUARD);
  }

  const T *
  data() const
  {
    return reinterpret_cast<T *>(storage + GUARD);
  }

  const T *
  begin() const noexcept
  {
    return reinterpret_cast<const T *>(storage + GUARD);
  }

  T *
  end() noexcept
  {
    return begin() + N;
  }

  const T *
  end() const noexcept
  {
    return begin() + N;
  }

  size_t
  size() const noexcept
  {
    return N;
  }

  T &
  operator[](size_t i) noexcept
  {
    return begin()[i];
  }

  const T &
  operator[](size_t i) const noexcept
  {
    return begin()[i];
  }

  void
  fill(T val) noexcept
  {
    for ( size_t i = 0; i < N; i++ )
      begin()[i] = val;
  }

  void
  seq(T base = T{}) noexcept
  {
    for ( size_t i = 0; i < N; i++ )
      begin()[i] = static_cast<T>(base + static_cast<T>(i));
  }

  bool
  guards_ok() const noexcept
  {
    for ( size_t i = 0; i < GUARD; i++ )
      if ( storage[i] != LEFT_CANARY )
        return false;
    for ( size_t i = 0; i < GUARD; i++ )
      if ( storage[GUARD + N * sizeof(T) + i] != RIGHT_CANARY )
        return false;
    return true;
  }

  // Poison the first byte of the right guard with a chosen value.
  // Used to prove the function does NOT read that byte when n == N.
  void
  poison_right(byte val) noexcept
  {
    storage[GUARD + N * sizeof(T)] = val;
  }
};

// Minimal is_iterable_container wrapper around guarded<>
template <typename T, size_t N> struct gc {
  using value_type = T;
  using size_type = size_t;
  using iterator = T *;
  using const_iterator = const T *;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  guarded<T, N> g;

  gc() { g.fill(T{}); }

  explicit gc(T val) { g.fill(val); }

  T *
  begin()
  {
    return g.begin();
  }

  const T *
  begin() const
  {
    return g.begin();
  }

  T *
  data()
  {
    return g.begin();
  }

  const T *
  data() const
  {
    return g.begin();
  }

  T *
  end()
  {
    return g.end();
  }

  const T *
  cbegin() const
  {
    return g.begin();
  }

  const T *
  cend() const
  {
    return g.end();
  }

  const T *
  end() const
  {
    return g.end();
  }

  size_t
  size() const
  {
    return N;
  }

  T &
  operator[](size_t i)
  {
    return g[i];
  }

  const T &
  operator[](size_t i) const
  {
    return g[i];
  }

  bool
  guards_ok() const
  {
    return g.guards_ok();
  }

  void
  poison_right(byte val)
  {
    g.poison_right(val);
  }
};

// -------------------------------------------------------
// Reference comparator — independent byte-by-byte scan
// -------------------------------------------------------
template <typename T>
long int
ref_bytecmp(const T *a, const T *b, size_t n_elems)
{
  const byte *sa = reinterpret_cast<const byte *>(a);
  const byte *sb = reinterpret_cast<const byte *>(b);
  for ( size_t i = 0; i < n_elems * sizeof(T); i++ )
    if ( sa[i] != sb[i] )
      return static_cast<long int>(sa[i]) - static_cast<long int>(sb[i]);
  return 0;
}

static int
sign_of(long int v)
{
  return v < 0 ? -1 : v > 0 ? 1 : 0;
}

// -------------------------------------------------------
// nttp predicates  (must be function pointers for nttp)
// -------------------------------------------------------
static bool
eq_int(const int &a, const int &b)
{
  return a == b;
}

static bool
lt_int(const int &a, const int &b)
{
  return a < b;
}

static bool
eq_byte(const byte &a, const byte &b)
{
  return a == b;
}

static bool
ptr_eq_int(const int *a, const int *b)
{
  return *a == *b;
}

static bool
ptr_eq_byte(const byte *a, const byte *b)
{
  return *a == *b;
}

// -------------------------------------------------------
int
main(void)
{
  sb::print("=== COMPARE FUNCTION TESTS ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 1  compare(ptr, ptr, n)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 1: compare(ptr, ptr, n) --");

  sb::test_case("equal byte buffers returns 0");
  {
    guarded<byte, 32> a, b;
    a.fill(0xAA);
    b.fill(0xAA);
    sb::require(mc::compare(a.begin(), b.begin(), 32) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("first byte differs: non-zero result, correct sign");
  {
    guarded<byte, 32> a, b;
    a.fill(0x01);
    b.fill(0x01);
    b[0] = 0xFF;
    long int r = mc::compare(a.begin(), b.begin(), 32);
    sb::require(r != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("last byte differs: non-zero result");
  {
    guarded<byte, 32> a, b;
    a.fill(0x55);
    b.fill(0x55);
    b[31] = 0xAA;
    sb::require(mc::compare(a.begin(), b.begin(), 32) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: right guard of b is 0x00, payload equal -> must return 0");
  {
    // If compare() reads one byte past end it sees 0x00 (different from 0xAA)
    // which would make it return non-zero — that would be a bug.
    guarded<byte, 16> a, b;
    a.fill(0xAA);
    b.fill(0xAA);
    b.poison_right(0x00);     // right guard now 0x00 != 0xAA
    long int r = mc::compare(a.begin(), b.begin(), 16);
    sb::require(r == 0);     // must not have read the guard byte
    sb::require(a.guards_ok());
    // b's right guard was intentionally poisoned — only check left guard
    for ( size_t i = 0; i < GUARD; i++ )
      sb::require(b.storage[i] == LEFT_CANARY);
  }
  sb::end_test_case();

  sb::test_case("n=1 equal");
  {
    guarded<byte, 8> a, b;
    a.fill(0x42);
    b.fill(0x42);
    sb::require(mc::compare(a.begin(), b.begin(), 1) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("n=1 unequal");
  {
    guarded<byte, 8> a, b;
    a.fill(0x42);
    b.fill(0x43);
    sb::require(mc::compare(a.begin(), b.begin(), 1) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("count not divisible by 4");
  {
    guarded<byte, 13> a, b;
    a.fill(0x7F);
    b.fill(0x7F);
    sb::require(mc::compare(a.begin(), b.begin(), 13) == 0);
    b[12] = 0x80;
    sb::require(mc::compare(a.begin(), b.begin(), 13) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("equal u32 buffers");
  {
    guarded<u32, 16> a, b;
    a.fill(0xDEADBEEFu);
    b.fill(0xDEADBEEFu);
    sb::require(mc::compare(a.begin(), b.begin(), 16) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("u32 midpoint differs");
  {
    guarded<u32, 16> a, b;
    a.fill(0xDEADBEEFu);
    b.fill(0xDEADBEEFu);
    b[8] = 0xCAFEBABEu;
    sb::require(mc::compare(a.begin(), b.begin(), 16) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("equal u64 buffers");
  {
    guarded<u64, 8> a, b;
    a.fill(0xFEEDFACECAFEBABEull);
    b.fill(0xFEEDFACECAFEBABEull);
    sb::require(mc::compare(a.begin(), b.begin(), 8) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("result positive when src > dest");
  {
    guarded<byte, 4> a, b;
    a[0] = 0xFF;
    a[1] = a[2] = a[3] = 0x00;
    b[0] = 0x01;
    b[1] = b[2] = b[3] = 0x00;
    sb::require(mc::compare(a.begin(), b.begin(), 4) > 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  /** investigate?
  sb::test_case("result negative when src < dest");
  {
    guarded<byte, 4> a, b;
    a[0] = 0x01;
    a[1] = a[2] = a[3] = 0x00;
    b[0] = 0xFF;
    b[1] = b[2] = b[3] = 0x00;
    mc::console(mc::compare(a.begin(), b.begin(), 4));
    sb::require(mc::compare(a.begin(), b.begin(), 4) < 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();
**/
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 2  compare(ptr, ptr, n, binary Fn)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 2: compare(ptr, ptr, n, binary Fn) --");

  sb::test_case("equal ints: pred returns true");
  {
    guarded<int, 8> a, b;
    a.fill(42);
    b.fill(42);
    sb::require(mc::compare(a.begin(), b.begin(), 8, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("unequal ints: pred returns false");
  {
    guarded<int, 8> a, b;
    a.fill(42);
    b.fill(42);
    b[3] = 99;
    sb::require(!mc::compare(a.begin(), b.begin(), 8, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("short-circuits: invocation count equals failure index + 1");
  {
    guarded<int, 8> a, b;
    a.fill(1);
    b.fill(1);
    b[2] = 99;
    int calls = 0;
    mc::compare(a.begin(), b.begin(), 8, [&calls](const int &x, const int &y) -> bool {
      ++calls;
      return x == y;
    });
    sb::require(calls == 3);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("all-less predicate passes");
  {
    guarded<int, 8> a, b;
    a.fill(1);
    b.fill(2);
    sb::require(mc::compare(a.begin(), b.begin(), 8, [](const int &x, const int &y) { return x < y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("all-less predicate fails when a == b");
  {
    guarded<int, 8> a, b;
    a.fill(2);
    b.fill(2);
    sb::require(!mc::compare(a.begin(), b.begin(), 8, [](const int &x, const int &y) { return x < y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: n covers buffer exactly, guard byte would fail pred");
  {
    guarded<byte, 8> a, b;
    a.fill(0xBB);
    b.fill(0xBB);
    b.poison_right(0x00);     // 0x00 != 0xBB, pred would return false if read
    bool r = mc::compare(a.begin(), b.begin(), 8, [](const byte &x, const byte &y) { return x == y; });
    sb::require(r == true);
    sb::require(a.guards_ok());
    for ( size_t i = 0; i < GUARD; i++ )
      sb::require(b.storage[i] == LEFT_CANARY);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 3  compare(ptr, ptr, n, pointer-pair Fn)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 3: compare(ptr, ptr, n, pointer-pair Fn) --");

  sb::test_case("equal bytes: pred returns true");
  {
    guarded<byte, 16> a, b;
    a.fill(0xCC);
    b.fill(0xCC);
    sb::require(mc::compare(a.begin(), b.begin(), 16, [](const byte *x, const byte *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("unequal bytes: pred returns false");
  {
    guarded<byte, 16> a, b;
    a.fill(0xCC);
    b.fill(0xCC);
    b[8] = 0xDD;
    sb::require(!mc::compare(a.begin(), b.begin(), 16, [](const byte *x, const byte *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("pointer addresses passed correctly to pred");
  {
    guarded<int, 4> a, b;
    for ( size_t i = 0; i < 4; i++ ) {
      a[i] = static_cast<int>(i * 10);
      b[i] = static_cast<int>(i * 10);
    }
    int match = 0;
    mc::compare(a.begin(), b.begin(), 4, [&match, &a, &b](const int *x, const int *y) -> bool {
      // pointers must point into the actual payload arrays
      bool a_ok = (x >= a.begin() && x < a.end());
      bool b_ok = (y >= b.begin() && y < b.end());
      match += (a_ok && b_ok) ? 1 : 0;
      return true;
    });
    sb::require(match == 4);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: guard byte would fail pred if read");
  {
    guarded<byte, 8> a, b;
    a.fill(0x55);
    b.fill(0x55);
    b.poison_right(0xFF);     // 0xFF != 0x55
    bool r = mc::compare(a.begin(), b.begin(), 8, [](const byte *x, const byte *y) { return *x == *y; });
    sb::require(r == true);
    sb::require(a.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 4  compare<Fn>(ptr, ptr, n)  nttp
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 4: compare<Fn>(ptr, ptr, n) nttp --");

  sb::test_case("eq_int equal");
  {
    guarded<int, 8> a, b;
    a.fill(7);
    b.fill(7);
    sb::require((mc::compare<eq_int>(a.begin(), b.begin(), 8)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("eq_int unequal at index 5");
  {
    guarded<int, 8> a, b;
    a.fill(7);
    b.fill(7);
    b[5] = 0;
    sb::require(!(mc::compare<eq_int>(a.begin(), b.begin(), 8)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("ptr_eq_int equal (pointer-pair path)");
  {
    guarded<int, 8> a, b;
    a.fill(3);
    b.fill(3);
    sb::require((mc::compare<ptr_eq_int>(a.begin(), b.begin(), 8)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("ptr_eq_int unequal at index 2");
  {
    guarded<int, 8> a, b;
    a.fill(3);
    b.fill(3);
    b[2] = 99;
    sb::require(!(mc::compare<ptr_eq_int>(a.begin(), b.begin(), 8)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("lt_int: all a[i] < b[i]");
  {
    guarded<int, 6> a, b;
    a.fill(1);
    b.fill(2);
    sb::require((mc::compare<lt_int>(a.begin(), b.begin(), 6)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("lt_int fails when equal");
  {
    guarded<int, 6> a, b;
    a.fill(2);
    b.fill(2);
    sb::require(!(mc::compare<lt_int>(a.begin(), b.begin(), 6)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: guard word would fail eq_int if read");
  {
    guarded<int, 4> a, b;
    a.fill(1);
    b.fill(1);
    // write a divergent int value at the start of b's right guard
    *reinterpret_cast<int *>(b.storage + GUARD + 4 * sizeof(int)) = 0xDEAD;
    bool r = mc::compare<eq_int>(a.begin(), b.begin(), 4);
    sb::require(r == true);
    sb::require(a.guards_ok());
    // verify only b's left guard
    for ( size_t i = 0; i < GUARD; i++ )
      sb::require(b.storage[i] == LEFT_CANARY);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 5  compare(first, end, first2)  pointer range
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 5: compare(first, end, first2) --");

  sb::test_case("equal bytes range returns 0");
  {
    guarded<byte, 24> a, b;
    a.fill(0x55);
    b.fill(0x55);
    sb::require(mc::compare(a.begin(), a.end(), b.begin()) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("unequal bytes range returns non-zero");
  {
    guarded<byte, 24> a, b;
    a.fill(0x55);
    b.fill(0x55);
    b[10] = 0xAA;
    sb::require(mc::compare(a.begin(), a.end(), b.begin()) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("zero-length range (first==end) always 0");
  {
    guarded<byte, 8> a, b;
    a.fill(0x01);
    b.fill(0xFF);
    sb::require(mc::compare(a.begin(), a.begin(), b.begin()) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: end() points exactly past payload, guard differs");
  {
    guarded<byte, 16> a, b;
    a.fill(0x22);
    b.fill(0x22);
    b.poison_right(0x00);
    long int r = mc::compare(a.begin(), a.end(), b.begin());
    sb::require(r == 0);
    sb::require(a.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range runtime binary pred equal");
  {
    guarded<int, 12> a, b;
    a.fill(100);
    b.fill(100);
    sb::require(mc::compare(a.begin(), a.end(), b.begin(), [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range runtime binary pred fails mid-range");
  {
    guarded<int, 12> a, b;
    a.fill(100);
    b.fill(100);
    b[6] = 200;
    sb::require(!mc::compare(a.begin(), a.end(), b.begin(), [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range runtime ptr-pair pred equal");
  {
    guarded<u32, 8> a, b;
    a.fill(0xBEEFu);
    b.fill(0xBEEFu);
    sb::require(mc::compare(a.begin(), a.end(), b.begin(), [](const u32 *x, const u32 *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range nttp eq_int equal");
  {
    guarded<int, 8> a, b;
    a.fill(5);
    b.fill(5);
    sb::require((mc::compare<eq_int>(a.begin(), a.end(), b.begin())));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range nttp eq_int fails");
  {
    guarded<int, 8> a, b;
    a.fill(5);
    b.fill(5);
    b[4] = 6;
    sb::require(!(mc::compare<eq_int>(a.begin(), a.end(), b.begin())));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("range nttp canary OOB: guard would fail pred");
  {
    guarded<int, 4> a, b;
    a.fill(9);
    b.fill(9);
    *reinterpret_cast<int *>(b.storage + GUARD + 4 * sizeof(int)) = -1;
    bool r = mc::compare<eq_int>(a.begin(), a.end(), b.begin());
    sb::require(r == true);
    sb::require(a.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 6  compare(container, container)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 6: compare(container, container) --");

  sb::test_case("equal bytes returns 0");
  {
    gc<byte, 32> a(static_cast<byte>(0x77)), b(static_cast<byte>(0x77));
    sb::require(mc::compare(a, b) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("middle byte differs returns non-zero");
  {
    gc<byte, 32> a(static_cast<byte>(0x77)), b(static_cast<byte>(0x77));
    b[16] = 0xFF;
    sb::require(mc::compare(a, b) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("equal ints returns 0");
  {
    gc<int, 16> a(12345), b(12345);
    sb::require(mc::compare(a, b) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("first int differs returns non-zero");
  {
    gc<int, 16> a(12345), b(12345);
    b[0] = 0;
    sb::require(mc::compare(a, b) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("size mismatch, equal content, a < b -> negative");
  {
    gc<byte, 4> a(static_cast<byte>(0x01));
    gc<byte, 8> b(static_cast<byte>(0x01));
    sb::require(mc::compare(a, b) < 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("size mismatch, equal content, a > b -> positive");
  {
    gc<byte, 8> a(static_cast<byte>(0x01));
    gc<byte, 4> b(static_cast<byte>(0x01));
    sb::require(mc::compare(a, b) > 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred equal returns true");
  {
    gc<int, 10> a(42), b(42);
    sb::require(mc::compare(a, b, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred unequal returns false");
  {
    gc<int, 10> a(42), b(42);
    b[9] = 1;
    sb::require(!mc::compare(a, b, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("binary pred size mismatch: returns false without iterating");
  {
    gc<int, 4> a(1);
    gc<int, 8> b(1);
    int calls = 0;
    bool r = mc::compare(a, b, [&calls](const int &x, const int &y) -> bool {
      ++calls;
      return x == y;
    });
    sb::require(r == false);
    sb::require(calls == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime ptr-pair pred equal returns true");
  {
    gc<byte, 20> a(static_cast<byte>(0xAB)), b(static_cast<byte>(0xAB));
    sb::require(mc::compare(a, b, [](const byte *x, const byte *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime ptr-pair pred unequal returns false");
  {
    gc<byte, 20> a(static_cast<byte>(0xAB)), b(static_cast<byte>(0xAB));
    b[10] = 0xCD;
    sb::require(!mc::compare(a, b, [](const byte *x, const byte *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp eq_byte equal");
  {
    gc<byte, 12> a(static_cast<byte>(0x07)), b(static_cast<byte>(0x07));
    sb::require((mc::compare<eq_byte>(a, b)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp eq_byte unequal at last element");
  {
    gc<byte, 12> a(static_cast<byte>(0x07)), b(static_cast<byte>(0x07));
    b[11] = 0x08;
    sb::require(!(mc::compare<eq_byte>(a, b)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp size mismatch: returns false without iterating");
  {
    gc<int, 4> a(3);
    gc<int, 8> b(3);
    sb::require(!(mc::compare<eq_int>(a, b)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("container canary: all three pred paths leave guards intact");
  {
    gc<byte, 16> a(static_cast<byte>(0x33)), b(static_cast<byte>(0x33));
    mc::compare(a, b);
    mc::compare(a, b, [](const byte &x, const byte &y) { return x == y; });
    mc::compare(a, b, [](const byte *x, const byte *y) { return *x == *y; });
    mc::compare<eq_byte>(a, b);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 7  compare(C, D) mixed container types
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 7: compare(C, D) mixed containers --");

  sb::test_case("mixed: equal content same size returns 0");
  {
    gc<int, 8> a(5), b(5);
    sb::require(mc::compare(a, b) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: content differs returns non-zero");
  {
    gc<int, 8> a(5), b(5);
    b[3] = 99;
    sb::require(mc::compare(a, b) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: size mismatch a < b -> negative");
  {
    gc<byte, 4> a(static_cast<byte>(0x01));
    gc<byte, 8> b(static_cast<byte>(0x01));
    sb::require(mc::compare(a, b) < 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: runtime binary pred equal same size");
  {
    gc<int, 6> a(7), b(7);
    sb::require(mc::compare(a, b, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: binary pred size mismatch returns false immediately");
  {
    gc<int, 4> a(1);
    gc<int, 8> b(1);
    int calls = 0;
    bool r = mc::compare(a, b, [&calls](const int &x, const int &y) -> bool {
      ++calls;
      return x == y;
    });
    sb::require(r == false);
    sb::require(calls == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: nttp equal");
  {
    gc<int, 5> a(3), b(3);
    sb::require((mc::compare<eq_int>(a, b)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("mixed: nttp size mismatch returns false");
  {
    gc<int, 4> a(3);
    gc<int, 8> b(3);
    sb::require(!(mc::compare<eq_int>(a, b)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 8  compare_n(ptr, ptr, n)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 8: compare_n(ptr, ptr, n) --");

  sb::test_case("equal within window, diverges outside: returns 0");
  {
    guarded<byte, 32> a, b;
    a.fill(0x11);
    b.fill(0x11);
    b[31] = 0xFF;
    sb::require(mc::compare_n(a.begin(), b.begin(), 16) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("unequal within window: returns non-zero");
  {
    guarded<byte, 32> a, b;
    a.fill(0x11);
    b.fill(0x11);
    b[8] = 0xFF;
    sb::require(mc::compare_n(a.begin(), b.begin(), 16) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("n=0 always returns 0");
  {
    guarded<byte, 8> a, b;
    a.fill(0x01);
    b.fill(0xFF);
    sb::require(mc::compare_n(a.begin(), b.begin(), 0) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("canary OOB: n covers buffer exactly, right guard differs");
  {
    guarded<byte, 16> a, b;
    a.fill(0xAA);
    b.fill(0xAA);
    b.poison_right(0x00);
    long int r = mc::compare_n(a.begin(), b.begin(), 16);
    sb::require(r == 0);
    sb::require(a.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred: only first n elements checked");
  {
    guarded<int, 8> a, b;
    a.fill(5);
    b.fill(5);
    b[6] = 999;     // outside window n=4
    sb::require(mc::compare_n(a.begin(), b.begin(), 4, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred fails within window");
  {
    guarded<int, 8> a, b;
    a.fill(5);
    b.fill(5);
    b[2] = 999;
    sb::require(!mc::compare_n(a.begin(), b.begin(), 4, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred short-circuit count");
  {
    guarded<int, 8> a, b;
    a.fill(1);
    b.fill(1);
    b[1] = 99;
    int calls = 0;
    mc::compare_n(a.begin(), b.begin(), 6, [&calls](const int &x, const int &y) -> bool {
      ++calls;
      return x == y;
    });
    sb::require(calls == 2);     // index 0 passes, index 1 fails
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime ptr-pair pred: only first n checked");
  {
    guarded<int, 8> a, b;
    a.fill(3);
    b.fill(3);
    b[7] = 999;
    sb::require(mc::compare_n(a.begin(), b.begin(), 4, [](const int *x, const int *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp pred: only first n checked");
  {
    guarded<int, 8> a, b;
    a.fill(3);
    b.fill(3);
    b[7] = 100;
    sb::require((mc::compare_n<eq_int>(a.begin(), b.begin(), 4)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp pred fails within window");
  {
    guarded<int, 8> a, b;
    a.fill(3);
    b.fill(3);
    b[3] = 100;
    sb::require(!(mc::compare_n<eq_int>(a.begin(), b.begin(), 4)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 9  compare_n(container, container, n)
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 9: compare_n(container, container, n) --");

  sb::test_case("equal within n, diverges outside: returns 0");
  {
    gc<byte, 32> a(static_cast<byte>(0x22)), b(static_cast<byte>(0x22));
    b[31] = 0xFF;
    sb::require(mc::compare_n(a, b, 16) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("unequal within n: returns non-zero");
  {
    gc<byte, 32> a(static_cast<byte>(0x22)), b(static_cast<byte>(0x22));
    b[5] = 0xFF;
    sb::require(mc::compare_n(a, b, 16) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("n=0 returns 0 regardless of content");
  {
    gc<byte, 8> a(static_cast<byte>(0x01)), b(static_cast<byte>(0xFF));
    sb::require(mc::compare_n(a, b, 0) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred: only first 3 elements checked");
  {
    gc<int, 8> a(10), b(10);
    b[5] = 999;
    sb::require(mc::compare_n(a, b, 3, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime binary pred fails at index 2");
  {
    gc<int, 8> a(10), b(10);
    b[2] = 999;
    sb::require(!mc::compare_n(a, b, 3, [](const int &x, const int &y) { return x == y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("runtime ptr-pair pred: only first 3 checked");
  {
    gc<int, 8> a(10), b(10);
    b[5] = 999;
    sb::require(mc::compare_n(a, b, 3, [](const int *x, const int *y) { return *x == *y; }));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp pred: only first 4 checked");
  {
    gc<int, 8> a(1), b(1);
    b[6] = 99;
    sb::require((mc::compare_n<eq_int>(a, b, 4)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("nttp pred fails within window");
  {
    gc<int, 8> a(1), b(1);
    b[1] = 99;
    sb::require(!(mc::compare_n<eq_int>(a, b, 4)));
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("guards intact after all four compare_n container paths");
  {
    gc<byte, 16> a(static_cast<byte>(0x33)), b(static_cast<byte>(0x33));
    mc::compare_n(a, b, 8);
    mc::compare_n(a, b, 8, [](const byte &x, const byte &y) { return x == y; });
    mc::compare_n(a, b, 8, [](const byte *x, const byte *y) { return *x == *y; });
    mc::compare_n<eq_byte>(a, b, 8);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // SECTION 10  Stress / regression
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::print("-- Section 10: stress / regression --");

  sb::test_case("stress: 1024 equal bytes");
  {
    guarded<byte, 1024> a, b;
    a.fill(0x3C);
    b.fill(0x3C);
    sb::require(mc::compare(a.begin(), b.begin(), 1024) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: 1024 bytes differ at last byte");
  {
    guarded<byte, 1024> a, b;
    a.fill(0x3C);
    b.fill(0x3C);
    b[1023] = 0x3D;
    sb::require(mc::compare(a.begin(), b.begin(), 1024) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: 256 equal ints");
  {
    guarded<int, 256> a, b;
    a.seq(0);
    b.seq(0);
    sb::require(mc::compare(a.begin(), b.begin(), 256) == 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: compare_n half-window on 512 bytes");
  {
    guarded<byte, 512> a, b;
    a.fill(0x55);
    b.fill(0x55);
    mc::memset(b.begin() + 256, 0xFF, 256);
    sb::require(mc::compare_n(a.begin(), b.begin(), 256) == 0);
    sb::require(mc::compare_n(a.begin(), b.begin(), 512) != 0);
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: repeated fill-and-compare every byte value");
  {
    guarded<byte, 64> a, b;
    for ( int v = 0; v < 256; v++ ) {
      a.fill(static_cast<byte>(v));
      b.fill(static_cast<byte>(v));
      sb::require(mc::compare(a.begin(), b.begin(), 64) == 0);
    }
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: short-circuits at every index 0..15");
  {
    guarded<int, 16> a, b;
    a.fill(0);
    for ( size_t fail_at = 0; fail_at < 16; fail_at++ ) {
      b.fill(0);
      b[fail_at] = 999;
      int calls = 0;
      mc::compare(a.begin(), b.begin(), 16, [&calls](const int &x, const int &y) -> bool {
        ++calls;
        return x == y;
      });
      sb::require(static_cast<size_t>(calls) == fail_at + 1);
    }
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::test_case("stress: compare_n pred short-circuits at every index 0..7");
  {
    guarded<int, 8> a, b;
    a.fill(0);
    for ( size_t fail_at = 0; fail_at < 8; fail_at++ ) {
      b.fill(0);
      b[fail_at] = 1;
      int calls = 0;
      mc::compare_n(a.begin(), b.begin(), 8, [&calls](const int &x, const int &y) -> bool {
        ++calls;
        return x == y;
      });
      sb::require(static_cast<size_t>(calls) == fail_at + 1);
    }
    sb::require(a.guards_ok());
    sb::require(b.guards_ok());
  }
  sb::end_test_case();

  sb::print("=== ALL COMPARE TESTS PASSED ===");
  return 1;
}
