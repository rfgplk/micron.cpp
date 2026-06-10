#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/map.hpp"
#include "../src/vector/vector.hpp"

// algorithm.cpp
// micron::algorithm
// the core range-algorithm library
//
// See also:
//   examples/algorithm_find.cpp        — search/find/index_of
//   examples/algorithm_fold_filter.cpp — fold / filter / reduce in detail
//   examples/algorithm_fp.cpp          — functional programming (option, etc)
//   examples/sort.cpp                  — the dedicated sort namespace
//
// Unlike std::algorithm:
//   - almost all functions accept a whole container OR an iterator pair.
//   - sum() returns the widest representable accumulator type:
//     f128 for floating point, umax_t for integers.
//   - fold_left / fold_right receive a POINTER to the current
//     element, not the value: `[](int acc, const int *p) { ... *p ... }`.
//     This gives the lambda freedom to inspect/skip without copying.

int
main()
{
  // fill writes a value into every element
  micron::array<int, 8> a;
  micron::fill(a.begin(), a.end(), 7);
  // or
  micron::fill(a, 7);
  micron::io::println("fill: ", a);

  // works on any contiguous container
  micron::vector<u64> vec(10);
  micron::fill(vec, 5);
  micron::io::println("fill: ", vec);

  // generate, applies fn to each element of container
  int n = 0;
  micron::generate(a, [&n]() { return n++; });
  micron::io::println("generate: ", a);

  // transform applies a function in place
  micron::array<int, 4> t({ 1, 2, 3, 4 });
  micron::transform(t, [](int x) { return x * x; });
  micron::io::println("transform^2 : ", t);

  // works on maps too
  micron::fast_map<u64> mp;
  mp.insert(0, 2);
  mp.insert(1, 2);
  mp.insert(2, 2);

  micron::transform(mp, [](u64 k, u64 v) -> u64 { return v * v; });
  micron::io::println("transform^2 : ", mp);

  // where copies matching elements into a new container
  micron::vector<int> v({ 1, 2, 3, 4, 5, 6, 7, 8 });
  auto evens = micron::where(v, [](int x) { return x % 2 == 0; });
  micron::io::println("where(even) : ", evens);

  // reverse reverses the container in place
  micron::vector<int> r({ 1, 2, 3, 4, 5 });
  micron::reverse(r);
  micron::io::println("reverse: ", r);

  // rotate rotates the container
  micron::array<int, 4> rot({ 1, 2, 3, 4 });
  micron::rotate_left(rot, 1);
  micron::io::println("rotate_l(1) : ", rot);
  micron::rotate_right(rot, 1);
  micron::io::println("rotate_r(1) : ", rot);

  // shift the container
  micron::array<int, 4> sh({ 10, 20, 30, 40 });
  micron::shift_left(sh, 1);
  micron::io::println("shift_l(1)  : ", sh);      // last slot becomes 0

  // folds containers (in given direction)
  micron::vector<int> fv({ 1, 2, 3, 4, 5 });
  int product = micron::fold_left(fv, 1, [](int acc, const int *p) { return acc * *p; });
  micron::io::println("fold_left * : ", product);

  int sum_r = micron::fold_right(fv, [](const int *p, int acc) { return acc + *p; }, 0);
  micron::io::println("fold_right +: ", sum_r);

  // aggregations over the whole container
  micron::vector<int> agg({ 3, 1, 4, 1, 5, 9, 2, 6 });
  micron::io::println("agg     = ", agg);
  micron::io::println("sum     = ", micron::sum(agg));
  micron::io::println("min     = ", micron::min(agg));
  micron::io::println("max     = ", micron::max(agg));
  micron::io::println("mean    = ", micron::mean(agg));

  // predicate scanning
  micron::vector<int> pv({ 2, 4, 6, 8 });
  micron::io::println("all_even=", micron::all_of(pv.cbegin(), pv.cend(), [](int x) { return x % 2 == 0; }),
                      " any_gt5=", micron::any_of(pv.cbegin(), pv.cend(), [](int x) { return x > 5; }),
                      " none_neg=", micron::none_of(pv.cbegin(), pv.cend(), [](int x) { return x < 0; }));

  return 0;
}
