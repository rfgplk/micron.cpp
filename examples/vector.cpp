#include "../src/vector/vector.hpp"
#include "../src/io/console.hpp"

struct Point {
  int x, y;

  Point() : x(0), y(0) { }

  Point(int x, int y) : x(x), y(y) { }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vector.cpp
// micron::vector<T>
// heap-allocated, dynamically growable container
//
// Unlike std::vector:
//   - bounds checks are enabled by default (via Sf=true)
//   - vector comes in two forms, one for copyable objects and the other for movable only objects
//   - default capacity is set to 4096 bytes (one page granularity); __triples__ on each reallocation (this is allocator spec dependent)
//   - append() copies from another vector; weld() moves (zero-copy merge)
//   - max_size() returns allocated capacity (there is no capacity())
//   - ... (way more fill out later)
//
// Other vector variants:
//   - convector<T>: concurrency safe vector (internal lock)
//   - circle_vector<T>: stack allocated circular buffer
//   - fvector<T>: non-copyable variant for fundamental types only
//   - svector<T, N>: stack-allocated, fixed capacity N vector (inplace_vector like)
//   - ivector<T, N>: CoW contiguous memory immutable vector
//   - pvector<T, N>: trie backed persistent vector with pathcopying

// See also:
//   examples/vector_members.cpp — full member-by-member tour
//   examples/svector.cpp        — stack-allocated bounded variant
//   examples/io.cpp             — println(vector) container printing

int
main()
{
  // default construction
  micron::vector<int> v;
  micron::io::println("empty: size=", v.size(), " max_size=", v.max_size());

  // common mutating operations (same as stl, mostly)
  v.push_back(1);
  v.push_back(2);
  v += 3;      // operator+= is push_back
  micron::io::println("after 3 pushes: ", v);
  v.move_back(3);
  v.move_back(4);      // explicit rvalue ref move
  micron::io::println("after 2 more pushes: ", v);
  v.emplace_back(5);
  v.emplace_back(6);      // emplace_backs
  micron::io::println("after 2 more emplaces: ", v);

  // construction via size
  micron::vector<int> sized(8, 99);      // 8 elements, each = 99
  micron::io::println("sized = ", sized);

  // construction via generator fn
  int n = 0;
  micron::vector<int> gen(6, [&n]() { return n++; });
  micron::io::println("gen   = ", gen);

  // construction via slice
  micron::slice<byte> slice(128);
  slice.fill(1);
  micron::vector<int> gen_slice(*slice);
  gen_slice.set_size(128 / sizeof(int));
  micron::io::println("gen_slice  = ", gen_slice);

  // via initializer lists
  micron::vector<int> ilist({ 10, 20, 30, 40 });
  micron::io::println("ilist = ", ilist);

  // move semantics
  micron::vector<int> src({ 1, 2, 3 });
  micron::vector<int> dst = micron::move(src);
  micron::io::println("after move: src.empty=", src.empty(), " dst=", dst);

  // both operator[] AND at() are bounds checked
  micron::vector<int> w({ 5, 6, 7, 8 });
  micron::io::println("w=", w, "  w[2]=", w[2], "  w.at(3)=", w.at(3));

  // inserts / erases
  micron::vector<int> ie({ 1, 2, 4, 5 });
  ie.insert(2, 3);      // insert 3 at index 2
  micron::io::println("after insert(2, 3): ", ie);
  ie.erase(usize(0));
  micron::io::println("after erase(0):     ", ie);

  // pop_back
  micron::vector<int> pb({ 1, 2, 3 });
  pb.pop_back();
  micron::io::println("after pop_back: ", pb);

  // reserve / resize
  micron::vector<int> rv;
  rv.reserve(64);
  micron::io::println("after reserve(64): max_size=", rv.max_size());
  rv.resize(8, 0);
  micron::io::println("after resize(8, 0): ", rv);

  // clear / fast_clear
  micron::vector<int, micron::allocator_serial<>, false> rc(10, 10);
  micron::io::println("before clear(): rc[5]", rc[5]);
  rc.clear();      // clear hard zeroes a vector out
  micron::io::println("after clear(): rc[5]", rc[5]);
  rc.resize(10, 10);
  rc.fast_clear();      // fast clear DOESN'T zero out memory iff type is fundamental
  micron::io::println("after fast_clear(): rc[5]", rc[5]);

  // append (copy) vs weld (move)
  // append: src untouched
  micron::vector<int> a({ 1, 2, 3 });
  micron::vector<int> b({ 4, 5, 6 });
  a.append(b);
  micron::io::println("after append: a=", a, "  b=", b);

  // weld: src is consumed (zero-copy merge)
  micron::vector<int> c({ 7, 8, 9 });
  a.weld(micron::move(c));
  micron::io::println("after weld:   a=", a, "  c.empty=", c.empty());

  // supports easily being converted to slices
  micron::vector<int> d(100, 100);
  micron::slice<int> d_slice = d[];
  d.fill(2);
  micron::io::println("after slicing: d_slice=", d_slice);
  micron::slice<int> d_slice_small = d[10, 20];
  micron::io::println("after slicing: d_slice_small=", d_slice_small);

  // emplace_back: in-place construct
  micron::vector<Point> pts;
  pts.emplace_back(3, 4);
  pts.emplace_back(5, 6);
  micron::io::println("pts[0]=(", pts[0].x, ",", pts[0].y, ")  pts[1]=(", pts[1].x, ",", pts[1].y, ")");

  // find
  // Returns an iterator to the first matching element; end() if not found.
  micron::vector<int> f({ 1, 2, 3, 2, 1 });
  auto it = f.find(2);
  micron::io::println("find(2) offset = ", it - f.begin());

  // element-wise printing of a nested vector
  micron::vector<micron::vector<int>> nested;
  nested.emplace_back(micron::vector<int>({ 1, 2, 3 }));
  nested.emplace_back(micron::vector<int>({ 4, 5, 6 }));
  micron::io::println("nested = ", nested);

  return 0;
}
