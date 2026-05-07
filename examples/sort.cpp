// sort.cpp
// Tour of micron::sort:: — sorting algorithms in src/sort/.
//
// Every sort lives in the `micron::sort::` sub-namespace so the names
// don't collide with each other or with `micron::sum`/`micron::min` etc.
//
// The algorithms expose two shapes:
//   sort::ALGO(container)                 — sort the whole container
//   sort::ALGO(container, comp)           — same with a custom comparator
//   sort::ALGO(begin, end)                — iterator pair (some algos)
//   sort::ALGO(begin, end, comp)          — iterator pair + comp
//
// What's available (each is in its own header in src/sort/):
//   insertion / bubble / counting / radix / merge / quick / heap /
//   selection (in select.hpp) / fradix (radix on floats).
//
// Algorithms are concept-constrained on `is_iterable_container`, so any
// micron container with begin/end works — vector, array, span, etc.
//
// STL deltas:
//   - Names are explicit: you ask for the algorithm by name (`quick`,
//     `radix`, `bitonic`...), not the generic `std::sort`.
//   - radix and counting are first-class — no need to roll your own.
//   - Whole-container overloads exist; iterator pairs are not the only
//     way to call them.

#include "../src/io/console.hpp"
#include "../src/sort/sorts.hpp"
#include "../src/vector/vector.hpp"

template <typename T>
static micron::vector<T>
fresh()
{
  return micron::vector<T>({3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5});
}

int
main()
{
  // ================================================================
  // 1. sort::insertion — small / nearly-sorted arrays
  // ================================================================
  micron::io::println("-- 1. insertion --");
  {
    auto v = fresh<int>();
    micron::sort::insertion(v);
    micron::io::println("insertion = ", v);
  }

  // Custom comparator — descending
  {
    auto v = fresh<int>();
    micron::sort::insertion(v, [](int a, int b) { return a > b; });
    micron::io::println("insertion desc = ", v);
  }

  // Limit overload — sort only the first 5 elements via the (arr, lim) form
  {
    auto v = fresh<int>();
    micron::sort::insertion(v, (decltype(v)::size_type)5);
    micron::io::println("insertion partial[0..5) = ", v);
  }

  // ================================================================
  // 2. sort::bubble — for completeness; O(n^2)
  // ================================================================
  micron::io::println("-- 2. bubble --");
  {
    auto v = fresh<int>();
    micron::sort::bubble(v);
    micron::io::println("bubble = ", v);
  }

  // ================================================================
  // 3. sort::selection — O(n^2), in-place, minimal swaps
  // ================================================================
  micron::io::println("-- 3. selection --");
  {
    auto v = fresh<int>();
    micron::sort::selection(v);
    micron::io::println("selection = ", v);
  }

  // ================================================================
  // 4. sort::quick — the workhorse, O(n log n) average
  // ================================================================
  micron::io::println("-- 4. quick --");
  {
    auto v = fresh<int>();
    micron::sort::quick(v);
    micron::io::println("quick = ", v);
  }

  // ================================================================
  // 5. sort::merge — stable, O(n log n) worst case
  // ================================================================
  micron::io::println("-- 5. merge --");
  {
    auto v = fresh<int>();
    micron::sort::merge(v);
    micron::io::println("merge = ", v);
  }

  // ================================================================
  // 6. sort::as_heap — make_heap + heap, full sort
  // ----------------------------------------------------------------
  // sort::heap on its own assumes the input IS already a heap (it's
  // the equivalent of std::sort_heap). Use sort::as_heap to combine
  // make_heap + heap into a full sort. sort::make_heap also exists
  // separately.
  // ================================================================
  micron::io::println("-- 6. as_heap --");
  {
    auto v = fresh<int>();
    micron::sort::as_heap(v);
    micron::io::println("as_heap = ", v);
  }

  // ================================================================
  // 7. sort::counting — integer LSD; available but specialised
  // ----------------------------------------------------------------
  // Suitable for narrow integer ranges. See src/sort/counting.hpp.
  // (Skipped here since the example aim is to show the API surface.)
  // ================================================================

  // ================================================================
  // 8. sort::fradix — radix sort on floats
  // ----------------------------------------------------------------
  // Byte-wise radix sort over the float bit pattern. Linear in n.
  // (sort::radix on integers also exists; see src/sort/radix.hpp)
  // ================================================================
  micron::io::println("-- 8. fradix --");
  {
    micron::vector<f32> v({3.14f, 1.41f, 2.71f, 0.57f, 1.61f});
    micron::sort::fradix(v);
    micron::io::println("fradix f32 = ", v);
  }

  // ================================================================
  // 9. Sorting other container types
  // ----------------------------------------------------------------
  // Anything modelling is_iterable_container plugs in. Here a span
  // (stack, fixed cap) goes through sort::quick.
  // ================================================================
  micron::io::println("-- 9. across containers --");
  {
    micron::vector<int> v({5, 1, 4, 1, 5, 9});
    micron::sort::quick(v);
    micron::io::println("vector via sort::quick = ", v);
  }

  return 0;
}
