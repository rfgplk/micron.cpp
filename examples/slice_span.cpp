// slice_span.cpp
// Tour of micron's contiguous-memory views: src/slice.hpp, src/span.hpp.
//
// micron has THREE related types in this area, and the names are
// counter-intuitive if you come from std::span:
//
//   raw_slice<T>  — a fat pointer (T* + size_t). Non-owning, copyable,
//                   trivial. The everyday "view" you pass around.
//
//   slice<T>      — heap-allocated owning buffer. Construct with size,
//                   from a [a, b) range, or from a generator. Non-copyable
//                   (move-only). Roughly the std::vector of the slice family.
//
//   span<T, N>    — STACK-allocated, fixed-capacity owning buffer. No heap.
//                   Plenty of rust-style helpers: split, strip_prefix,
//                   trim_ascii, rotate_left, etc.
//
// Mental model:
//   "slice" in std::span land == raw_slice<T> here.
//   "span" in std::span land  has no direct match — micron::span<T,N>
//   is the "owning, on-stack, fixed-capacity" half of std::array+std::span.
//
// Why the inversion? micron prioritises ownership semantics by default —
// types that look like containers ARE containers. The non-owning view
// gets the explicit "raw_" prefix because it skips destruction.

#include "../src/io/console.hpp"
#include "../src/slice.hpp"
#include "../src/span.hpp"
#include "../src/vector/vector.hpp"

