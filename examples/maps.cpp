// maps.cpp
// micron's hash-map family: robin_map, hopscotch_map, stack_swiss_map.
//
// See also:
//   examples/io.cpp — println(map) prints {k: v, ...} natively
//
// Key differences from std::unordered_map:
//   - FIXED capacity: no automatic resizing. Set capacity at construction.
//     Inserting past 7/8 load factor (robin/hopscotch) throws.
//   - find() returns V* (nullptr on miss), not an iterator.
//   - No operator[] that inserts on miss — use insert() explicitly.
//   - stack_swiss_map lives entirely on the stack (no heap).
//   - robin / hopscotch use SIMD probing (AVX2/SSE2/NEON).
//   - Aliases: micron::map<K,V> = robin_map,
//              micron::fmap<V>  = hopscotch_map<hash64_t, V>

#include "../src/except.hpp"
#include "../src/io/console.hpp"
#include "../src/map.hpp"

int
main()
{
  // ================================================================
  // --- robin_map<K, V> ---
  // Robin Hood open-addressing. Keeps probe sequences short by
  // "stealing" slots from rich elements.
  // ================================================================
  micron::io::println("-- robin_map --");

  micron::robin_map<int, int> rm;
  rm.insert(1, 100);
  rm.insert(2, 200);
  rm.insert(3, 300);

  // find() returns V* — nullptr if key absent (unlike std:: which returns end())
  int *val = rm.find(2);
  micron::io::println("find(2)=", val ? *val : -1);
  micron::io::println("find(99) is null=", rm.find(99) == nullptr);

  // contains() — boolean existence check
  micron::io::println("contains(1)=", rm.contains(1));
  micron::io::println("contains(5)=", rm.contains(5));

  // erase
  rm.erase(2);
  micron::io::println("after erase(2): contains(2)=", rm.contains(2));

  // size / capacity / empty
  micron::io::println("size=", rm.size(), " empty=", rm.empty());

  // Whole-map printing — println dispatches the map overload
  // (see examples/io.cpp for details)
  micron::io::println("rm = ", rm);

  // Or iterate — yields node references (not std::pair)
  for ( auto it = rm.begin(); it != rm.end(); ++it ) {
    micron::io::println("  key=", it->key, " val=", it->value);
  }

  // Convenience alias: micron::map<K,V>
  micron::map<int, int> m2;
  m2.insert(10, 1000);
  micron::io::println("alias map find(10)=", *m2.find(10));

  // ================================================================
  // --- hopscotch_map<K, V> ---
  // Hopscotch hashing: keys stay within a fixed "hop" neighbourhood.
  // Slightly more cache-friendly probe pattern than robin hood.
  // ================================================================
  micron::io::println("-- hopscotch_map --");

  micron::hopscotch_map<int, int> hm;
  hm.insert(7, 77);
  hm.insert(8, 88);
  micron::io::println("hopscotch find(7)=", *hm.find(7));

  // fmap<V> is hopscotch_map<hash64_t, V> — keys are pre-hashed 64-bit integers
  micron::fmap<int> fm;
  fm.insert(0xDEADBEEFu, 42);
  micron::io::println("fmap find(0xDEADBEEF)=", *fm.find(0xDEADBEEFu));

  // ================================================================
  // --- stack_swiss_map<K, V, N> ---
  // Swiss table on the stack. N must be a multiple of 16 and >= 16.
  // Zero heap allocation. Ideal for small, hot lookup tables.
  // ================================================================
  micron::io::println("-- stack_swiss_map --");

  micron::stack_swiss_map<int, int, 32> ssm;
  ssm.insert(1, 111);
  ssm.insert(2, 222);
  ssm.insert(3, 333);

  micron::io::println("ssm find(1)=", *ssm.find(1));
  micron::io::println("ssm size=", ssm.size());
  micron::io::println("ssm contains(3)=", ssm.contains(3));

  ssm.erase(2);
  micron::io::println("ssm after erase(2): contains(2)=", ssm.contains(2));

  return 0;
}
