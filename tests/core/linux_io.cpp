//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/linux/io/ext.hpp"
#include "../src/linux/io/fd.hpp"
#include "../src/linux/io/inode.hpp"
#include "../src/linux/io/io_structs.hpp"
#include "../src/linux/io/sys.hpp"


#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

int
main(void)
{
  enable_scope() {
    auto fd = mc::posix::open("/etc/fedora-release", 0, 0);
  }
  return 0;
}