int
main()
{
  // ================================================================
  // 1. raw_slice<T> — the lightweight non-owning view
  // ----------------------------------------------------------------
  // Construct from a (ptr, len) pair. Has begin/end/cbegin/cend, so
  // every algorithm in src/algorithm/ accepts it directly.
  // ================================================================
  micron::io::println("-- 1. raw_slice --");

  int data[] = {10, 20, 30, 40, 50};
  micron::raw_slice<int> rs(data, 5);

  micron::io::println("size=", rs.size(), " is_empty=", rs.is_empty());
  micron::io::println("rs[2]=", rs[2]);

  // Iterate via begin/end — works in any range-for context.
  micron::io::print("contents = ");
  for ( auto *p = rs.begin(); p != rs.end(); ++p ) micron::io::print(*p, " ");
  micron::io::println("");

  // ================================================================
  // 2. slice<T> — heap-owning, sized buffer
  // ----------------------------------------------------------------
  // Construct in several ways:
  //   slice<T>(n)           — n default-init slots
  //   slice<T>(n, val)      — n copies of val
  //   slice<T>(begin, end)  — copy from [begin, end)
  //   slice<T>(fn, src)     — slice<T>(fn, src) maps fn over src
  //
  // Always move, never copy: copy ctor is deleted.
  // ================================================================
  micron::io::println("-- 2. slice --");

  // Fill of size 6, value 7
  micron::slice<int> s(6, 7);
  // QUIRK: slice<T>::end() points at the LAST element (not one past),
  // by design — iterating tests use `end() + 1`. So passing a slice
  // directly to println shows one fewer element. Print via .iter()
  // (a raw_slice) for the natural [0, size) view.
  micron::io::println("filled slice (via .iter()) = ", s.iter());

  // Copy from a raw range
  int src[] = {1, 2, 3, 4};
  micron::slice<int> from_range(src, src + 4);
  micron::io::println("from range  = ", from_range.iter());

  // Map another container element-wise via the (Fn, Container) ctor
  micron::vector<int> v({1, 2, 3, 4, 5});
  micron::slice<int> squared([](int x) { return x * x; }, v);
  micron::io::println("squared     = ", squared.iter());

  // Subscript with two args returns a sub-slice (heap-copy), exposing
  // [n, m) of the original. Note: this is operator[](size_t, size_t),
  // a C++23 multi-dim subscript, not a method call.
  micron::slice<int> mid = from_range[1, 3];
  micron::io::println("mid [1,3)   = ", mid.iter());

  // Element accessors. get(i) returns a pointer and bounds-checks
  // (returns nullptr on overflow). first()/last() return endpoints.
  micron::io::println("first = ", *from_range.first(), " last = ", *from_range.last());

  // ================================================================
  // 3. slice<T> mutating helpers
  // ================================================================
  micron::io::println("-- 3. slice ops --");

  micron::slice<int> ops(5, 0);
  // fill_with: pass a generator
  int counter = 0;
  ops.fill_with([&counter]() { return counter++; });
  micron::io::println("filled  = ", ops.iter());

  ops.reverse();
  micron::io::println("reversed= ", ops.iter());

  ops.rotate_left(2);
  micron::io::println("rotL(2) = ", ops.iter());

  // swap two indices
  ops.swap(0, 4);
  micron::io::println("swap0,4 = ", ops.iter());

  // ================================================================
  // 4. span<T, N> — stack-allocated, fixed capacity
  // ----------------------------------------------------------------
  // No heap allocation; capacity is a compile-time template arg N.
  // Construction options mirror slice. Push/insert beyond N throws.
  // ================================================================
  micron::io::println("-- 4. span --");

  micron::span<int, 16> sp({1, 2, 3, 4, 5});
  micron::io::println("span     = ", sp);
  micron::io::println("size=", sp.size(), " max=", sp.max_size(), " full=", sp.full());

  sp.push_back(6);
  sp.push_back(7);
  micron::io::println("after push_back x2 = ", sp);

  // ================================================================
  // 5. span: rust-style helpers
  // ----------------------------------------------------------------
  // span carries a wide assortment of split/strip/contains/starts_with
  // routines that std::span does not have.
  // ================================================================
  micron::io::println("-- 5. span helpers --");

  micron::span<int, 32> hay({1, 2, 3, 4, 5, 0, 6, 7, 0, 8, 9});

  // split() — invokes a callback for each segment delimited by pred.
  // Useful for tokenisation without allocating.
  int seg = 0;
  hay.split([](const int &x) { return x == 0; }, [&seg](micron::raw_slice<int> piece) {
    micron::io::print("  segment ", seg++, " (len ", piece.size(), "): ");
    for ( usize i = 0; i < piece.size(); ++i ) micron::io::print(piece[i], " ");
    micron::io::println("");
  });

  // contains / starts_with / ends_with
  micron::io::println("contains(7)=", hay.contains(7));
  int pre[] = {1, 2, 3};
  micron::raw_slice<int> pre_v(pre, 3);
  micron::io::println("starts_with{1,2,3}=", hay.starts_with(pre_v));

  // strip_prefix returns a raw_slice over the remainder (or empty if no match)
  auto remainder = hay.strip_prefix(pre_v);
  micron::io::print("after strip {1,2,3}: ");
  for ( usize i = 0; i < remainder.size(); ++i ) micron::io::print(remainder[i], " ");
  micron::io::println("");

  // ================================================================
  // 6. span: ASCII helpers
  // ----------------------------------------------------------------
  // For span<char, N> / span<byte, N> there's a small ASCII toolkit:
  // is_ascii, to_ascii_uppercase/lowercase, trim_ascii*.
  // ================================================================
  micron::io::println("-- 6. span<char> ascii --");

  micron::span<char, 32> text({' ', ' ', 'h', 'i', '!', ' '});
  micron::io::println("is_ascii=", text.is_ascii());

  text.to_ascii_uppercase();
  micron::io::print("upper trimmed: '");
  auto trimmed = text.trim_ascii();
  for ( usize i = 0; i < trimmed.size(); ++i ) micron::io::print(trimmed[i]);
  micron::io::println("'");

  // ================================================================
  // 7. iter() — get a raw_slice view from a slice/span
  // ----------------------------------------------------------------
  // Both slice<T> and span<T,N> expose .iter() / .iter_mut() returning
  // a raw_slice<T>. Use it to pass a non-owning handle into helper
  // functions without committing to a particular owning type.
  // ================================================================
  micron::io::println("-- 7. iter() --");

  auto view = sp.iter();
  micron::io::println("view.size=", view.size(), " view[0]=", view[0]);

  return 0;
}
