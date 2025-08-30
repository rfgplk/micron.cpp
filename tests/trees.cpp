#include "../src/io/console.hpp"
#include "../src/std.hpp"
#include "../src/string/unistring.hpp"
#include "../src/string/strings.hpp"
#include "../src/trees/b.hpp"

int
main()
{
  [](){
    mc::b_tree<int, 5> b;
    for(int i = 0; i < 100000; ++i)
      b.insert(i);
    mc::console(b.search(4));
    mc::console(b.search(11));
  };
  {
    mc::b_tree<mc::string, 5> b;
    b.insert("Hi");
    b.insert("World");
    b.insert("Blllldgogk9e");
    b.insert("Hello...");
    b.insert("How are you?");
    for(int i = 0; i < 100000; ++i)
      b.insert(mc::to_string(i));
  }
  return 0;
}
