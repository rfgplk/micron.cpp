// range.cpp
// micron's range types from src/range.hpp.
//
// See also:
//   examples/algorithm.cpp  — range-aware algorithms (fill/transform/sum)
//   examples/slice_span.cpp — non-owning views over contiguous memory
//   examples/io.cpp         — println(container) — prints any range source
//
//   range<From, To>       — compile-time unsigned range [From, To)
//   count_range<T,F,To>   — typed range for any arithmetic T
//   int_range<F,To>       — alias for count_range<i32, F, To>
//   float_range<F,To>     — alias for count_range<float, F, To>
//   range_of<T, N>        — adaptor: view first N elements of a container
//
// All ranges are STATIC (no instance state). begin/end/size are static.
// perform(fn) — invoke fn() once per step (no-arg form).
//
// ranges:: namespace — ADL-based free functions (begin, end, size, data, ...)
// that accept any container or raw array.

#include "../src/range.hpp"
#include "../src/vector/vector.hpp"
#include "../src/array/array.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // ================================================================
  // range<From, To> — compile-time unsigned integer range
  // All members are static — no instance needed
  // ================================================================

  micron::io::println("--- range<0, 5> ---");

  using R = micron::range<0, 5>;

  // Iterate with range-based for (uses static begin/end)
  micron::io::print("range-for: ");
  for ( auto it = R::begin(); it != R::end(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // size / ssize
  micron::io::println("size=", R::size(), " ssize=", R::ssize());

  // Reverse iteration
  micron::io::print("reverse: ");
  for ( auto it = R::rbegin(); it != R::rend(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // perform(fn) — call fn() once per step; fn must be an lvalue
  micron::io::print("perform: ");
  int counter = 0;
  auto print_counter = [&counter]() { micron::io::print(counter++, " "); };
  R::perform(print_counter);
  micron::io::println("");

  // ================================================================
  // count_range<T, From, To> — typed range for any arithmetic type
  // ================================================================

  micron::io::println("--- int_range<-3, 4> ---");

  using IR = micron::int_range<-3, 4>;
  micron::io::print("int_range: ");
  for ( auto it = IR::begin(); it != IR::end(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // Float range — template args must be float literals
  micron::io::println("--- float_range<0.f, 5.f> ---");
  using FR = micron::float_range<0.f, 5.f>;
  micron::io::print("float_range: ");
  for ( auto it = FR::begin(); it != FR::end(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // u64 range
  using U64R = micron::u64_range<10, 15>;
  micron::io::print("u64_range<10,15>: ");
  for ( auto it = U64R::begin(); it != U64R::end(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // ================================================================
  // counting_iter — the iterator type underlying all ranges
  // Supports random access, subscript, arithmetic
  // ================================================================

  micron::io::println("--- counting_iter ---");

  micron::counting_iter<int> it(10);
  micron::io::println("*it=", *it, " it[3]=", it[3]);   // 10, 13
  ++it;
  micron::io::println("after ++: *it=", *it);   // 11

  auto it2 = it + 5;
  micron::io::println("it+5: *it2=", *it2);   // 16

  micron::io::println("it2 - it=", it2 - it);   // 5

  // Comparison
  micron::io::println("it < it2: ", it < it2);
  micron::io::println("it == it2: ", it == it2);

  // ================================================================
  // range_of<T, N> — adaptor that views first N elements of a container
  // ================================================================

  // range_of<C, N> — C is the CONTAINER type, N is the element count
  // T in range_of must have T::iterator and T::size_type (i.e. be a container)
  micron::io::println("--- range_of<vector<int>, 3> ---");

  micron::vector<int> vec({10, 20, 30, 40, 50});

  // perform(container, fn) calls fn on each of the first N elements
  micron::io::print("perform first 3: ");
  micron::range_of<micron::vector<int>, 3>::perform(
      vec, [](int x) { micron::io::print(x, " "); });
  micron::io::println("");

  // view<C> is a non-owning window over the first N elements
  micron::range_of<micron::vector<int>, 3>::view<micron::vector<int>> w(vec);
  micron::io::print("view begin..end: ");
  for ( auto it = w.begin(); it != w.end(); ++it )
    micron::io::print(*it, " ");
  micron::io::println("");

  // ================================================================
  // ranges:: namespace — ADL-compatible free functions
  // Work on any container that has begin/end/size/data
  // ================================================================

  micron::io::println("--- ranges:: free functions ---");

  micron::vector<int> v({5, 4, 3, 2, 1});

  // ranges::size — generic size query
  auto sz = micron::ranges::size(v);
  micron::io::println("ranges::size(v)=", sz);

  // ranges::empty
  micron::io::println("ranges::empty(v)=", micron::ranges::empty(v));

  micron::vector<int> empty_v;
  micron::io::println("ranges::empty(empty_v)=", micron::ranges::empty(empty_v));

  // ranges::data — raw pointer to first element
  int *dptr = micron::ranges::data(v);
  micron::io::println("ranges::data[0]=", dptr[0]);

  // ranges::begin / end — generic iterators
  auto rb = micron::ranges::begin(v);
  micron::io::println("ranges::begin *rb=", *rb);

  // Works on raw arrays too
  int arr[5] = {100, 200, 300, 400, 500};
  auto ab = micron::ranges::begin(arr);
  micron::io::println("ranges on array: *begin=", *ab, " size=", micron::ranges::size(arr));

  // ================================================================
  // perform — call a method on an object once per step
  // ================================================================

  micron::io::println("--- perform with member function ---");

  struct Logger {
    int count = 0;
    void tick() { micron::io::print("[", count++, "] "); }
  } logger;

  micron::range<0, 4>::perform(logger, &Logger::tick);
  micron::io::println("");

  return 0;
}
