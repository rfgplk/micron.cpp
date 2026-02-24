#include "linux/std.hpp"
#include "std.hpp"

#include "build.hh"
#include "clean.hh"
#include "help.hh"
#include "make.hh"
#include "run.hh"
#include "test.hh"

enum class __modes : i32 { build, compile, debug, run, make, test, recipes, __end };

auto
match(char **argv) -> __modes
{
  // builds file/project - if file build single file, if dir, every file in dir
  if ( mc::strcmp(argv[1], "build") == 0 )
    return __modes::build;
  // compiles file. does not wait
  else if ( mc::strcmp(argv[1], "compile") == 0 )
    return __modes::compile;
  // shorthand for build -d/-g -w
  else if ( mc::strcmp(argv[1], "debug") == 0 )
    return __modes::debug;
  // builds file and runs it, replacing current proc.
  else if ( mc::strcmp(argv[1], "run") == 0 )
    return __modes::run;
  // makes a new project from template
  else if ( mc::strcmp(argv[1], "make") == 0 )
    return __modes::make;
  // builds test files and runs, verifies if each exits with '1' (success) or '0' (fail)
  else if ( mc::strcmp(argv[1], "test") == 0 )
    return __modes::test;
  else if ( mc::strcmp(argv[1], "recipes") == 0 )
    return __modes::recipes;
  // nothing
  return __modes::__end;
}

int
main(int argc, char **argv)
{
  if ( argc < 3 ) [[unlikely]] {
    if ( argc == 2 )
      if ( mc::strcmp(argv[1], "help") == 0 ) {
        help();
        return 0;
      }
    mc::cerror("Invalid command arguments provided");
  }
  auto mode = match(argv);
  switch ( mode ) {
  case __modes::build : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs )
      build<mc::exec_wait>(conf);
    break;
  }
  case __modes::compile : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs )
      build<mc::exec_continue>(conf);
    break;
  }
  case __modes::debug : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs )
      build_debug(conf);
    break;
  }
  case __modes::run : {
    // can't be batched doesn't make sense
    config_t conf = parse_argv_build_single(argc - 2, argv + 2);
    build_and_run(conf);
    break;
  }
  case __modes::make : {
    break;
    // config_t conf = parse_argv_make(argc - 2, argv + 2);
    // return make(conf);
  }
  case __modes::test : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    cicd_test(confs);
    break;
  }
  case __modes::recipes : {
    break;
  }
  case __modes::__end : {
    mc::cerror("Invalid command argument provided");
  }
  };
  return 0;
};
