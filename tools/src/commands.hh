#pragma once

#include "commands/build.hh"
#include "commands/doctor.hh"
#include "commands/help.hh"
#include "commands/parallel.hh"
#include "commands/run.hh"
#include "commands/emulate.hh"
#include "commands/test.hh"

#include "splat.hh"

#include "../../src/io/io.hpp"

enum class __modes : i32 { build, batch, link, compile, debug, emulate, run, make, test, doctor, recipes, __end };

template <typename T = void>
auto
match(char **argv) -> __modes
  requires(recipes::__using_gnu)
{
  // builds file/project - if file build single file, if dir, every file in dir
  if ( mc::strcmp(argv[1], "build") == 0 ) return __modes::build;
  // reads a sh/makefile like file and executes argv from it sequentially
  else if ( mc::strcmp(argv[1], "batch") == 0 )
    return __modes::batch;
  // links obj files together
  else if ( mc::strcmp(argv[1], "link") == 0 )
    return __modes::link;
  // compiles file. does not wait
  else if ( mc::strcmp(argv[1], "compile") == 0 )
    return __modes::compile;
  // shorthand for build -d/-g -w
  else if ( mc::strcmp(argv[1], "debug") == 0 )
    return __modes::debug;
  // builds file and runs it, replacing current proc.
  else if ( mc::strcmp(argv[1], "run") == 0 )
    return __modes::run;
  // same as run, but emulates via qemu
  else if ( mc::strcmp(argv[1], "emulate") == 0 )
    return __modes::emulate;
  // makes a new project from template
  else if ( mc::strcmp(argv[1], "make") == 0 )
    return __modes::make;
  // builds test files and runs them (under qemu for cross targets)
  else if ( mc::strcmp(argv[1], "test") == 0 )
    return __modes::test;
  else if ( mc::strcmp(argv[1], "doctor") == 0 )
    return __modes::doctor;
  else if ( mc::strcmp(argv[1], "recipes") == 0 )
    return __modes::recipes;
  // nothing
  return __modes::__end;
}

// duck splat <cmdline>: emit the command(s) we would have issued
template <typename T = void>
inline int
splat_dispatch(__modes mode, int argc, char **argv)
  requires(recipes::__using_gnu)
{
  using namespace recipes::gnu;
  auto emit = [](const string_type &command) { mc::console(command); };

  switch ( mode ) {
  case __modes::build :
  case __modes::test : {
    const int fi = ( argc > 2 and mc::strcmp(argv[2], "parallel") == 0 ) ? 3 : 2;
    if ( argc <= fi ) mc::cerror("splat requires at least one source");
    for ( auto &conf : parse_argv_build(argc - fi, argv + fi) ) {
      emit(batch(conf));
      // test compiles each target and then runs it; splat shows both, in that order
      if ( mode == __modes::test ) emit(run_command(conf));
    }
    break;
  }
  case __modes::compile :
  case __modes::link : {
    for ( auto &conf : parse_argv_build(argc - 2, argv + 2) ) emit(batch(conf));
    break;
  }
  case __modes::debug : {
    for ( auto &conf : parse_argv_build(argc - 2, argv + 2) ) {
      recipe_debug(conf);
      emit(batch(conf));
    }
    break;
  }
  case __modes::doctor : {
    for ( auto &conf : parse_argv_build(argc - 2, argv + 2) ) {
      recipe_doctor(conf);
      emit(batch(conf));
    }
    break;
  }
  case __modes::run : {
    config_t conf = parse_argv_build_single(argc - 2, argv + 2);
    emit(batch(conf));
    emit(conf.target_out);      // run execs the binary directly, never under qemu
    break;
  }
  case __modes::emulate : {
    config_t conf = parse_argv_build_single(argc - 2, argv + 2);
    emit(batch(conf));
    emit(run_command(conf));
    break;
  }
  // batch is handled by parse_main; make/recipes are reserved. nothing to issue for any of them
  case __modes::batch :
  case __modes::make :
  case __modes::recipes : {
    break;
  }
  case __modes::__end : {
    mc::cerror("Invalid command argument provided");
  }
  };
  return 0;
}

