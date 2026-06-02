// quake_heap_exhaustive.cpp

#include "../../src/heap/quake_heap.hpp"
#include "../../src/std.hpp"

#include "../../src/io/console.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{
struct counted {
  static inline long live = 0;
  int v;

  counted() : v(0) { ++live; }

  counted(int x) : v(x) { ++live; }

  counted(const counted &o) : v(o.v) { ++live; }

  counted(counted &&o) noexcept : v(o.v) { ++live; }

  counted &
  operator=(const counted &o)
  {
    v = o.v;
    return *this;
  }

  counted &
  operator=(counted &&o) noexcept
  {
    v = o.v;
    return *this;
  }

  ~counted() { --live; }

  bool
  operator<(const counted &o) const
  {
    return v < o.v;
  }

  bool
  operator>(const counted &o) const
  {
    return v > o.v;
  }
};

struct scalar_less {
  bool
  operator()(const int &a, const int &b) const
  {
    return a < b;
  }
};

u64 __s = 0x9e3779b97f4a7c15ULL;

inline u32
rnd()
{
  __s = __s * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<u32>(__s >> 32);
}

void
isort(int *a, int n)
{
  for ( int i = 1; i < n; ++i ) {
    int k = a[i], j = i;
    while ( j > 0 && a[j - 1] > k ) {
      a[j] = a[j - 1];
      --j;
    }
    a[j] = k;
  }
}
}      // namespace

