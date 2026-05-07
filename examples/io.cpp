// io.cpp
// Tour of micron's printing / output facilities (src/io/).
//
// micron has no <iostream>. All output goes through a single template
// family in src/io/stdout.hpp that resolves the formatter at compile time
// via concepts. The headline feature: a container, pair, or map can be
// passed straight to println — there is no `<<` chain, no manipulators,
// no <format> machinery.
//
// What this example covers:
//   - print / println / printn          (stdout, with and without newline)
//   - error / errorln                   (stderr counterparts)
//   - native printing of arrays, vectors, maps, pairs, tuples
//   - multi-arg fold expansion: println(a, " ", b, " ", c)
//   - byte buffers and hex dumps (io::bin)
//   - ANSI styling via console.hpp (set_color, set_color_reset)
//
// Key STL deltas:
//   - println takes a parameter pack. With more than one arg it joins
//     them then appends a newline; with exactly one, it just prints+nl.
//     If you want spaces between args, pass them yourself: println(a, " ", b).
//   - println(vec) prints "{ 1, 2, 3 }" because the container overload
//     fires for any type that exposes cbegin/cend/begin/end and lacks c_str.
//   - Maps with a category_type == map_tag print as "{ k: v, ... }".
//   - There is no operator<< — overload resolution does all the work.

#include "../src/array/array.hpp"
#include "../src/io/console.hpp"
#include "../src/map.hpp"
#include "../src/tuple.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  // ================================================================
  // 1. Scalars and strings
  // ================================================================
  micron::io::println("-- 1. scalars --");

  // println takes any number of arguments. Each is formatted by the
  // best-matching `printk` overload (arithmetic, char-array, pointer,
  // c_str class, container, map, pair).
  micron::io::println("int=", 42, " float=", 3.14f, " bool=", true);

  // print() does NOT append a newline. Useful for building up a line.
  micron::io::print("no newline here -> ");
  micron::io::print("...still on same line\n");

  // printn() (note the trailing 'n') adds a newline AFTER EACH argument
  // rather than only at the end. Handy for column-style output.
  micron::io::printn("alpha", "beta", "gamma");

  // ================================================================
  // 2. Native container printing
  // ----------------------------------------------------------------
  // Any type that models is_container (cbegin/cend, no c_str) prints
  // as "{ a, b, c }". Works recursively — a vector<vector<int>> will
  // print nested braces.
  // ================================================================
  micron::io::println("-- 2. containers --");

  micron::array<int, 5> arr({10, 20, 30, 40, 50});
  micron::io::println("array  = ", arr);

  micron::vector<int> vec({1, 2, 3, 4, 5});
  micron::io::println("vector = ", vec);

  // Nested
  micron::vector<micron::vector<int>> mat;
  mat.emplace_back(micron::vector<int>({1, 2, 3}));
  mat.emplace_back(micron::vector<int>({4, 5, 6}));
  micron::io::println("matrix = ", mat);

  // Vectors of floats also work — Ryu is used for float -> string.
  micron::vector<f64> fv({1.5, 2.25, 3.125});
  micron::io::println("floats = ", fv);

  // ================================================================
  // 3. Pairs and tuples
  // ================================================================
  micron::io::println("-- 3. pairs --");

  // micron::pair<A,B> has fields .a / .b (not .first / .second)
  // and prints as [a, b].
  micron::pair<int, f64> p(7, 7.5);
  micron::io::println("pair = ", p);

  // pair-of-vector — composition just works
  micron::pair<micron::vector<int>, micron::vector<int>> halves;
  halves.a = micron::vector<int>({1, 2, 3});
  halves.b = micron::vector<int>({4, 5, 6});
  micron::io::println("split = ", halves);

  // ================================================================
  // 4. Maps print as { key: value, ... }
  // ================================================================
  micron::io::println("-- 4. maps --");

  micron::robin_map<int, int> rm;
  rm.insert(1, 100);
  rm.insert(2, 200);
  rm.insert(3, 300);
  micron::io::println("robin_map = ", rm);
  // Note: each insert performs key hashing internally; the printed
  // entries show key:value because robin_map keeps the original key.
  // (hopscotch_map stores hashes; iterating it directly is not
  // recommended — see examples/maps.cpp for the find/contains API.)

  // ================================================================
  // 5. stderr stream
  // ----------------------------------------------------------------
  // error / errorln / errorn / printn all mirror their stdout twins,
  // they just route to fd 2.
  // ================================================================
  micron::io::println("-- 5. stderr --");
  micron::io::errorln("(this line goes to stderr, but is still visible)");

  // ================================================================
  // 6. ANSI colours (console.hpp)
  // ----------------------------------------------------------------
  // set_color(color, style) emits an ANSI escape; set_color_reset()
  // restores. Only meaningful on a terminal — redirected output gets
  // raw escapes, so wrap with isatty() in real code.
  // ================================================================
  micron::io::println("-- 6. ansi --");
  micron::set_color(micron::color::green, micron::style::bold);
  micron::io::print("green bold");
  micron::set_color_reset();
  micron::io::print(" -> ");
  micron::set_color(micron::color::red, micron::style::italic);
  micron::io::print("red italic");
  micron::set_color_reset();
  micron::io::println("");

  // ================================================================
  // 7. Hex dump of a byte buffer
  // ----------------------------------------------------------------
  // io::bin(container) writes lower-case hex with no separators.
  // Useful for inspecting raw memory or wire data.
  // ================================================================
  micron::io::println("-- 7. hex dump --");
  micron::array<byte, 8> bytes({0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE});
  micron::io::print("hex = ");
  micron::io::bin(bytes);
  micron::io::println("");

  // ================================================================
  // 8. console() — quiet alias for println
  // ----------------------------------------------------------------
  // micron::console(...) is just println without the io:: prefix —
  // useful when you've already pulled in console.hpp.
  // ================================================================
  micron::io::println("-- 8. console alias --");
  micron::console("via console(): ", vec);

  return 0;
}
