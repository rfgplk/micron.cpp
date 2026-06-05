#include "../src/except.hpp"
#include "../src/io/console.hpp"
#include "../src/trees.hpp"

#include "../src/algorithm/fix.hpp"
#include "../src/function.hpp"

#include "../src/range.hpp"
#include "../src/std.hpp"

int
main()
{
  mc::io::println("functional programming via trees");

  mc::rb_tree<u32> rb(100, [](u32 x) -> u32 { return x * 2; });

  mc::io::println("rb_tree is a tree: ", mc::is_tree<mc::rb_tree<u32>>);

  // sum
  mc::io::println(mc::fold(rb, 0, [](u32 a, const u32 &b) { return a + b; }));
  // all_of
  mc::io::println(mc::all_of(rb, [](const u32 &a) { return a > 0; }));
  mc::io::println(mc::all_of(rb, [](const u32 &a) { return a < mc::numeric_limits<u32>::max() / 2; }));

  // contains?
  for ( u32 n : mc::u32_range<0, 30>() ) mc::io::println("rb_tree contains [", n, "]? : ", rb.contains(n));

  auto rb2 = mc::fmap([](const u32 &n) -> u32 { return n % 10 == 0 ? 0 : n + 1; }, rb);
  for ( u32 n : mc::u32_range<0, 100>() ) mc::io::println("rb2 contains [", n, "]? : ", rb2.contains(n));
  return 0;
};
