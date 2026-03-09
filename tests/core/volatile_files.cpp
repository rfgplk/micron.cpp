//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/filesystem.hpp"
#include "../src/io/fsys.hpp"
#include "../src/io/paths.hpp"
#include "../src/io/serial.hpp"
#include "../src/std.hpp"

#include "../src/io/stream.hpp"

#include "../src/io/posix/volatile.hpp"

#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

#include "../snowball/snowball.hpp"

int
main(void)
{
  enable_scope()
  {
    mc::io::volatile_t vol("my_file");
    mc::sstring<1 << 12> str("Hello World");
    mc::sstring<1 << 12> blank;
    vol.write(str);
    vol.write(str);
    vol.write(str);
    vol.write(str);
    vol.rewind();
    blank.set_size(vol.read(blank));
    mc::console("Size: ", blank.size());
    mc::console(blank);
  }
  return 0;
}
