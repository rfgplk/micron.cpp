#include "../../src/linux/users.hpp"
#include "../../src/thread/posix/limits.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.h"

int main(void)
{
  mc::posix::limits_t lm;
  mc::console((u64)lm.lim[0].rlim_cur);
  mc::console((u64)lm.lim[1].rlim_cur);
  mc::console((u64)lm.lim[2].rlim_cur);
  mc::console((u64)lm.lim[3].rlim_cur);
  mc::console((u64)lm.lim[4].rlim_cur);
  mc::console((u64)lm.lim[5].rlim_cur);
  mc::console((u64)lm.lim[6].rlim_cur);
  mc::console((u64)lm.lim[7].rlim_cur);
  mc::console((u64)lm.lim[8].rlim_cur);
  mc::console(mc::is_root());
  return 0;
}
