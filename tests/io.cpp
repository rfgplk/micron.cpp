//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/filesystem.hpp"
#include "../src/std.h"
#include "../src/string/format.h"

int
main(void)
{
  mc::fsys::file f("/usr/include/c++/14/cstddef", mc::io::modes::read, 4096);
  mc::infolog(f.name());
  try {
    f.read_bytes(2500);
  } catch ( const mc::except::io_error &e ) {
    mc::infolog(e.what());
    return 0;
  }
  mc::infolog(f.count());
  mc::infolog(f.get());
  mc::console_newline();
  mc::console_newline();
  mc::console_newline();
  mc::fsys::file d("/usr/include/c++/14/atomic", mc::io::modes::read, 4096);
  try {
    d.set(85);
    d.read_bytes(39);
    mc::infolog(d.count());
    mc::infolog(d.get());
    mc::infolog(mc::format::starts_with(d.get(), "// This file is part of the GNU ISO C++"));
  } catch ( const mc::except::io_error &e ) {
    mc::infolog(e.what());
    return 0;
  }
  mc::ustr8 file_contents(micron::move(d.pull()));
  d.reopen("/usr/include/c++/14/cmath");

  d.load();
  mc::infolog(d.get());
  mc::infolog(file_contents);
  d.reopen("/tmp/hello.txt", mc::io::modes::append);
  d.push(micron::move(file_contents));
  mc::infolog(d.get());
  d.write();
  return 1;
}
