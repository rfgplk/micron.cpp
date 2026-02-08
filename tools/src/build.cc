
#include "../../src/linux/process/exec.hpp"
#include "../src/control.hpp"
#include "../src/linux/process/fork.hpp"
#include "../src/linux/process/process.hpp"
#include "../src/std.hpp"
#include "../src/thread/signal.hpp"

#include "../src/io/console.hpp"
#include "../src/io/filesystem.hpp"

#include "../../src/linux/std.hpp"

int
main(int argc, char **argv)
{
  // replace this with out custom build system eventually
  mc::execute("tools/ninja", argv);
  return 0;
}
