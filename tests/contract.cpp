#include "../src/sync/contract.hpp"
#include "../src/io/console.hpp"
#include "../src/parallel/for.hpp"
#include "../src/std.hpp"
#include "../src/vector/vector.hpp"

#include "snowball/snowball.hpp"

int x = 0;

int
is_ten(void)
{
  if ( x == 10 )
    return x;
  return x;
}

bool
enforcement_fn(void)
{
  // error of something
  mc::console("Contract violation!");
  return false;     // don't terminate
}

int
main()
{
  // Simulating work
  mc::go([&x](){ mc::ssleep(2); x = 10; });
  enable_scope() { mc::contract<mc::contract_state::lenient, int> contract(is_ten); };
  enable_scope()
  {
    mc::contract<mc::contract_state::enforcing, int> contract(is_ten);
    contract.enforce(enforcement_fn);
    contract.sign();
  };
  // enable_scope() { mc::contract<mc::contract_state::strict, int> contract(is_ten); };
  return 0;
}
