#pragma once

#include "../syscall.hpp"
#include <unistd.h>

namespace micron
{

int
yield()
{
  ::syscall(SYS_sched_yield);
  return 0;
}

};
