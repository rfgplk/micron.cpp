#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/vector/vector.hpp"
#include "../../src/string/strings.hpp"


int
main()
{
  if constexpr ( true ) {
    mc::string first = "Hello World!";
    mc::string second = "!olleH dlroW";
    mc::vector<int> third(65536, 'A'); // shouldn't fire
    mc::posix::write(1, first.data(), first.size());
    mc::posix::write(1, second.data(), second.size());
    mc::vector<int> fourth(512, 'B'); // should fire
    mc::posix::write(1, "\n", 1);
    mc::posix::write(1, first.data(), 24); // size got reset
    mc::posix::write(1, second.data(), 24);
    mc::string fifth = "reset once again";
    for(int n : fourth)
      mc::posix::write(1, &n, sizeof(int));
  }
  return 0;
}
