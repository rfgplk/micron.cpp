// concepts.cpp
// micron's C++20 concepts (src/concepts.hpp).
// Concepts replace SFINAE for cleaner constraint expression.
// Most container / algorithm APIs in micron are constrained with these.
//
// See also:
//   examples/type_traits.cpp — the type_traits used to build concepts
//   examples/types.cpp       — fundamental types these concepts reason about

#include "../src/concepts.hpp"
#include "../src/io/console.hpp"

// --- Custom function constrained by micron concepts ---

// fn_codomain: Fn(T) -> T  (endomorphism — same input/output type)
template <typename Fn, typename T>
  requires micron::fn_codomain<Fn, T>
T
apply(Fn fn, T val)
{
  return fn(val);
}

// fn_binary_predicate: Fn(T, T) -> bool
template <typename Fn, typename T>
  requires micron::fn_binary_predicate<Fn, T>
bool
compare(Fn fn, T a, T b)
{
  return fn(a, b);
}

// integral concept — restricts to integer types only
template <micron::integral T>
T
double_it(T v)
{
  return v * 2;
}

// totally_ordered — type must support all comparison operators
template <micron::totally_ordered T>
T
clamp_example(T v, T lo, T hi)
{
  if ( v < lo ) return lo;
  if ( v > hi ) return hi;
  return v;
}

// --- Container concept example ---
// is_iterable_container: has begin/end/data/size
template <micron::is_iterable_container C>
typename C::size_type
count_positive(const C &c)
{
  typename C::size_type n = 0;
  for ( auto it = c.cbegin(); it != c.cend(); ++it )
    if ( *it > 0 ) ++n;
  return n;
}

int
main()
{
  // fn_codomain: lambda int->int satisfies the constraint
  auto doubled = apply([](int x) { return x * 2; }, 21);
  micron::io::println("apply double 21 -> ", doubled);

  // fn_binary_predicate
  bool gt = compare([](int a, int b) { return a > b; }, 10, 5);
  micron::io::println("10 > 5 -> ", gt);

  // integral concept
  micron::io::println("double_it(7) = ", double_it(7));
  // double_it(3.14);  // would not compile — float does not satisfy integral

  // totally_ordered
  micron::io::println("clamp(15, 0, 10) = ", clamp_example(15, 0, 10));

  // Concept checks at compile time with static_assert
  static_assert(micron::integral<int>);
  static_assert(micron::integral<u64>);
  static_assert(!micron::integral<f32>);

  static_assert(micron::floating_point<f32>);
  static_assert(micron::floating_point<f64>);

  static_assert(micron::same_as<int, int>);
  static_assert(micron::distinct<int, long>);

  static_assert(micron::convertible_to<int, long>);

  // movable / copyable / default_initializable
  // movable requires move-constructible AND move-assignable
  struct Movable {
    Movable() = default;
    Movable(Movable &&) = default;
    Movable &operator=(Movable &&) = default;
  };
  static_assert(micron::movable<Movable>);
  static_assert(micron::default_initializable<int>);

  // equality_comparable
  static_assert(micron::equality_comparable<int>);
  static_assert(micron::totally_ordered<int>);

  micron::io::println("all concept checks passed");
  return 0;
}
