// vector.cpp
// micron::vector<T> — a heap-allocated, growable sequence.
//
// See also:
//   examples/vector_members.cpp — full member-by-member tour
//   examples/svector.cpp        — stack-allocated bounded variant
//   examples/io.cpp             — println(vector) container printing
//
// Unlike std::vector:
//   - Default capacity starts at 8; doubles on each reallocation.
//   - allocator_serial is a static class — no per-instance allocator.
//   - operator+= is push_back (syntactic sugar).
//   - append() copies from another vector; weld() moves (zero-copy merge).
//   - max_size() returns allocated capacity (there is no capacity()).
//   - fvector<T>: non-copyable variant for fundamental types.
//   - svector<T, N>: stack-allocated, fixed upper capacity N.
//   - Bounds checks are on by default (Sf=true); .at() throws micron::oor.

#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

struct Point {
  int x, y;
  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}
};

int
main()
{
  // --- Default construction ---
  micron::vector<int> v;
  micron::io::println("empty: size=", v.size(), " max_size=", v.max_size());

  // --- push_back / operator+= ---
  v.push_back(1);
  v.push_back(2);
  v += 3;     // operator+= is push_back
  micron::io::println("after 3 pushes: ", v);

  // --- Construction with size ---
  micron::vector<int> sized(8, 99);     // 8 elements, each = 99
  micron::io::println("sized = ", sized);

  // --- Generator constructor: (size, Fn() -> T) ---
  int n = 0;
  micron::vector<int> gen(6, [&n]() { return n++; });
  micron::io::println("gen   = ", gen);

  // --- Initializer list ---
  micron::vector<int> ilist({10, 20, 30, 40});
  micron::io::println("ilist = ", ilist);

  // --- Move semantics ---
  micron::vector<int> src({1, 2, 3});
  micron::vector<int> dst = micron::move(src);
  micron::io::println("after move: src.empty=", src.empty(), " dst=", dst);

  // --- Access: operator[] vs at() ---
  // operator[] — unchecked
  // at()       — bounds-checked, throws micron::oor
  micron::vector<int> w({5, 6, 7, 8});
  micron::io::println("w=", w, "  w[2]=", w[2], "  w.at(3)=", w.at(3));

  // --- insert / erase ---
  micron::vector<int> ie({1, 2, 4, 5});
  ie.insert(2, 3);     // insert 3 at index 2
  micron::io::println("after insert(2, 3): ", ie);

  // erase(usize) — cast to disambiguate from the iterator overload
  ie.erase(usize(0));
  micron::io::println("after erase(0):     ", ie);

  // --- pop_back ---
  micron::vector<int> pb({1, 2, 3});
  pb.pop_back();
  micron::io::println("after pop_back: ", pb);

  // --- reserve / resize ---
  micron::vector<int> rv;
  rv.reserve(64);
  micron::io::println("after reserve(64): max_size=", rv.max_size());
  rv.resize(8, 0);
  micron::io::println("after resize(8, 0): ", rv);

  // --- append (copy) vs weld (move) ---
  // append: src untouched. weld: src is consumed (zero-copy merge).
  micron::vector<int> a({1, 2, 3});
  micron::vector<int> b({4, 5, 6});
  a.append(b);
  micron::io::println("after append: a=", a, "  b=", b);

  micron::vector<int> c({7, 8, 9});
  a.weld(micron::move(c));
  micron::io::println("after weld:   a=", a, "  c.empty=", c.empty());

  // --- emplace_back: in-place construct ---
  micron::vector<Point> pts;
  pts.emplace_back(3, 4);
  pts.emplace_back(5, 6);
  micron::io::println("pts[0]=(", pts[0].x, ",", pts[0].y,
                      ")  pts[1]=(", pts[1].x, ",", pts[1].y, ")");

  // --- find ---
  // Returns an iterator to the first matching element; end() if not found.
  micron::vector<int> f({1, 2, 3, 2, 1});
  auto it = f.find(2);
  micron::io::println("find(2) offset = ", it - f.begin());

  // --- Element-wise printing of a nested vector
  micron::vector<micron::vector<int>> nested;
  nested.emplace_back(micron::vector<int>({1, 2, 3}));
  nested.emplace_back(micron::vector<int>({4, 5, 6}));
  micron::io::println("nested = ", nested);

  return 0;
}
