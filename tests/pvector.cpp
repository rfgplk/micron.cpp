#include "../src/vector/pvector.hpp"
#include "../src/range.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

#include "../src/io/console.hpp"

int
main()
{
  {
    mc::pvector<int, 6, 8> pvec(10);
    mc::console(pvec.size());
    mc::console(pvec.max_size());
    mc::console(pvec.set(1, 25));     // yields new vec
    mc::console(pvec);
  }
  {
    mc::pvector<mc::vector<int>> pvec(5);
    mc::console(pvec.size());
    mc::console(pvec);
    mc::console(pvec.set(1, { 5, 5, 5, 5 }));
    // should look like this
    //5
    //{ {  }, {  }, {  }, {  }, {  } }
    //{ {  }, { 5, 5, 5, 5 }, {  }, {  }, {  } }
  }
  return 1;
}
