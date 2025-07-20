//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/fsys.hpp"
#include "../src/io/filesystem.hpp"
#include "../src/io/paths.hpp"
#include "../src/io/serial.hpp"
#include "../src/string/unistring.hpp"
#include "../src/std.h"

#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

int
main(void)
{
  mc::io::path_t bg = "/as/345";
  mc::console(mc::fsys::valid_path(bg));
  for(int i = 0; i < 100; i++)
    mc::fsys::make("/tmp/" + mc::to_string(i));
}
