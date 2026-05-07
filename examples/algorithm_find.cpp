// algorithm_find.cpp
// micron's find / search algorithms.
//
// See also:
//   examples/algorithm.cpp           — fill / transform / where / fold
//   examples/algorithm_fold_filter.cpp — folds and filters in detail
//   examples/algorithm_fp.cpp        — option-returning fp helpers
//
// Key deviations from std::algorithm:
//   - All find* return raw T* pointers, not iterators. nullptr means
//     "not found" — no end() sentinel comparison needed.
//   - Predicate overloads come in TWO flavors:
//       fn(const T)   — receives the value directly
//       fn(const T*)  — receives a pointer (use when you need the address)
//   - contains / starts_with / ends_with are first-class.
//   - count / count_if operate on raw pointer ranges.

#include "../src/algorithm/find.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  micron::vector<int> v({3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5});

  // ----------------------------------------------------------------
  // find — returns pointer to first match, nullptr if absent
  // Unlike std::find which returns end() for "not found"
  // ----------------------------------------------------------------
  const int *p = micron::find(v.cbegin(), v.cend(), 5);
  micron::io::println("find(5):  found=", p != nullptr, " offset=", p - v.cbegin());

  const int *miss = micron::find(v.cbegin(), v.cend(), 99);
  micron::io::println("find(99): found=", miss != nullptr);

  // ----------------------------------------------------------------
  // find_if — predicate variant, value form fn(const T)
  // ----------------------------------------------------------------
  const int *big = micron::find_if(v.cbegin(), v.cend(), [](int x) { return x > 7; });
  micron::io::println("find_if(>7): value=", big ? *big : -1);

  // find_if — pointer form fn(const T*)
  const int *ptr_form = micron::find_if(v.cbegin(), v.cend(), [](const int *p) { return *p > 7; });
  micron::io::println("find_if ptr form: value=", ptr_form ? *ptr_form : -1);

  // ----------------------------------------------------------------
  // find_if_not — returns first element NOT matching predicate
  // NOTE: all overloads receive a pointer (const T*); dereference to get value
  // ----------------------------------------------------------------
  const int *not_one = micron::find_if_not(v.cbegin(), v.cend(), [](const int *p) { return *p < 5; });
  micron::io::println("find_if_not(<5): value=", not_one ? *not_one : -1);

  // ----------------------------------------------------------------
  // find_last / find_last_if — scan from the end
  // ----------------------------------------------------------------
  const int *last5 = micron::find_last(v.cbegin(), v.cend(), 5);
  micron::io::println("find_last(5): offset=", last5 - v.cbegin());

  const int *last_big = micron::find_last_if(v.cbegin(), v.cend(), [](int x) { return x > 4; });
  micron::io::println("find_last_if(>4): value=", last_big ? *last_big : -1);

  // ----------------------------------------------------------------
  // find_first_of — first element matching any in a set
  // ----------------------------------------------------------------
  micron::array<int, 3> needles({2, 6, 9});
  const int *first_of = micron::find_first_of(
      v.cbegin(), v.cend(), needles.cbegin(), needles.cend());
  micron::io::println("find_first_of({2,6,9}): value=", first_of ? *first_of : -1);

  // ----------------------------------------------------------------
  // adjacent_find — first position where arr[i] == arr[i+1]
  // ----------------------------------------------------------------
  micron::vector<int> adj({1, 2, 2, 3, 3, 3});
  const int *adj_p = micron::adjacent_find(adj.cbegin(), adj.cend());
  micron::io::println("adjacent_find: value=", adj_p ? *adj_p : -1, " offset=", adj_p - adj.cbegin());

  // ----------------------------------------------------------------
  // count / count_if — element counting
  // ----------------------------------------------------------------
  usize cnt5 = micron::count(v.cbegin(), v.cend(), 5);
  micron::io::println("count(5)=", cnt5);

  usize cnt_even = micron::count_if(v.cbegin(), v.cend(), [](int x) { return x % 2 == 0; });
  micron::io::println("count_if(even)=", cnt_even);

  // ----------------------------------------------------------------
  // mismatch — first position where two ranges differ
  // Returns pair<const T*, const T*> pointing to the mismatch
  // ----------------------------------------------------------------
  micron::vector<int> a({1, 2, 3, 4, 5});
  micron::vector<int> b({1, 2, 7, 4, 5});
  auto [ma, mb] = micron::mismatch(a.cbegin(), a.cend(), b.cbegin());
  micron::io::println("mismatch: a_val=", *ma, " b_val=", *mb);

  // ----------------------------------------------------------------
  // equal — compare two ranges element-by-element
  // ----------------------------------------------------------------
  micron::vector<int> c({1, 2, 3});
  micron::vector<int> d({1, 2, 3});
  micron::vector<int> e({1, 2, 4});
  micron::io::println("equal(c,d)=", micron::equal(c.cbegin(), c.cend(), d.cbegin()));
  micron::io::println("equal(c,e)=", micron::equal(c.cbegin(), c.cend(), e.cbegin()));

  // ----------------------------------------------------------------
  // search — find a subsequence (pattern) within a range
  // Returns pointer to the first element of the first match, or nullptr
  // ----------------------------------------------------------------
  micron::vector<int> haystack({1, 2, 3, 4, 5, 6, 7, 8});
  micron::vector<int> needle({4, 5, 6});
  const int *found = micron::search(
      haystack.cbegin(), haystack.cend(),
      needle.cbegin(),   needle.cend());
  micron::io::println("search({4,5,6}): offset=", found ? found - haystack.cbegin() : -1);

  // ----------------------------------------------------------------
  // contains — boolean membership test
  // ----------------------------------------------------------------
  micron::io::println("contains(v, 9)=", micron::contains(v.cbegin(), v.cend(), 9));
  micron::io::println("contains(v, 7)=", micron::contains(v.cbegin(), v.cend(), 7));

  // ----------------------------------------------------------------
  // starts_with / ends_with — prefix / suffix tests
  // ----------------------------------------------------------------
  micron::vector<int> seq({1, 2, 3, 4, 5, 6});
  micron::vector<int> prefix({1, 2, 3});
  micron::vector<int> suffix({4, 5, 6});
  micron::io::println("starts_with({1,2,3})=",
      micron::starts_with(seq.cbegin(), seq.cend(), prefix.cbegin(), prefix.cend()));
  micron::io::println("ends_with({4,5,6})=",
      micron::ends_with(seq.cbegin(), seq.cend(), suffix.cbegin(), suffix.cend()));

  // ----------------------------------------------------------------
  // search_n — n consecutive equal elements
  // ----------------------------------------------------------------
  micron::vector<int> rep({1, 2, 3, 3, 3, 4});
  const int *sn = micron::search_n(rep.cbegin(), rep.cend(), 3, 3);
  micron::io::println("search_n(3, val=3): offset=", sn ? sn - rep.cbegin() : -1);

  return 0;
}
