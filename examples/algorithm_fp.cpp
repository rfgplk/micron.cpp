// algorithm_fp.cpp
// micron's fp:: namespace — functional-programming algorithms.
// src/algorithm/fp.hpp re-exports everything from the fp:: submodules.
//
// See also:
//   examples/algorithm.cpp           — basic range algorithms
//   examples/algorithm_fold_filter.cpp — fold + filter primitives
//   examples/sum.cpp                 — sum types (option / any / variant)
//
// All fp:: functions are in the micron namespace (exported via
// `using fp::...`). The names track the Haskell vocabulary:
//
//   fmap(fn, container)  — Functor map: T -> U over a container
//   scanl / scanr        — Prefix scans (running folds)
//   zip_with(a, b, fn)   — Element-wise binary op; returns
//                          option<C, bad_zip_error> on size mismatch
//   traverse(c, fn)      — Kleisli map: fn returns option<T,E>;
//                          short-circuit on first error
//   partition(c, fn)     — Split into (matching, non-matching)
//   take / drop          — Prefix / suffix selection
//   flat_map             — Map then flatten
//   nub                  — Remove all duplicates
//   unique               — Remove consecutive duplicates
//   replicate            — N copies of a value
//   zip_with_trunc       — Like zip_with, but truncates instead of erroring

#include "../src/algorithm/fp.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

// println(vec) gives "{ 1, 2, 3 }"; this thin wrapper just prefixes
// a label.
static void
print_vec(const char *label, const micron::vector<int> &v)
{
  micron::io::println(label, ": ", v);
}

