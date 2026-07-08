#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "../recipes/gnu/config.hh"

#include "../recipes/gnu/batch.hh"
#include "../recipes/gnu/qemu.hh"

// build a single source then run it under user-mode qemu
template<typename T = void>
int
build_and_emulate(const recipes::gnu::config_t &conf)
  requires(recipes::__using_gnu)
{
  if ( !recipes::gnu::needs_emulation(conf) )
    mc::cerror("emulate needs a cross target: pass --arm or --arm64 (a native binary should use `duck run`)");
  if ( !recipes::gnu::target_runnable(conf) ) {
    mc::set_color(mc::color::red);
    mc::console("Cannot emulate: missing ", recipes::gnu::__missing_for(conf));
    mc::set_color(mc::color::reset);
    mc::abort();
  }
  mc::set_color(mc::color::blue);
  mc::consoled("Building ");
  mc::set_color(mc::color::green);
  mc::consoled(conf.target);
  mc::set_color(mc::color::blue);
  mc::consoled(" into ");
  mc::set_color(mc::color::green);
  mc::console(conf.target_out);
  mc::set_color(mc::color::yellow);
  auto start = mc::now();
  int r = 0;
  {
    auto command = recipes::gnu::batch(conf);
    mc::console("with command: ", command);
    mc::set_color(mc::color::reset);
    r = mc::wexitstatus(mc::execute<mc::exec_wait>(conf.compiler_path, command));
  };
  auto end = mc::now();
  mc::set_color(mc::color::yellow);
  if ( end - start > 1000 )
    mc::console("Compilation took: ", (end - start) / 1000, " seconds");
  else
    mc::console("Compilation took: ", (end - start), " milliseconds");

  // don't run bin if compilation failed, avoids invoking prior compiled version if it exists

  if ( r != 0 ) {
    mc::console("Compilation failed with error code: ", r);
    mc::set_color(mc::color::reset);      // don't leave the terminal stained yellow on abort
    mc::abort();
  }
  mc::set_color(mc::color::reset);
  mc::console("Emulating: ", conf.target_out);
  auto status = recipes::gnu::run_target<mc::exec_wait>(conf);
  const int rc = recipes::gnu::verdict_of(status.status);
  mc::console("[ ", conf.target_out, " returned: ", rc, " ]");
  return rc;
}
