
#include "../src/io/console.hpp"
#include "../src/map.hpp"
#include "../src/maps/hopscotch.hpp"
#include "../src/std.h"
#include "../src/string/strings.h"


int
main(void)
{
  //std::unordered_map<mc::hash64_t, int> hmp;
  mc::hopscotch_map<mc::hash64_t, int> hmp(25000);
  for ( size_t i = 0; i < 10000; i++ ) {
    try {
      hmp.insert(i, 5 * i);
    } catch ( mc::except::library_error &e ) {
    }
  }
  mc::console("Done");
  mc::console("Done");
  return 0;
  mc::map<mc::string, int> mp;
  mc::console(mp.size());
  mc::console(mp.max_size());
  mp.emplace("hello", 1);
  mp.insert("hello", 1);
  mp.insert("hello", 1);
  mp.insert("cat", 1);
  mp.insert("cat", 1);
  mc::console(mp.count("hello"));
  mc::console(mp.count("cat"));
  return 0;
}
