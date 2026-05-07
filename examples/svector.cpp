// svector.cpp
// micron::svector<T, N> — a stack-allocated vector.
//
// See also:
//   examples/vector.cpp        — heap-allocated workhorse
//   examples/vector_members.cpp — full vector API tour
//   examples/slice_span.cpp    — span<T,N> is similar in spirit
//
// svector stores up to N elements entirely on the stack; no heap.
// It is otherwise API-compatible with vector<T>. push_back beyond N
// throws micron::length (unlike heap vector which reallocates).
//
// Use svector for hot paths where heap allocation latency is
// unacceptable, or for small short-lived collections in embedded /
// kernel contexts.

#include "../src/except.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/svector.hpp"

int
main()
{
  // --- Declare with explicit capacity ---
  micron::svector<int, 8> sv;
  micron::io::println("empty svector: size=", sv.size(), " max_size=", sv.max_size());

  // --- push_back (no heap allocation) ---
  for ( int i = 0; i < 8; ++i ) sv.push_back(i * 10);
  micron::io::println("after 8 pushes: ", sv);

  // --- Overflow throws micron::length ---
  try {
    sv.push_back(999);     // capacity 8 — this is the 9th element
  } catch ( const micron::length &ex ) {
    micron::io::println("overflow caught: ", ex.what());
  }

  // --- pop_back / clear ---
  sv.pop_back();
  micron::io::println("after pop_back: ", sv);

  sv.clear();
  micron::io::println("after clear: empty=", sv.empty());

  // --- Initializer list construction ---
  micron::svector<int, 16> sv2({1, 2, 3, 4, 5});
  micron::io::println("init-list sv2 = ", sv2);

  // --- Generator construction ---
  int n = 0;
  micron::svector<int, 8> sv3(8, [&n]() { return n++; });
  micron::io::println("generated sv3 = ", sv3);

  return 0;
}
