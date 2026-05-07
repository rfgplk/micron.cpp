// array.cpp
// micron::array<T, N> — a fixed-size, stack-allocated array.
//
// See also:
//   examples/array_members.cpp — full member-by-member tour
//   examples/io.cpp            — how println() prints any container
//   examples/slice_span.cpp    — non-owning views over the same memory
//
// Unlike std::array:
//   - 64-byte aligned storage by default (SIMD-friendly).
//   - Generator-lambda constructor: array<T,N>([]{ return ...; }).
//   - Element-wise arithmetic operators (+= -= *= /= %=) — SIMD-aware.
//   - farray<T,N>: fundamental-type-only variant with trivial dtor.
//   - carray<T,N>: SIMD-optimised, compile-time loop unrolling.
//   - conarray / constarray: constexpr / immutable variants.
//   - static_size: compile-time capacity constant.

#include "../src/array/array.hpp"
#include "../src/io/console.hpp"

int
main()
{
  // --- Default construction (zero-initialises fundamental types) ---
  micron::array<int, 8> a;
  micron::io::println("default a = ", a);

  // --- Broadcast: fill every element with one value ---
  micron::array<int, 8> b(42);
  micron::io::println("broadcast b = ", b);

  // --- Generator: Fn() -> T called once per slot ---
  int counter = 0;
  micron::array<int, 8> c([&counter]() { return counter++; });
  micron::io::println("generated c = ", c);

  // --- Initializer list ---
  micron::array<int, 4> d({10, 20, 30, 40});
  micron::io::println("init-list d = ", d);

  // --- Copy / move ---
  micron::array<int, 4> e = d;                  // copy
  micron::array<int, 4> f = micron::move(d);    // move
  micron::io::println("copy e = ", e, "  move f = ", f);

  // --- Element access ---
  // operator[] — unchecked
  // at()       — bounds-checked; throws micron::runtime on overflow
  micron::array<int, 4> g({1, 2, 3, 4});
  micron::io::println("g[1]=", g[1], " g.at(2)=", g.at(2));

  // --- Iterators (still useful when you want a fold) ---
  int sum = 0;
  for ( auto it = g.cbegin(); it != g.cend(); ++it ) sum += *it;
  micron::io::println("manual sum = ", sum);

  // --- Element-wise arithmetic operators (SIMD-dispatched) ---
  micron::array<int, 4> x({1, 2, 3, 4});
  x += micron::array<int, 4>(10);     // add 10 to every element
  micron::io::println("after += 10: ", x);
  x *= micron::array<int, 4>(2);
  micron::io::println("after *= 2 : ", x);

  // --- size / static_size ---
  micron::io::println("size=", g.size(), " static_size=", decltype(g)::static_size);

  // --- farray: trivial-destructor variant for fundamental types ---
  micron::farray<f64, 4> fa(3.14);
  micron::io::println("farray fa = ", fa);

  // --- carray: SIMD-optimised, compile-time unrolled ---
  micron::carray<float, 8> ca(1.0f);
  ca += micron::carray<float, 8>(0.5f);
  micron::io::println("carray ca = ", ca);

  // --- conarray: constexpr, immutable ---
  // (point readers at the header — no runtime demo needed.)
  // see src/array/conarray.hpp / constarray.hpp / iarray.hpp

  return 0;
}
