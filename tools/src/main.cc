#include "linux/std.hpp"
#include "std.hpp"

#include "build.hh"
#include "clean.hh"
#include "make.hh"
#include "run.hh"
#include "test.hh"

enum class __modes : i32 { build, run, make, test, clean, __end };

auto
match(char **argv) -> __modes
{
  // builds file/project
  if ( mc::strcmp(argv[1], "build") == 0 )
    return __modes::build;
  // builds file and runs
  else if ( mc::strcmp(argv[1], "run") == 0 )
    return __modes::run;
  // makes a new project
  else if ( mc::strcmp(argv[1], "make") == 0 )
    return __modes::make;
  // builds test files and runs
  else if ( mc::strcmp(argv[1], "test") == 0 )
    return __modes::test;
  // cleans unnecessary files
  else if ( mc::strcmp(argv[1], "clean") == 0 )
    return __modes::clean;
  // nothing
  return __modes::__end;
}

int
main(int argc, char **argv)
{
  if ( argc < 3 ) {
    mc::cerror("Invalid command arguments provided");
  }
  auto mode = match(argv);
  switch ( mode ) {
  case __modes::build : {
    config_t conf = parse_argv_build(argc - 2, argv + 2);
    return build(conf);
    break;
  }
  case __modes::run : {
    break;
    // config_t conf = parse_argv_run(argc - 2, argv + 2);
    // return run(conf);
  }
  case __modes::make : {
    break;
    // config_t conf = parse_argv_make(argc - 2, argv + 2);
    // return make(conf);
  }
  case __modes::test : {
    break;
    // config_t conf = parse_argv_test(argc - 2, argv + 2);
    // return test(conf);
  }
  case __modes::clean : {
    break;
  }
  case __modes::__end : {
    mc::cerror("Invalid command argument provided");
  }
  };
  return -1;
};
