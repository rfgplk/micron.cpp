
#include "../../src/cmalloc.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/string/strings.hpp"

#include <random>

struct s {
  int x;
  int y;
};
int
main()
{
  if constexpr ( true ) {
    byte *buf = abc::alloc(65536);
    mc::sstr<32> *tst = new (buf) mc::sstr<32>("Hello World!");
    mc::console(tst->c_str());
    volatile byte *p;
    for ( int i = 0; i < 1000; i++ )
      p = abc::alloc(65536);
    mc::console(p);
    mc::console(tst->c_str());
    auto st = abc::fetch<s>();
    st->x = 5;
    st->y = 10;
    mc::console(st->x);
    mc::console(st->y);
    abc::freeze(buf);
    mc::infolog("Success");
  }
  if constexpr ( false ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000000);
    for ( size_t n = 0; n < (2 << 8); ++n ) {
      abc::malloc(dist(gen));     // oom checking
    }
    mc::infolog("Success");
  }
  return 0;
}
