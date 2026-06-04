//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/array.hpp"
#include "../../src/heap/binary_heap.hpp"
#include "../../src/heap/quake_heap.hpp"
#include "../../src/sort/sorts.hpp"
#include "../../src/trees/rb.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{
using namespace micron;

u64 g_seed = 0x9E3779B97F4A7C15ULL;

int
rnd(int lo, int hi) noexcept
{
  g_seed = g_seed * 6364136223846793005ULL + 1442695040888963407ULL;
  const u64 span = static_cast<u64>(hi - lo + 1);
  return lo + static_cast<int>((g_seed >> 20) % span);
}

void
ref_sort(int *a, int n) noexcept
{
  for ( int i = 1; i < n; ++i ) {
    int k = a[i];
    int j = i - 1;
    while ( j >= 0 && a[j] > k ) {
      a[j + 1] = a[j];
      --j;
    }
    a[j + 1] = k;
  }
}

const int SIZES[] = { 0, 1, 2, 3, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 127, 255, 256, 257, 1000, 1024, 4096 };
constexpr int N_SIZES = static_cast<int>(sizeof(SIZES) / sizeof(SIZES[0]));

bool
verify(vector<int> &v, const int *input, int n, bool asc)
{
  if ( static_cast<int>(v.size()) != n ) return false;
  for ( int i = 1; i < n; ++i )
    if ( asc ? (v[i - 1] > v[i]) : (v[i - 1] < v[i]) ) return false;
  long long s1 = 0, s2 = 0;
  u64 x1 = 0, x2 = 0;
  for ( int i = 0; i < n; ++i ) {
    s1 += v[i];
    s2 += input[i];
    x1 ^= static_cast<u64>(static_cast<u32>(v[i])) * 0x9E3779B97F4A7C15ULL;
    x2 ^= static_cast<u64>(static_cast<u32>(input[i])) * 0x9E3779B97F4A7C15ULL;
  }
  if ( s1 != s2 || x1 != x2 ) return false;
  if ( n <= 257 ) {
    int *ref = new int[n > 0 ? n : 1];
    for ( int i = 0; i < n; ++i ) ref[i] = input[i];
    ref_sort(ref, n);
    bool ok = true;
    for ( int i = 0; ok && i < n; ++i )
      if ( v[i] != (asc ? ref[i] : ref[n - 1 - i]) ) ok = false;
    delete[] ref;
    if ( !ok ) return false;
  }
  return true;
}

template<typename Sorter>
bool
sweep_asc(Sorter s)
{
  for ( int si = 0; si < N_SIZES; ++si ) {
    const int n = SIZES[si];
    vector<int> v;
    int *ref = new int[n > 0 ? n : 1];
    for ( int i = 0; i < n; ++i ) {
      int x = rnd(-5000, 5000);
      v.push_back(x);
      ref[i] = x;
    }
    s(v);
    bool ok = verify(v, ref, n, true);
    delete[] ref;
    if ( !ok ) return false;
  }
  return true;
}

template<typename Sorter>
bool
sweep_desc(Sorter s)
{
  for ( int si = 0; si < N_SIZES; ++si ) {
    const int n = SIZES[si];
    vector<int> v;
    int *ref = new int[n > 0 ? n : 1];
    for ( int i = 0; i < n; ++i ) {
      int x = rnd(-5000, 5000);
      v.push_back(x);
      ref[i] = x;
    }
    s(v);
    bool ok = verify(v, ref, n, false);
    delete[] ref;
    if ( !ok ) return false;
  }
  return true;
}

struct kv {
  int key;
  int tag;
};

