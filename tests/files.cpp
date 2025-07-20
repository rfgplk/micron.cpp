//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../src/io/fsys.hpp"
#include "../src/io/filesystem.hpp"
#include "../src/io/paths.hpp"
#include "../src/io/serial.hpp"
#include "../src/std.h"

#include "../src/control.hpp"
#include "../src/errno.hpp"
#include "../src/io/console.hpp"

#include <fstream>      // for std::ifstream
#include <iostream>     // for std::cout

/*
int main() {
    std::ifstream file("TODO");
    if (!file) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    file.close();
    return 0;
}*/
int
main(void)
{
  mc::ustr8 str;
  mc::ustr8 assertfile;
  mc::ustr8 binfile;
  mc::fsys::system<mc::io::rwc> sys;     //("build.ninja");
  // for(size_t i = 0; i < 100000; i++)
  {
    // sys["TODO", mc::io::rw]; or this
    mc::console((int)mc::io::get_type_at("/proc/self/maps"));
    mc::console((int)mc::io::get_type_at("/proc"));
    mc::console((int)mc::io::get_type_at("/dev/sda"));
    mc::console((int)mc::io::get_type_at("/tmp"));
    mc::console((int)mc::io::get_type_at("/tmp/testing"));
    mc::console((int)mc::io::get_type_at("/dev/null"));
    try {
      sys["/tmp/data"];
      sys["/tmp/second"];
      sys["/tmp/testing"] >> str;
      sys["/usr/include/assert.h", mc::io::rd] >> assertfile;
      sys["/bin/errno", mc::io::rd] >> binfile;
    } catch ( mc::except::io_error &e ) {
      mc::console(mc::error::what_errno());
      return -1;
    }
    auto p = sys.list();
    for ( const auto &n : p )
      mc::console(n);
    mc::console("Is opened /usr/include/err.h: ", sys.is_opened("/usr/include/err.h"));
    mc::console("Is opened /tmp/second: ", sys.is_opened("/tmp/second"));
    // mc::console(str);
    // mc::console(sys.is_regular_file("/tmp/testing"));
    // mc::console(sys.is_directory("/tmp/newfile"));
    // mc::console(sys.is_block_device("/tmp/bofile"));
    str += "This is a test! Let's hope it works!\n";
    sys["/tmp/testing"] << str;
    sys.rename("/tmp/third", "/tmp/fifth");
    sys.copy("/tmp/testing", "/tmp/second");
    sys.copy_list("/tmp/testing", "/tmp/second", "/tmp/third", "/tmp/fourth");
    str.clear();
  }
  // f.load();
  // mc::console(f.pull());
  return 0;
  /*mc::io::path p("/");
  mc::io::println("Created path.");
  mc::io::println("Current path is: ", p.get());
  auto ps = p.dirs();
  mc::io::println("Subdirectories:");
  for ( auto n : ps )
    mc::io::println(n);
  auto fs = p.files();
  mc::io::println("Files:");
  for ( auto n : fs )
    mc::io::println(n);*/
}
