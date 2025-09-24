#include "../src/io/uxin/uxin.hpp"
#include "../src/io/uxin/virtual.hpp"
#include "../src/std.hpp"

int
main()
{
  try {
    mc::uxin::make_uinput();
  } catch ( mc::except::memory_error &e ) {
    mc::console(e.what());
  }

  return 1;
}