template <typename T = void>
inline int
parse_main(int argc, char **argv)
  requires(recipes::__using_gnu)
{
  using namespace recipes::gnu;
  if ( argc >= 2 and argv[1] != nullptr and mc::strcmp(argv[1], "splat") == 0 ) {
    if ( argc < 3 ) mc::cerror("splat must be followed by a duck command line, ie duck splat build src/main.cpp");
    const bool __was_splatting = splat::__active;
    splat::__active = true;
    mc::vector<char *> __argv;
    __argv.push_back(argv[0]);
    for ( int i = 2; i < argc; ++i ) __argv.push_back(argv[i]);
    const int r = parse_main(static_cast<int>(__argv.size()), __argv.data());
    splat::__active = __was_splatting;
    return r;
  }
  // duck <recipe>.duck  is equal to duck batch <recipe>.duck
  if ( argc >= 2 and argv[1] != nullptr and string_type{ argv[1] }.ends_with(".duck") ) {
    mc::vector<char *> __argv;
    __argv.push_back(argv[0]);
    char batch_lit[] = "batch";
    __argv.push_back(batch_lit);
    for ( int i = 1; i < argc; ++i ) __argv.push_back(argv[i]);
    return parse_main(static_cast<int>(__argv.size()), __argv.data());
  }
  if ( argc < 3 ) [[unlikely]] {
    if ( argc == 2 )
      if ( (mc::strcmp(argv[1], "help") == 0) or (mc::strcmp(argv[1], "--help") == 0) or (mc::strcmp(argv[1], "--h") == 0)
           or (mc::strcmp(argv[1], "-h") == 0) ) {
        help();
        return 0;
      }
    mc::cerror("Invalid command arguments provided");
  }
  auto mode = match(argv);
  // splat: emit what we would have run then stop
  if ( splat::active() and mode != __modes::batch ) return splat_dispatch(mode, argc, argv);
  switch ( mode ) {
  case __modes::build : {
    if ( argc > 2 and mc::strcmp(argv[2], "parallel") == 0 ) {
      if ( argc < 4 ) mc::cerror("build parallel requires at least one source");
      auto confs = parse_argv_build(argc - 3, argv + 3);
      return build_parallel(confs);
    }
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs ) build<mc::exec_wait>(conf);
    break;
  }
  case __modes::batch : {
    bool parallel = false;
    int fi = 2;
    if ( argc > 2 and mc::strcmp(argv[2], "parallel") == 0 ) {
      parallel = true;
      fi = 3;
    }
    if ( parallel ? argc <= fi : argc != fi + 1 )
      mc::cerror("Must provide a path to a valid batchfile");
    if ( !mc::posix::exists(argv[fi]) ) mc::cerror("File doesn't exist");
    // there is nothing to pool when we only print
    if ( splat::active() ) parallel = false;
    mc::string batchfile;
    auto __f = mc::io::open_file(argv[fi]);
    mc::io::read(__f, batchfile);

    auto lines = mc::fmt::splitlines(batchfile);

    mc::vector<recipes::gnu::config_t> pool;
    mc::vector<mc::string> deferred;
    int rc = 0;      // a batchfile fails if any of its lines fails

    for ( auto &line : lines ) {
      // strip comments and blank lines
      mc::fmt::strip(line);
      if ( line.empty() || line[0] == '#' ) continue;

      auto tokens = mc::fmt::split_to(line, "");

      if ( tokens.empty() ) continue;

      if ( parallel ) {
        if ( tokens[0] == "build" or tokens[0] == "compile" or tokens[0] == "link" ) {
          mc::vector<char *> __argv;
          usize from = (tokens.size() > 1 and tokens[1] == "parallel") ? 2 : 1;
          for ( usize i = from; i < tokens.size(); i++ ) __argv.push_back(tokens[i].begin());
          if ( __argv.empty() ) continue;
          auto cfs = parse_argv_build(static_cast<int>(__argv.size()), __argv.data());
          for ( auto &c : cfs ) pool.move_back(mc::move(c));
        } else
          deferred.push_back(line);
        continue;
      }

      // first entry is nullptr, parse_main expects the first arg to be bin name (posix convention)
      mc::vector<char *> __argv;
      __argv.push_back(nullptr);

      for ( auto &tok : tokens ) __argv.push_back(tok.begin());

      if ( int r = parse_main(static_cast<int>(__argv.size()), __argv.data()); r != 0 ) rc = r;
    }

    if ( parallel ) {
      if ( int r = build_parallel(pool); r != 0 ) rc = r;
      for ( auto &line : deferred ) {
        auto tokens = mc::fmt::split_to(line, "");
        if ( tokens.empty() ) continue;
        mc::vector<char *> __argv;
        __argv.push_back(nullptr);
        for ( auto &tok : tokens ) __argv.push_back(tok.begin());
        if ( int r = parse_main(static_cast<int>(__argv.size()), __argv.data()); r != 0 ) rc = r;
      }
    }

    return rc;
  }
  case __modes::compile : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    mc::vector<mc::status_t> stats;
    stats.reserve(confs.size());
    for ( auto &conf : confs ) stats.push_back(build<mc::exec_continue>(conf));
    // reap before exiting: don't orphan compilers, and surface their failures
    for ( auto &s : stats ) mc::waitpid(s);
    for ( usize i = 0; i < confs.size(); i++ )
      if ( int r = mc::wexitstatus(stats[i].status); r != 0 ) mc::console("[ ", confs[i].target, " failed, exit: ", r, " ]");
    break;
  }
  case __modes::link : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    mc::vector<mc::status_t> stats;
    stats.reserve(confs.size());
    for ( auto &conf : confs ) stats.push_back(build<mc::exec_continue>(conf));
    for ( auto &s : stats ) mc::waitpid(s);
    for ( usize i = 0; i < confs.size(); i++ )
      if ( int r = mc::wexitstatus(stats[i].status); r != 0 ) mc::console("[ ", confs[i].target, " failed, exit: ", r, " ]");
    break;
  }
  case __modes::debug : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs ) build_debug(conf);
    break;
  }
  case __modes::run : {
    // can't be batched doesn't make sense
    config_t conf = parse_argv_build_single(argc - 2, argv + 2);
    build_and_run(conf);
    break;
  }
  case __modes::emulate : {
    config_t conf = parse_argv_build_single(argc - 2, argv + 2);
    return build_and_emulate(conf);
  }
  case __modes::make : {
    break;
    // config_t conf = parse_argv_make(argc - 2, argv + 2);
    // return make(conf);
  }
  case __modes::test : {
    if ( argc > 2 and mc::strcmp(argv[2], "parallel") == 0 ) {
      if ( argc < 4 ) mc::cerror("test parallel requires at least one source");
      auto confs = parse_argv_build(argc - 3, argv + 3);
      return cicd_test_parallel(confs);
    }
    auto confs = parse_argv_build(argc - 2, argv + 2);
    return cicd_test(confs);
  }
  case __modes::doctor : {
    auto confs = parse_argv_build(argc - 2, argv + 2);
    for ( auto &conf : confs ) doctor<mc::exec_wait>(conf);
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