int
main()
{
  sb::print("=== QUAKE_HEAP TESTS ===");

  test_case("default construction is empty");
  {
    micron::quake_heap<int> h;
    require_true(h.empty());
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("insert then find_min returns the minimum (min-heap)");
  {
    micron::quake_heap<int> h;
    h.insert(5);
    h.insert(9);
    h.insert(2);
    h.insert(7);
    require(h.find_min(), 2);
    require(h.size(), size_t(4));
    require_false(h.empty());
  }
  end_test_case();

  test_case("insert(const T&), insert(T&&), emplace");
  {
    micron::quake_heap<int> h;
    int x = 42;
    h.insert(x);
    h.insert(13);
    h.emplace(7);
    require(h.find_min(), 7);
    require(h.size(), size_t(3));
  }
  end_test_case();

  test_case("extract_min returns elements in ascending order");
  {
    micron::quake_heap<int> h;
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) h.insert(int(x));
    int prev = -1000000;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      prev = cur;
    }
    require(h.size(), size_t(0));
  }
  end_test_case();

  test_case("duplicates");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 5; i++ ) h.insert(7);
    require(h.size(), size_t(5));
    require(h.extract_min(), 7);
    require(h.extract_min(), 7);
    require(h.size(), size_t(3));
  }
  end_test_case();

  test_case("find_min / extract_min on empty throw");
  {
    micron::quake_heap<int> h;
    require_throw([&] { (void)h.extract_min(); });
    require_throw([&] { (void)h.find_min(); });
  }
  end_test_case();

  test_case("decrease_key lowers a key and updates the minimum");
  {
    micron::quake_heap<int> h;
    h.insert(10);
    auto hb = h.insert(20);
    h.insert(30);
    require(h.find_min(), 10);
    h.decrease_key(hb, 5);
    require(h.find_min(), 5);
    require(h.extract_min(), 5);
    require(h.extract_min(), 10);
    require(h.extract_min(), 30);
  }
  end_test_case();

  test_case("decrease_key is a no-op when not an improvement");
  {
    micron::quake_heap<int> h;
    h.insert(1);
    auto hb = h.insert(8);
    h.decrease_key(hb, 50);
    require(h.find_min(), 1);
    require(h.size(), size_t(2));
    require(h.extract_min(), 1);
    require(h.extract_min(), 8);
  }
  end_test_case();

  test_case("decrease_key survives extract_min churn (handle stays valid)");
  {
    micron::quake_heap<int> h;
    auto target = h.insert(100);
    for ( int i = 0; i < 30; ++i ) h.insert(i * 3 + 1);
    (void)h.extract_min();
    (void)h.extract_min();
    (void)h.extract_min();
    h.decrease_key(target, -7);
    require(h.find_min(), -7);
    require(h.extract_min(), -7);
  }
  end_test_case();

  test_case("decrease_key vs brute-force oracle (heavy, triggers quake)");
  {
    constexpr int N = 1500;
    micron::quake_heap<int> h;
    static micron::quake_heap<int>::handle hs[N];
    static int keys[N];
    for ( int i = 0; i < N; ++i ) {
      int v = static_cast<int>(rnd() % 100000);
      keys[i] = v;
      hs[i] = h.insert(v);
    }

    for ( int t = 0; t < 4000; ++t ) {
      int j = static_cast<int>(rnd() % N);
      int dec = static_cast<int>(rnd() % 500) + 1;
      keys[j] -= dec;
      h.decrease_key(hs[j], keys[j]);
    }
    require(h.size(), size_t(N));
    static int oracle[N];
    for ( int i = 0; i < N; ++i ) oracle[i] = keys[i];
    isort(oracle, N);
    int idx = 0;
    int prev = -2000000;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      require(cur, oracle[idx]);
      prev = cur;
      ++idx;
    }
    require(idx, N);
  }
  end_test_case();

  test_case("erase removes an arbitrary element (structural delete)");
  {
    micron::quake_heap<int> h;
    h.insert(5);
    auto hb = h.insert(15);
    h.insert(25);
    auto hd = h.insert(35);
    h.erase(hb);
    require(h.size(), size_t(3));
    h.erase(hd);
    require(h.size(), size_t(2));
    require(h.extract_min(), 5);
    require(h.extract_min(), 25);
    require_true(h.empty());
  }
  end_test_case();

  test_case("meld absorbs another heap, source emptied");
  {
    micron::quake_heap<int> a;
    for ( int i = 0; i < 8; ++i ) a.insert(i * 2);
    micron::quake_heap<int> b;
    for ( int i = 0; i < 8; ++i ) b.insert(i * 2 + 1);
    a.meld(micron::move(b));
    require(a.size(), size_t(16));
    require_true(b.empty());
    require(b.size(), size_t(0));
    int prev = -1;
    for ( int i = 0; i < 16; ++i ) {
      int cur = a.extract_min();
      require(cur, i);
      require_true(cur > prev);
      prev = cur;
    }
  }
  end_test_case();

  test_case("max-heap via __quake_greater drains descending");
  {
    micron::quake_heap<int, micron::__quake_greater<int>> g;
    int in[] = { 3, 1, 4, 1, 5, 9, 2, 6 };
    for ( int x : in ) g.insert(int(x));
    require(g.find_min(), 9);
    int prev = 1000000;
    while ( !g.empty() ) {
      int cur = g.extract_min();
      require_true(cur <= prev);
      prev = cur;
    }
  }
  end_test_case();

  test_case("SIMD-vs-scalar parity: default vs custom-comparator give same drain");
  {
    constexpr int N = 4096;
    micron::quake_heap<int> hs;
    micron::quake_heap<int, scalar_less> hc;
    u64 save = __s;
    for ( int i = 0; i < N; ++i ) {
      int v = static_cast<int>(rnd() % 1000000);
      hs.insert(v);
      hc.insert(v);
    }
    bool ok = true;
    for ( int i = 0; i < N; ++i ) {
      int a = hs.extract_min();
      int b = hc.extract_min();
      if ( a != b ) ok = false;
    }
    require_true(ok);
    require_true(hs.empty() && hc.empty());
    __s = save;
  }
  end_test_case();

  test_case("SIMD argmin over a large root forest (decrease_key flood then drain)");
  {
    constexpr int N = 1200;
    micron::quake_heap<int> h;
    static micron::quake_heap<int>::handle hs[N];
    for ( int i = 0; i < N; ++i ) hs[i] = h.insert(1000000 + i);

    for ( int i = 0; i < N / 2; ++i ) h.decrease_key(hs[i], i);
    require(h.find_min(), 0);
    int prev = -1;
    int cnt = 0;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, N);
  }
  end_test_case();

  test_case("move construction transfers, source emptied");
  {
    micron::quake_heap<int> a;
    for ( int i = 0; i < 6; i++ ) a.insert(int(i));
    micron::quake_heap<int> b(micron::move(a));
    require(b.size(), size_t(6));
    require_true(a.empty());
    require(b.find_min(), 0);
  }
  end_test_case();

  test_case("move-assign onto NON-EMPTY destination (no leak/double-free)");
  {
    micron::quake_heap<int> dst;
    for ( int i = 0; i < 64; i++ ) dst.insert(int(i));
    micron::quake_heap<int> src;
    for ( int i = 0; i < 10; i++ ) src.insert(int(1000 + i));
    dst = micron::move(src);
    require(dst.size(), size_t(10));
    require(dst.find_min(), 1000);
  }
  end_test_case();

  test_case("self move-assign is safe");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 4; i++ ) h.insert(int(i));
    h = micron::move(h);
    require(h.size(), size_t(4));
  }
  end_test_case();

  test_case("clear empties and stays usable");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 20; i++ ) h.insert(int(i));
    h.clear();
    require_true(h.empty());
    h.insert(42);
    require(h.find_min(), 42);
  }
  end_test_case();

  test_case("stress: 5000 elements drain fully sorted ascending");
  {
    micron::quake_heap<int> h;
    for ( int i = 0; i < 5000; i++ ) h.insert((i * 7919) % 104729);
    require(h.size(), size_t(5000));
    int prev = -1000000, cnt = 0;
    while ( !h.empty() ) {
      int cur = h.extract_min();
      require_true(cur >= prev);
      prev = cur;
      ++cnt;
    }
    require(cnt, 5000);
  }
  end_test_case();

  test_case("lifetime: non-trivial elements balance ctor/dtor (entry + node pools)");
  {
    counted::live = 0;
    {
      micron::quake_heap<counted> h;
      for ( int i = 0; i < 80; i++ ) h.insert(counted(i));
      for ( int i = 0; i < 30; i++ ) (void)h.extract_min();
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: decrease_key + erase + clear keep counted balanced");
  {
    counted::live = 0;
    {
      micron::quake_heap<counted> h;
      static micron::quake_heap<counted>::handle hs[60];
      for ( int i = 0; i < 60; i++ ) hs[i] = h.insert(counted(i + 100));
      for ( int i = 0; i < 60; i += 2 ) h.decrease_key(hs[i], counted(i - 1));
      for ( int i = 1; i < 20; i += 2 ) h.erase(hs[i]);
      (void)h.extract_min();
      (void)h.extract_min();
      h.clear();
      require_true(h.empty());
    }
    require(counted::live, 0L);
  }
  end_test_case();

  test_case("lifetime: move-assign over non-empty destroys old elements");
  {
    counted::live = 0;
    {
      micron::quake_heap<counted> dst;
      for ( int i = 0; i < 40; i++ ) dst.insert(counted(i));
      micron::quake_heap<counted> src;
      for ( int i = 0; i < 5; i++ ) src.insert(counted(100 + i));
      dst = micron::move(src);
    }
    require(counted::live, 0L);
  }
  end_test_case();

  sb::print("=== ALL QUAKE_HEAP TESTS PASSED ===");
  return 1;
}