template<typename Sorter>
bool
stable_ok(Sorter s, int n, int K)
{
  vector<kv> v;
  for ( int i = 0; i < n; ++i ) v.push_back(kv{ rnd(0, K - 1), i });
  s(v);
  for ( usize i = 1; i < v.size(); ++i ) {
    if ( v[i - 1].key > v[i].key ) return false;
    if ( v[i - 1].key == v[i].key && v[i - 1].tag > v[i].tag ) return false;
  }
  return true;
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== SORT RIGOR TESTS ===");

  test_case("sort(intro): ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::sort(v); }));
  end_test_case();
  test_case("quick: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::quick(v); }));
  end_test_case();
  test_case("merge: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::merge(v); }));
  end_test_case();
  test_case("merge_bottom_up: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::merge_bottom_up(v); }));
  end_test_case();
  test_case("heap: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::heap(v); }));
  end_test_case();
  test_case("insertion: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::insertion(v); }));
  end_test_case();
  test_case("shell: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::shell(v); }));
  end_test_case();
  test_case("comb: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::comb(v); }));
  end_test_case();
  test_case("bubble: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::bubble(v); }));
  end_test_case();
  test_case("selection: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::selection(v); }));
  end_test_case();
  test_case("bitonic: ascending sweep (pow2 SIMD + non-pow2 fallback)");
  require_true(sweep_asc([](vector<int> &v) { sort::bitonic(v); }));
  end_test_case();
  test_case("stable: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::stable(v); }));
  end_test_case();
  test_case("counting: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::counting(v); }));
  end_test_case();
  test_case("radix: ascending sweep");
  require_true(sweep_asc([](vector<int> &v) { sort::radix(v); }));
  end_test_case();

  auto gt = [](int a, int b) { return a > b; };
  test_case("descending comparator: sort/quick/merge/heap/insertion/shell/comb/bubble/selection/bitonic/stable");
  {
    bool ok = sweep_desc([gt](vector<int> &v) { sort::sort(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::quick(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::merge(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::heap(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::insertion(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::shell(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::comb(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::bubble(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::selection(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::bitonic(v, gt); });
    ok = ok && sweep_desc([gt](vector<int> &v) { sort::stable(v, gt); });
    require_true(ok);
  }
  end_test_case();

  test_case("iterator-pair: quick/merge/insertion/bubble/selection");
  {
    bool ok = sweep_asc([](vector<int> &v) { sort::quick<vector<int>>(v.begin(), v.end()); });
    ok = ok && sweep_asc([](vector<int> &v) { sort::merge<vector<int>>(v.begin(), v.end()); });
    ok = ok && sweep_asc([](vector<int> &v) { sort::insertion<vector<int>>(v.begin(), v.end()); });
    ok = ok && sweep_asc([](vector<int> &v) { sort::bubble<vector<int>>(v.begin(), v.end()); });
    ok = ok && sweep_asc([](vector<int> &v) { sort::selection<vector<int>>(v.begin(), v.end()); });
    require_true(ok);
  }
  end_test_case();

  test_case("lim overloads: quick/merge/merge_bottom_up/insertion/as_heap sort the prefix");
  {
    bool ok = true;
    auto chk = [&](auto sorter) {
      vector<int> v;
      for ( int i = 0; i < 100; ++i ) v.push_back(100 - i);
      sorter(v, (usize)40);
      for ( usize i = 1; i < 40; ++i )
        if ( v[i - 1] > v[i] ) ok = false;
    };
    chk([](vector<int> &v, usize k) { sort::quick(v, k); });
    chk([](vector<int> &v, usize k) { sort::merge(v, k); });
    chk([](vector<int> &v, usize k) { sort::merge_bottom_up(v, k); });
    chk([](vector<int> &v, usize k) { sort::insertion(v, k); });
    chk([](vector<int> &v, usize k) { sort::as_heap(v, k); });
    require_true(ok);
  }
  end_test_case();

  test_case("key-projection: counting/radix sort structs by integer key; fradix by float key");
  {
    bool ok = true;
    for ( int n : { 1, 5, 64, 1000 } ) {
      vector<kv> a, b, c;
      for ( int i = 0; i < n; ++i ) {
        int k = rnd(-2000, 2000);
        a.push_back(kv{ k, i });
        b.push_back(kv{ k, i });
        c.push_back(kv{ k, i });
      }
      sort::counting(a, [](const kv &x) { return x.key; });
      sort::radix(b, [](const kv &x) { return x.key; });
      sort::fradix(c, [](const kv &x) { return (float)x.key; });
      for ( usize i = 1; i < a.size(); ++i )
        if ( a[i - 1].key > a[i].key || b[i - 1].key > b[i].key || c[i - 1].key > c[i].key ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("counting wide/sparse keys -> radix fallback, still sorted");
  {
    vector<int> v;
    v.push_back(0);
    v.push_back(2000000000);
    v.push_back(-2000000000);
    v.push_back(7);
    v.push_back(-7);
    sort::counting(v);
    bool ok = sort::is_sorted(v);
    require_true(ok);
  }
  end_test_case();

  test_case("fradix: floats incl negatives sort ascending");
  {
    const float in[8] = { 3.5f, -1.0f, 0.0f, -7.25f, 2.0f, -0.5f, 100.0f, -100.0f };
    const float rd[8] = { -100.0f, -7.25f, -1.0f, -0.5f, 0.0f, 2.0f, 3.5f, 100.0f };
    vector<float> v;
    for ( int i = 0; i < 8; ++i ) v.push_back(in[i]);
    sort::fradix(v);
    bool ok = (v.size() == 8);
    for ( int i = 0; ok && i < 8; ++i )
      if ( v[i] != rd[i] ) ok = false;
    require_true(ok);
  }
  end_test_case();

  test_case("nth_element: a[k] is k-th smallest, partitioned around it");
  {
    bool ok = true;
    for ( int n : { 1, 2, 33, 1001 } ) {
      vector<int> v;
      for ( int i = 0; i < n; ++i ) v.push_back(rnd(-1000, 1000));
      const usize k = (usize)(n / 2);
      sort::nth_element(v, k);
      const int piv = v[k];
      for ( usize i = 0; i < k; ++i )
        if ( v[i] > piv ) ok = false;
      for ( usize i = k + 1; i < v.size(); ++i )
        if ( v[i] < piv ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();
  test_case("quickselect: returns k-th smallest");
  {
    vector<int> v;
    for ( int i = 0; i < 200; ++i ) v.push_back(200 - i);
    bool ok
        = (sort::quickselect(v, (usize)0) == 1) && (sort::quickselect(v, (usize)199) == 200) && (sort::quickselect(v, (usize)99) == 100);
    require_true(ok);
  }
  end_test_case();
  test_case("partial_sort: first k are the k smallest, in order");
  {
    bool ok = true;
    vector<int> v;
    for ( int i = 0; i < 1000; ++i ) v.push_back(rnd(0, 100000));
    sort::partial_sort(v, (usize)20);
    for ( usize i = 1; i < 20; ++i )
      if ( v[i - 1] > v[i] ) ok = false;
    const int mx = v[19];
    for ( usize i = 20; i < v.size(); ++i )
      if ( v[i] < mx ) ok = false;
    require_true(ok);
  }
  end_test_case();
  test_case("is_sorted / is_sorted_until");
  {
    vector<int> v;
    for ( int i = 0; i < 64; ++i ) v.push_back(i);
    bool ok = sort::is_sorted(v) && (sort::is_sorted_until(v) == v.cend());
    v[40] = -1;
    ok = ok && !sort::is_sorted(v) && (sort::is_sorted_until(v) == v.cbegin() + 40);
    require_true(ok);
  }
  end_test_case();

  test_case("extract: sorted() over rb_tree (for_each), drain over binary_heap/quake_heap");
  {
    bool ok = true;
    int xs[] = { 9, 3, 7, 1, 8, 2, 6, 0, 5, 4 };
    {
      rb_tree<int> t;
      for ( int x : xs ) t.insert(x);
      auto s = sort::sorted(t);
      ok = ok && (s.size() == 10);
      for ( int i = 0; ok && i < 10; ++i )
        if ( s[i] != i ) ok = false;
    }
    {
      binary_heap<int> h(16);
      for ( int x : xs ) h.insert(micron::move(x));
      auto s = sort::drain_sorted(h);
      ok = ok && (s.size() == 10) && (h.size() == 0);
      for ( int i = 0; ok && i < 10; ++i )
        if ( s[i] != 9 - i ) ok = false;
    }
    {
      quake_heap<int> h;
      for ( int x : xs ) h.insert(x);
      auto s = sort::drain_sorted(h);
      ok = ok && (s.size() == 10);
      for ( int i = 0; ok && i < 10; ++i )
        if ( s[i] != i ) ok = false;
    }
    {
      vector<int> v;
      for ( int i = 0; i < 50; ++i ) v.push_back(rnd(0, 1000));
      vector<int> out;
      sort::sort_into(v, out);
      ok = ok && (out.size() == 50) && sort::is_sorted(out);
    }
    require_true(ok);
  }
  end_test_case();

  test_case("SIMD bitonic == scalar bitonic (i32 & f32, large pow2)");
  {
    bool ok = true;
    for ( usize n : { (usize)256, (usize)1024, (usize)16384 } ) {
      vector<int> a, b;
      for ( usize i = 0; i < n; ++i ) {
        int x = rnd(-100000, 100000);
        a.push_back(x);
        b.push_back(x);
      }
      sort::bitonic(a);
      sort::__bitonic(b, [](const int &x, const int &y) { return x < y; });
      for ( usize i = 0; ok && i < n; ++i )
        if ( a[i] != b[i] ) ok = false;
    }
    for ( usize n : { (usize)256, (usize)4096 } ) {
      vector<float> a, b;
      for ( usize i = 0; i < n; ++i ) {
        float x = (float)rnd(-100000, 100000) * 0.5f;
        a.push_back(x);
        b.push_back(x);
      }
      sort::bitonic(a);
      sort::__bitonic(b, [](const float &x, const float &y) { return x < y; });
      for ( usize i = 0; ok && i < n; ++i )
        if ( a[i] != b[i] ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("stability: merge/stable/insertion/bubble (Cmp) + counting/radix (key) preserve tie order");
  {
    auto kcmp = [](const kv &a, const kv &b) { return a.key < b.key; };
    auto kproj = [](const kv &x) { return x.key; };
    bool ok = true;
    ok = ok && stable_ok([kcmp](vector<kv> &v) { sort::merge(v, kcmp); }, 300, 11);
    ok = ok && stable_ok([kcmp](vector<kv> &v) { sort::stable(v, kcmp); }, 300, 11);
    ok = ok && stable_ok([kcmp](vector<kv> &v) { sort::insertion(v, kcmp); }, 300, 11);
    ok = ok && stable_ok([kcmp](vector<kv> &v) { sort::bubble(v, kcmp); }, 300, 11);
    ok = ok && stable_ok([kproj](vector<kv> &v) { sort::counting(v, kproj); }, 300, 11);
    ok = ok && stable_ok([kproj](vector<kv> &v) { sort::radix(v, kproj); }, 300, 11);
    require_true(ok);
  }
  end_test_case();

  test_case("container fan-out: svector + array sort correctly");
  {
    bool ok = true;
    {
      svector<int, 256> v;
      int ref[200];
      for ( int i = 0; i < 200; ++i ) {
        int x = rnd(-1000, 1000);
        v.push_back(x);
        ref[i] = x;
      }
      sort::sort(v);
      ref_sort(ref, 200);
      for ( int i = 0; ok && i < 200; ++i )
        if ( v[i] != ref[i] ) ok = false;
    }
    {
      array<int, 64> a;
      int ref[64];
      for ( int i = 0; i < 64; ++i ) {
        int x = rnd(-1000, 1000);
        a[i] = x;
        ref[i] = x;
      }
      sort::quick(a);
      ref_sort(ref, 64);
      for ( int i = 0; ok && i < 64; ++i )
        if ( a[i] != ref[i] ) ok = false;

      array<int, 64> b;
      for ( int i = 0; i < 64; ++i ) b[i] = ref[63 - i];
      sort::bitonic(b);
      for ( int i = 0; ok && i < 64; ++i )
        if ( b[i] != ref[i] ) ok = false;
    }
    require_true(ok);
  }
  end_test_case();

  test_case("large-scale 65536: sort/quick/merge/heap/radix/counting/bitonic");
  {
    bool ok = true;
    const int n = 65536;
    auto big = [&](auto sorter) {
      vector<int> v;
      int *ref = new int[n];
      for ( int i = 0; i < n; ++i ) {
        int x = rnd(-1000000, 1000000);
        v.push_back(x);
        ref[i] = x;
      }
      sorter(v);
      ok = ok && verify(v, ref, n, true);
      delete[] ref;
    };
    big([](vector<int> &v) { sort::sort(v); });
    big([](vector<int> &v) { sort::quick(v); });
    big([](vector<int> &v) { sort::merge(v); });
    big([](vector<int> &v) { sort::heap(v); });
    big([](vector<int> &v) { sort::radix(v); });
    big([](vector<int> &v) { sort::counting(v); });
    big([](vector<int> &v) { sort::bitonic(v); });
    require_true(ok);
  }
  end_test_case();

  sb::print("=== ALL SORT RIGOR TESTS PASSED ===");
  return 1;
}
