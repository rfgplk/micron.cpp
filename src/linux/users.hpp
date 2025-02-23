#pragma once

#include "../thread/posix/system.hpp"

namespace micron
{
bool
is_root(void)
{
  // NOTE: only a surface level check, root may not have a UID of 0
  return (posix::getuid() == 0 or posix::geteuid() == 0);
}
};
