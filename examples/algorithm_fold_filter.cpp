// algorithm_fold_filter.cpp
// micron's fold and filter algorithms.
//
// See also:
//   examples/algorithm.cpp     — fill / transform / sum / min / max
//   examples/algorithm_find.cpp — find / contains / mismatch
//   examples/algorithm_fp.cpp   — option-returning fp helpers
//
// IMPORTANT deviation: fold_left / fold_right receive a POINTER to
// the current element, not the value itself:
//   fn(accumulator, const T*) -> accumulator   (fold_left)
//   fn(const T*, accumulator) -> accumulator   (fold_right)
// This lets the lambda inspect/skip without copying — but you must
// dereference (`*p`) to read the value.
//
// filter similarly passes fn(const T*).

#include "../src/algorithm/filter.hpp"
#include "../src/algorithm/fold.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  // ================================================================
  // FOLD
  // ================================================================

  micron::vector<int> v({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});

  // --- fold_left: left-associative scan ---
  // fn: (accumulator, const T*) -> accumulator
  int sum = micron::fold_left(v.cbegin(), v.cend(), 0,
      [](int acc, const int *p) { return acc + *p; });
  micron::io::println("fold_left sum 1..10 = ", sum);   // 55

  // Product
  long product = micron::fold_left(v.cbegin(), v.cend(), 1L,
      [](long acc, const int *p) { return acc * *p; });
  micron::io::println("fold_left product 1..10 = ", product);   // 3628800

  // --- fold_left with limit — stop after N elements ---
  int partial = micron::fold_left(v.cbegin(), v.cend(), 0,
      [](int acc, const int *p) { return acc + *p; }, usize(5));
  micron::io::println("fold_left sum first 5 = ", partial);   // 15

  // --- fold_left: container overload ---
  int csum = micron::fold_left(v, 0, [](int acc, const int *p) { return acc + *p; });
  micron::io::println("fold_left container sum = ", csum);

  // --- fold_right: right-associative ---
  // fn: (const T*, accumulator) -> accumulator
  // Processes from end to begin (rightmost element first)
  micron::vector<int> words({1, 2, 3, 4});
  // Build a number by treating elements as digits: 1*1000 + 2*100 + 3*10 + 4
  int decimal = micron::fold_right(words.cbegin(), words.cend(),
      [](const int *p, int acc) { return *p * 10 + acc; }, 0);
  micron::io::println("fold_right build number = ", decimal);   // 1234

  // --- fold_left to build a new container (running maximum) ---
  micron::vector<int> source({3, 1, 4, 1, 5, 9, 2, 6});
  micron::vector<int> running_max;
  running_max.push_back(source[0]);
  micron::fold_left(source.cbegin() + 1, source.cend(), source[0],
      [&running_max](int acc, const int *p) {
        int m = acc > *p ? acc : *p;
        running_max.push_back(m);
        return m;
      });
  micron::io::println("running max: ", running_max);

  // ================================================================
  // FILTER (pointer-predicate form)
  // ================================================================

  micron::vector<int> data({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});

  // --- filter container -> new container ---
  // fn receives const T* — dereference to get value
  auto evens = micron::filter(data, [](const int *p) { return *p % 2 == 0; });
  micron::io::println("filter(even)         : ", evens);

  // --- filter with limit ---
  auto first3_even = micron::filter(data, [](const int *p) { return *p % 2 == 0; }, usize(3));
  micron::io::println("filter(even, limit=3): ", first3_even);

  // --- filter into existing output container ---
  micron::vector<int> out_buf;
  out_buf.resize(data.size(), 0);
  micron::filter(data, [](const int *p) { return *p > 5; }, out_buf);
  micron::io::println("filter into out_buf (>5): ", out_buf);

  // --- filter raw pointer range -> output buffer ---
  int raw[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int out[10] = {};
  int *end_ptr = micron::filter(raw, raw + 10,
      [](const int *p) { return *p % 3 == 0; }, out);
  usize n_out = end_ptr - out;
  micron::io::print("filter raw (div3): count=", n_out, " values: ");
  for ( usize i = 0; i < n_out; ++i ) micron::io::print(out[i], " ");
  micron::io::println("");

  // --- filter into a pre-sized output container then resize ---
  // (filter_inplace is not currently stable; use raw pointer filter + resize)
  micron::vector<int> mutable_data({1, 2, 3, 4, 5, 6, 7, 8});
  micron::vector<int> odd_out;
  odd_out.resize(mutable_data.size(), 0);
  int *odd_end = micron::filter(mutable_data.cbegin(), mutable_data.cend(),
      [](const int *p) { return *p % 2 != 0; }, odd_out.begin());
  odd_out.resize(static_cast<usize>(odd_end - odd_out.begin()));
  micron::io::println("filter(odd) : ", odd_out, "  size=", odd_out.size());

  // --- prune — iterate until limit is hit, return first unprocessed pointer ---
  // Useful for streaming/chunked processing
  micron::vector<int> stream({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  micron::vector<int> chunk_out;
  chunk_out.resize(3, 0);
  auto unprocessed = micron::prune(stream.cbegin(), stream.cend(),
      [](const int *p) { return *p % 2 == 0; },
      chunk_out.begin(), usize(3));
  micron::io::println("prune(even, 3 max): ", chunk_out,
                      "  remaining at offset=", unprocessed - stream.cbegin());

  return 0;
}
