#include "../src/algorithm/fp.hpp"
#include "../src/io/console.hpp"
#include "../src/list.hpp"
#include "../src/lz.hpp"
#include "../src/maps/swiss.hpp"
#include "../src/vector/vector.hpp"

// algorithm_lz.cpp
// micron::lz -- the LAZY counterpart to micron::fp
//
// See also:
//   examples/algorithm_fp.cpp        -- the eager layer these mirror
//   benches/results/lazy_baseline.txt -- lz against a hand-written loop and against fp
//
// fp:: is eager. Every combinator takes a whole container and returns a whole container, so
//     v | fp::filter_c(p) | fp::take_c(3) | fp::fmap_c(f)
// allocates three times and runs the filter over all n elements before discarding all but three.
//
// lz:: is the same vocabulary as pull-based views. Nothing runs until a terminal asks for an
// element, so the chain fuses into ONE pass with ONE allocation at the end -- or none at all when
// the terminal is a scalar.
//
// Two rules worth knowing before you start:
//   * `lz::` is never re-exported into `micron::`. micron::fmap, ::filter, ::take, ::count, ::merge
//     and ::reverse all already exist and are eager; `lz::` at the call site is the mode marker.
//   * a chain does nothing until a TERMINAL runs. `auto p = v | lz::fmap(f);` has not called f once.

namespace lz = micron::lz;
using vec_i = micron::vector<int>;

static void
print_vec(const char *label, const vec_i &v)
{
  micron::io::print(label);
  for ( usize i = 0; i < v.size(); ++i ) {
    micron::io::print(v[i]);
    if ( i + 1 < v.size() ) micron::io::print(", ");
  }
  micron::io::println("");
}

int
main()
{
  vec_i v;
  for ( int i = 1; i <= 12; ++i ) v.push_back(i);

  // ================================================================
  // ONE PASS, ONE ALLOCATION
  // ================================================================
  // The eager spelling below builds a filtered vector, then a mapped vector, then a truncated
  // vector -- three allocations, and it maps every surviving element before throwing most away.
  auto lazy = v | lz::filter([](int x) { return (x & 1) == 1; }) | lz::fmap([](int x) { return x * 10; })
              | lz::take(3) | lz::collect<vec_i>();
  print_vec("filter|fmap|take(3)  : ", lazy);

  auto eager = micron::fp::take(micron::fp::fmap([](int x) { return x * 10; },
                                                 micron::filter(v, [](const int *x) { return (*x & 1) == 1; })),
                                3);
  print_vec("  the eager spelling : ", eager);

  // ================================================================
  // NO ALLOCATION AT ALL -- a scalar terminal never materialises anything
  // ================================================================
  micron::io::println("sum                  : ", static_cast<u64>(v | lz::sum()));
  micron::io::println("count_if(x > 8)      : ", v | lz::count_if([](int x) { return x > 8; }));
  micron::io::println("fmap|fold            : ", v | lz::fmap([](int x) { return x * 2; })
                                                     | lz::fold(0, [](int a, int x) { return a + x; }));
  micron::io::println("max                  : ", v | lz::max());

  // ================================================================
  // ENDLESS SOURCES
  // ================================================================
  // A generator has no end. That is safe here precisely because nothing runs until a terminal
  // pulls -- and collect refuses an unbounded chain at COMPILE time, so `counting(1) | collect<>`
  // is an error rather than a hang. Bound it with take or take_while.
  auto squares = lz::counting(1) | lz::fmap([](int x) { return x * x; }) | lz::take(6) | lz::collect<vec_i>();
  print_vec("first 6 squares      : ", squares);

  auto powers = lz::iterate([](int x) { return x * 3; }, 1) | lz::take_while([](int x) { return x < 200; })
                | lz::collect<vec_i>();
  print_vec("powers of 3 < 200    : ", powers);

  // ================================================================
  // SHORT-CIRCUITING -- the source is touched only as far as the answer needs
  // ================================================================
  usize probes = 0;
  const bool found = lz::counting(0) | lz::fmap([&](int x) {
                       ++probes;
                       return x;
                     })
                     | lz::any_of([](int x) { return x > 10; });
  micron::io::println("any_of(x > 10)       : ", found, "  after ", probes, " elements (0..11, and no more)");

  // ================================================================
  // ERRORS LIVE ON TERMINALS, NEVER ON ADAPTORS
  // ================================================================
  // A view cannot carry an error branch, so anything fallible is a terminal returning
  // option<T, E>. Success is is_first(); the value comes out with cast<T>().
  auto head = v | lz::head();
  micron::io::println("head                 : ", head.is_first() ? head.cast<int>() : -1);
  vec_i empty;
  micron::io::println("head of empty        : ", (empty | lz::head()).is_second() ? "error branch" : "value");

  auto div = v | lz::safe_divide<vec_i>(0);
  micron::io::println("safe_divide by 0     : ", div.is_second() ? div.cast<micron::fp::division_by_zero_error>().what() : "ok");

  // ================================================================
  // WINDOWS ARE SUB-VIEWS, NOT COPIES
  // ================================================================
  // chunk yields ptr_views pointing into the source. Nothing is copied, so chunk|count allocates
  // nothing at all -- where the eager chunk_into builds one inner container per chunk.
  micron::io::print("chunk(5) sizes       : ");
  v | lz::chunk(5) | lz::for_each([](auto w) { micron::io::print(w.size(), " "); });
  micron::io::println("");
  micron::io::println("flatten . chunk == id: ", (v | lz::chunk(5) | lz::flatten() | lz::count()) == v.size());

  // ================================================================
  // REVERSE STREAMS OVER A CONTIGUOUS SOURCE
  // ================================================================
  // and only buffers when the source cannot be walked backwards. The property is in the TYPE:
  static_assert(!decltype(micron::declval<vec_i &>() | lz::reverse())::__is_materializing);
  static_assert(decltype(micron::declval<micron::list<int> &>() | lz::reverse())::__is_materializing);
  print_vec("reverse|take(4)      : ", v | lz::reverse() | lz::take(4) | lz::collect<vec_i>());

  // ================================================================
  // ANY CONTAINER IS A SOURCE, INCLUDING THE NODE ONES
  // ================================================================
  micron::list<int> l;
  for ( int i = 1; i <= 6; ++i ) l.push_back(i * 11);
  micron::io::println("list, sum of evens   : ",
                      static_cast<u64>(l | lz::filter([](int x) { return (x & 1) == 0; }) | lz::sum()));

  // a map's cursor hands back a key/value pair; keys() and values() are views over it, not copies
  micron::stack_swiss_map<int, int, 32> m;
  for ( int i = 1; i <= 5; ++i ) m.insert(i, i * 100);
  micron::io::println("map, sum of values   : ", static_cast<u64>(m | lz::values() | lz::sum()));
  micron::io::println("map, has key 3       : ", m | lz::keys() | lz::elem(3));

  // ================================================================
  // THE ORDER OF A CHAIN IS A PERFORMANCE DECISION
  // ================================================================
  // filter re-reads the element it accepted -- once to test it, once to hand it on. Over a vector
  // that is free; over `fmap(f) | filter(p)` it calls f a second time for every survivor. Put the
  // filter FIRST when f is not trivial.
  micron::io::println("prefer filter|fmap over fmap|filter when the map is expensive");

  return 0;
}
