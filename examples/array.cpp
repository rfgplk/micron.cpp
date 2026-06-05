#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// array.cpp
// micron::array<T, N>
// fixed-size, stack-allocated arrays
//
// Unlike std::array:
//   - 64-byte aligned storage by default (SIMD-friendly).
//   - Generator-lambda constructor: array<T,N>([]{ return ...; }).
//   - Element-wise arithmetic operators (+= -= *= /= %=) — SIMD-aware.
//   - farray<T,N>: fundamental-type-only variant with trivial dtor.
//   - carray<T,N>: SIMD-optimised, compile-time loop unrolling.
//   - conarray / constarray: constexpr / immutable variants.
//   - static_size: compile-time capacity constant.
// See also:
//   examples/array_members.cpp: full member-by-member tour
//   examples/io.cpp           : how println() prints any container
//   examples/slice_span.cpp   : non-owning views over the same memory

int
main()
{
  // default construction zero initializes
  micron::array<int, 8> a;
  micron::io::println("default a = ", a);

  // can also construct by giving a value for the whole array
  micron::array<int, 8> b(42);
  micron::io::println("broadcast b = ", b);

  // constructible via generator fns
  int counter = 0;
  micron::array<int, 8> c([&counter]() { return counter++; });
  micron::io::println("generated c = ", c);

  // and initializer lists
  micron::array<int, 4> d({ 10, 20, 30, 40 });
  micron::io::println("init-list d = ", d);

  // copy && move semantics
  micron::array<int, 4> e = d;                    // copy
  micron::array<int, 4> f = micron::move(d);      // move
  micron::io::println("copy e = ", e, "  move f = ", f);

  // element accessing is normal; operator[] is unchecked, at() is checked for bounds/capacity
  micron::array<int, 4> g({ 1, 2, 3, 4 });
  micron::io::println("g[1]=", g[1], " g.at(2)=", g.at(2));

  // iterator support
  int sum = 0;
  for ( auto it = g.cbegin(); it != g.cend(); ++it ) sum += *it;
  micron::io::println("manual sum = ", sum);

  // SIMD dispatched element wise/array wise arithmetic
  micron::array<int, 4> x({ 1, 2, 3, 4 });
  x += micron::array<int, 4>(10);      // add 10 to every element
  micron::io::println("after += 10: ", x);
  // also valid and preferable
  x += 10;
  micron::io::println("after += 10: ", x);
  x *= micron::array<int, 4>(2);
  micron::io::println("after *= 2 : ", x);
  x *= 2;
  micron::io::println("after *= 2 : ", x);
  x.sum();
  micron::io::println(".sum() : ", x);
  // supports named arithmetic too
  x.mul(2);
  micron::io::println(".mul(2) : ", x);
  x.sub(10);
  micron::io::println(".sub(10) : ", x);
  x.div(1);
  micron::io::println(".div(1) : ", x);
  x.div(2);
  micron::io::println(".div(2) : ", x);
  x.fill(0);
  micron::io::println(".fill(0) : ", x);

  micron::array<u64, 16> sl_arr(10);
  micron::io::println("sl_arr: ", sl_arr);
  micron::slice<u64> slice = sl_arr[];
  micron::io::println("to a slice (heap-backed): ", slice);
  micron::io::println("to a slice (heap-backed) [addr]: ", &slice);
  micron::vector<u64> vec_s(*slice);
  vec_s.set_size(16);      // hint; slices have no length concept, we have to set the size of the vector manually so println iterators over
                           // the container
  micron::io::println("to a slice (heap-backed) [vector form]: ", vec_s);

  // size querying
  micron::io::println("size=", g.size(), " max_size=", g.max_size(), " static_size=", decltype(g)::static_size);
  // property querying
  micron::io::println("is_pod=", g.is_pod(), " is_class=", g.is_class(), " is_trivial=", g.is_trivial());

  // raw memory access
  micron::io::println("addr=", g.addr(), " data=", g.data(), " static_size=", decltype(g)::static_size);

  // farrays only support fundamental types
  micron::farray<f64, 4> fa(3.14);
  micron::io::println("farray fa = ", fa);

  // carrays are cache optimized; internally SIMD dispatched && compile-time unrolled
  micron::carray<float, 8> ca(1.0f);
  ca += 1.0f;
  micron::io::println("carray ca = ", ca);
  ca += 5.0f;
  micron::io::println("carray ca = ", ca);
  ca -= 2.0f;
  micron::io::println("carray ca = ", ca);
  ca.sqrt();
  micron::io::println("carray ca = ", ca);

  return 0;
}
