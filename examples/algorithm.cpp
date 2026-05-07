// algorithm.cpp
// micron::algorithm — the core range-algorithm library.
//
// See also:
//   examples/algorithm_find.cpp        — search/find/index_of
//   examples/algorithm_fold_filter.cpp — fold / filter / reduce in detail
//   examples/algorithm_fp.cpp          — functional programming (option, etc)
//   examples/sort.cpp                  — the dedicated sort namespace
//   examples/io.cpp                    — println(container) showing
//                                        whole-container output
//
// Unlike std::algorithm:
//   - Many functions accept a whole container OR an iterator pair.
//   - sum() returns the widest representable accumulator type:
//     f128 for floating point, umax_t for integers.
//   - fold_left / fold_right receive a POINTER to the current
//     element, not the value: `[](int acc, const int *p) { ... *p ... }`.
//     This gives the lambda freedom to inspect/skip without copying.
//   - where() (filter) returns a new container of matches.
//   - fold.hpp must be included separately.

#include "../src/algorithm/algorithm.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  // ----------------------------------------------------------------
  // fill — write a value into every element
  // ----------------------------------------------------------------
  micron::array<int, 8> a;
  micron::fill(a.begin(), a.end(), 7);
  micron::io::println("fill        : ", a);

  // generate — like fill but takes Fn() -> T (preferred over fill+lambda
  // to avoid ambiguity with the value-fill overload)
  int n = 0;
  micron::generate(a.begin(), a.end(), [&n]() { return n++; });
  micron::io::println("generate    : ", a);

  // ----------------------------------------------------------------
  // transform — apply a function in place
  // ----------------------------------------------------------------
  micron::array<int, 4> t({1, 2, 3, 4});
  micron::transform(t.begin(), t.end(), [](int x) { return x * x; });
  micron::io::println("transform^2 : ", t);

  // ----------------------------------------------------------------
  // where (filter) — copy matching elements into a new container.
  // The result has the SAME capacity as the input — unmatched slots
  // are left default-initialised (zero for ints). Treat the prefix
  // as the matched run; size() reports total slots, not matches.
  // ----------------------------------------------------------------
  micron::vector<int> v({1, 2, 3, 4, 5, 6, 7, 8});
  auto evens = micron::where(v, [](int x) { return x % 2 == 0; });
  micron::io::println("where(even) : ", evens);

  // ----------------------------------------------------------------
  // reverse
  // ----------------------------------------------------------------
  // The whole-container reverse(C&) overload is the safer call. The
  // iterator-pair form expects `end` to point at the LAST element
  // (not one past), which is unusual.
  micron::vector<int> r({1, 2, 3, 4, 5});
  micron::reverse(r);
  micron::io::println("reverse     : ", r);

  // ----------------------------------------------------------------
  // rotate_left / rotate_right
  // ----------------------------------------------------------------
  micron::array<int, 4> rot({1, 2, 3, 4});
  micron::rotate_left(rot.begin(), rot.end(), 1);
  micron::io::println("rotate_l(1) : ", rot);
  micron::rotate_right(rot.begin(), rot.end(), 1);
  micron::io::println("rotate_r(1) : ", rot);

  // ----------------------------------------------------------------
  // shift_left / shift_right — zero-fills exposed positions
  // ----------------------------------------------------------------
  micron::array<int, 4> sh({10, 20, 30, 40});
  micron::shift_left(sh.begin(), sh.end(), 1);
  micron::io::println("shift_l(1)  : ", sh);     // last slot becomes 0

  // ----------------------------------------------------------------
  // fold_left — left reduce
  // fn receives (acc, const T*).  Dereference to get the value.
  // ----------------------------------------------------------------
  micron::vector<int> fv({1, 2, 3, 4, 5});
  int product = micron::fold_left(fv.cbegin(), fv.cend(), 1,
                                  [](int acc, const int *p) { return acc * *p; });
  micron::io::println("fold_left * : ", product);

  int sum_r = micron::fold_right(fv.cbegin(), fv.cend(),
                                 [](const int *p, int acc) { return acc + *p; }, 0);
  micron::io::println("fold_right +: ", sum_r);

  // ----------------------------------------------------------------
  // sum / mean / min / max — whole-container aggregations
  // ----------------------------------------------------------------
  micron::vector<int> agg({3, 1, 4, 1, 5, 9, 2, 6});
  micron::io::println("agg     = ", agg);
  micron::io::println("sum     = ", micron::sum(agg));
  micron::io::println("min     = ", micron::min(agg));
  micron::io::println("max     = ", micron::max(agg));
  micron::io::println("mean    = ", micron::mean(agg));

  // ----------------------------------------------------------------
  // all_of / any_of / none_of — predicate scans
  // ----------------------------------------------------------------
  micron::vector<int> pv({2, 4, 6, 8});
  micron::io::println("all_even=", micron::all_of(pv.cbegin(), pv.cend(),
                                                  [](int x) { return x % 2 == 0; }),
                      " any_gt5=", micron::any_of(pv.cbegin(), pv.cend(),
                                                  [](int x) { return x > 5; }),
                      " none_neg=", micron::none_of(pv.cbegin(), pv.cend(),
                                                    [](int x) { return x < 0; }));

  // ----------------------------------------------------------------
  // clamp — pin a value into [lo, hi]
  // ----------------------------------------------------------------
  micron::io::println("clamp(15, 0, 10) = ", micron::clamp(15, 0, 10));
  micron::io::println("clamp(-3, 0, 10) = ", micron::clamp(-3, 0, 10));

  return 0;
}
