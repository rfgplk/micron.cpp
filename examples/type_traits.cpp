// type_traits.cpp
// micron's type_traits — a libc-free reimplementation of <type_traits>.
// The API mirrors C++23 std:: but lives in micron:: and avoids all
// STL / libc headers.
//
// See also:
//   examples/concepts.cpp — concepts built from these traits
//   examples/types.cpp    — the underlying type aliases (i32, f64, ...)

#include "../src/type_traits.hpp"
#include "../src/io/console.hpp"

// Helper: print a bool with a label
static void
check(const char *label, bool v)
{
  micron::io::println(label, v ? " -> true" : " -> false");
}

int
main()
{
  // --- is_same / is_same_v ---
  check("is_same<int, int>",   micron::is_same_v<int, int>);
  check("is_same<int, long>",  micron::is_same_v<int, long>);

  // --- remove_cv ---
  // Strips const and volatile qualifiers.
  static_assert(micron::is_same_v<micron::remove_cv_t<const volatile int>, int>);

  // --- remove_reference ---
  static_assert(micron::is_same_v<micron::remove_reference_t<int &&>, int>);

  // --- is_integral / is_floating_point ---
  check("is_integral<i32>",        micron::is_integral_v<i32>);
  check("is_integral<f64>",        micron::is_integral_v<f64>);
  check("is_floating_point<f64>",  micron::is_floating_point_v<f64>);

  // --- is_pointer ---
  check("is_pointer<int*>",   micron::is_pointer_v<int *>);
  check("is_pointer<int>",    micron::is_pointer_v<int>);

  // --- is_const / is_volatile ---
  check("is_const<const int>",     micron::is_const_v<const int>);
  check("is_volatile<volatile int>", micron::is_volatile_v<volatile int>);

  // --- is_trivially_copyable ---
  // Knowing this lets you use memcpy safely.
  struct Pod { int x; float y; };
  struct NonTrivial { NonTrivial(const NonTrivial &) {} int x; };
  check("is_trivially_copyable<Pod>",        micron::is_trivially_copyable_v<Pod>);
  check("is_trivially_copyable<NonTrivial>", micron::is_trivially_copyable_v<NonTrivial>);

  // --- conditional_t ---
  // Selects a type at compile time.
  using SmallOrBig = micron::conditional_t<sizeof(void*) == 8, i64, i32>;
  check("conditional_t selects i64 on 64-bit", micron::is_same_v<SmallOrBig, i64>);

  // --- enable_if_t ---
  // Classic SFINAE gate (prefer concepts in new code).
  // enable_if_t<Condition, T> is well-formed only when Condition is true.
  static_assert(micron::is_same_v<micron::enable_if_t<true, int>, int>);

  // --- decay ---
  // Models by-value argument decay (array -> pointer, ref -> value).
  static_assert(micron::is_same_v<micron::decay_t<int[5]>, int*>);
  static_assert(micron::is_same_v<micron::decay_t<int &>,  int>);

  // --- conjunction / disjunction / negation ---
  // Logical composition of traits.
  constexpr bool both_int = micron::conjunction_v<
      micron::is_integral<int>, micron::is_integral<long>>;
  check("conjunction<is_integral<int>, is_integral<long>>", both_int);

  constexpr bool either = micron::disjunction_v<
      micron::is_integral<float>, micron::is_floating_point<float>>;
  check("disjunction<is_integral<float>, is_fp<float>>", either);

  // --- integral_constant ---
  // The building block behind every trait.
  using Four = micron::integral_constant<int, 4>;
  static_assert(Four::value == 4);
  micron::io::println("integral_constant<int,4>::value = ", Four::value);

  return 0;
}
