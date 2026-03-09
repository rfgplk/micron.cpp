//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/fsys.hpp"
#include "../src/io/filesystem.hpp"
#include "../src/io/paths.hpp"
#include "../src/io/serial.hpp"
#include "../src/std.hpp"

#include "../src/io/stream.hpp"

#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

#include "../snowball/snowball.hpp"

int
main(void)
{
  enable_scope() {
    mc::string str;
    auto file = mc::io::open_file("/etc/bashrc");
    file.read(str);

    mc::console(str);
    mc::console("Size is: ", file.size());
    mc::console("Is virtual: ", file.is_system_virtual());
    mc::console("Owner: ", file.owner());
    mc::console("Access: ", file.access());
    mc::console("Modified: ", file.modified());
    mc::console("Is valid: ", file.valid());
    mc::console("Fd: ", file.raw_fd());
    mc::console("Inode: ", file.inode());
    mc::console("Device: ", file.device());
  }
  disable_scope() {
    mc::sstring<1<<16> str;
    auto file = mc::io::open_file("tools/src/main.cc");
    file.read(str);

    mc::console(str);
  }
  disable_scope()
  {
    mc::string str;
    mc::fsys::system<mc::io::rd> sys;
    sys["tools/src/main.cc"] >> str;
    mc::console(str);
  }
  return 0;
}