int
main()
{
  // ================================================================
  // fmap — Functor map: T -> T (endomorphism)
  // ================================================================
  micron::vector<int> v({1, 2, 3, 4, 5});

  // Lambda form: fn_codomain (T -> T)
  auto squared = micron::fmap([](int x) { return x * x; }, v);
  print_vec("fmap(x^2)", squared);

  // Pointer form: fn_arrow (T* -> T) — use when you need the address
  auto doubled = micron::fmap([](int *p) { return *p * 2; }, v);
  print_vec("fmap(*2, ptr)", doubled);

  // fmap_into — map from one element type to another
  auto as_f64 = micron::fmap_into<micron::vector<f64>>(
      [](int x) { return static_cast<f64>(x) * 0.5; }, v);
  micron::io::println("fmap_into<f64>: [0]=", as_f64[0], " [4]=", as_f64[4]);

  // fmap_c — curried form (returns a lambda that waits for its container)
  auto square_fn = micron::fmap_c([](int x) { return x * x; });
  auto sq2 = square_fn(v);
  print_vec("fmap_c(x^2)", sq2);

  // ================================================================
  // scanl / scanr — prefix scans (running fold)
  // scanl: left-to-right, each element = fold up to that point
  // scanr: right-to-left
  // ================================================================
  micron::vector<int> nums({1, 2, 3, 4, 5});

  // scanl produces running sum (same size as input, each elem = partial sum)
  auto run_sum = micron::scanl(nums, 0, [](int acc, int x) { return acc + x; });
  print_vec("scanl(+, 0)", run_sum);   // [1, 3, 6, 10, 15]

  auto run_prod = micron::scanr(nums, 1, [](int x, int acc) { return x * acc; });
  print_vec("scanr(*, 1)", run_prod);  // [120, 120, 60, 20, 5]

  // ================================================================
  // zip_with — element-wise binary operation
  // Returns option<C, bad_zip_error> — fails if sizes differ
  // ================================================================
  micron::vector<int> a({1, 2, 3, 4});
  micron::vector<int> b({10, 20, 30, 40});

  auto zipped = micron::zip_with(a, b, [](int x, int y) { return x + y; });
  if ( zipped.is_first() )
    print_vec("zip_with(+)", zipped.cast<micron::vector<int>>());

  // zip_with on mismatched sizes returns error
  micron::vector<int> shorter({1, 2});
  auto bad_zip = micron::zip_with(a, shorter, [](int x, int y) { return x + y; });
  micron::io::println("zip_with mismatch is_error=", bad_zip.is_second());

  // zip_with_trunc — truncates to the shorter length, never errors
  auto truncated = micron::zip_with_trunc(a, shorter, [](int x, int y) { return x * y; });
  print_vec("zip_with_trunc", truncated);   // only 2 elements

  // ================================================================
  // partition — split into (matching, non-matching)
  // Returns a tuple/pair of containers
  // ================================================================
  micron::vector<int> mixed({1, 2, 3, 4, 5, 6, 7, 8});
  // partition returns micron::tuple<C,C>; use micron::get<> to access
  auto parts  = micron::partition(mixed, [](int x) { return x % 2 != 0; });
  auto odds   = micron::get<0>(parts);
  auto evens_p = micron::get<1>(parts);
  print_vec("partition odds", odds);
  print_vec("partition evens", evens_p);

  // ================================================================
  // take / drop — prefix and suffix
  // ================================================================
  micron::vector<int> seq({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  auto first3 = micron::take(seq, 3);
  auto skip3  = micron::drop(seq, 3);
  print_vec("take(3)", first3);
  print_vec("drop(3)", skip3);

  // take_while / drop_while — predicate-based prefix/suffix
  auto below5 = micron::take_while(seq, [](int x) { return x < 5; });
  auto from5  = micron::drop_while(seq, [](int x) { return x < 5; });
  print_vec("take_while(<5)", below5);
  print_vec("drop_while(<5)", from5);

  // ================================================================
  // flat_map — map then flatten (monad bind for containers)
  // fn: T -> C (returns a container for each element)
  // ================================================================
  micron::vector<int> roots({1, 2, 3});
  // For each n, emit [n, n*10]
  auto flat = micron::flat_map(roots,
      [](int n) { return micron::vector<int>({n, n * 10}); });
  print_vec("flat_map([n, n*10])", flat);   // [1, 10, 2, 20, 3, 30]

  // ================================================================
  // nub — remove ALL duplicates (keeps first occurrence)
  // unique — remove CONSECUTIVE duplicates only
  // ================================================================
  micron::vector<int> dupes({3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5});
  auto no_dupes = micron::nub(dupes);
  print_vec("nub", no_dupes);   // each value appears once

  micron::vector<int> consec({1, 1, 2, 3, 3, 3, 4, 4});
  auto consec_unique = micron::unique(consec);
  print_vec("unique (consec)", consec_unique);   // [1, 2, 3, 4]

  // ================================================================
  // replicate — fill container with N copies of a value
  // ================================================================
  auto reps = micron::replicate<micron::vector<int>>(5, 42);
  print_vec("replicate(5, 42)", reps);

  // ================================================================
  // ap — applicative: apply a container of functions to a container of values
  // ================================================================
  // Build vector<function<>> via push_back (init-list not supported for function<>)
  micron::vector<micron::function<int(int)>> fns;
  fns.push_back(micron::function<int(int)>([](int x) { return x + 1; }));
  fns.push_back(micron::function<int(int)>([](int x) { return x * 2; }));

  micron::vector<int> vals({10, 20});
  // ap applies each fn to each val pairwise, returns option on size mismatch
  auto applied = micron::ap(fns, vals);
  if ( applied.is_first() )
    print_vec("ap(fns, vals)", applied.cast<micron::vector<int>>());

  // ================================================================
  // traverse — Kleisli: fn returns option<T,E>; short-circuits on error
  // ================================================================
  micron::vector<int> positives({1, 2, 3, 4});
  micron::vector<int> with_neg({1, 2, -3, 4});

  // Safe sqrt: returns option<int, micron::runtime> (None for negatives)
  auto safe_sqrt = [](int x) -> micron::option<int, micron::runtime> {
    if ( x < 0 ) return micron::runtime("negative value");
    return static_cast<int>(x);   // simplified
  };

  auto ok  = micron::traverse(positives, safe_sqrt);
  auto err = micron::traverse(with_neg,  safe_sqrt);
  micron::io::println("traverse all_pos=", ok.is_first(), " with_neg=", err.is_second());

  // ================================================================
  // safe_max / safe_min / safe_sum / safe_mean
  // Return option<T, empty_container_error> — handles empty containers
  // ================================================================
  micron::vector<int> data({3, 1, 4, 1, 5, 9, 2, 6});
  auto mx  = micron::safe_max(data);
  auto mn  = micron::safe_min(data);
  auto sm  = micron::safe_sum(data);

  if ( mx.is_first() ) micron::io::println("safe_max=", mx.cast<int>());
  if ( mn.is_first() ) micron::io::println("safe_min=", mn.cast<int>());
  if ( sm.is_first() ) micron::io::println("safe_sum=", sm.cast<umax_t>());

  micron::vector<int> empty_v;
  auto empty_max = micron::safe_max(empty_v);
  micron::io::println("safe_max(empty) is_error=", empty_max.is_second());

  // ================================================================
  // on — composition with a projection function
  // on(f, g)(x, y) == f(g(x), g(y))
  // ================================================================
  auto compare_by_mod3 = micron::on(
      [](int a, int b) { return a < b; },
      [](int x) { return x % 3; });
  micron::io::println("on(compare, mod3): 7<5 = ", compare_by_mod3(7, 5));  // 7%3=1, 5%3=2

  return 0;
}
