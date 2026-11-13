#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "../recipes/gnu/config.hh"

#include "../recipes/gnu/batch.hh"
#include "../recipes/gnu/qemu.hh"

template <typename T = void>
__attribute__((noreturn)) void
build_and_run(const recipes::gnu::config_t &conf)
  requires(recipes::__using_gnu)
{
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
    r = mc::wexitstatus(recipes::gnu::execute_batch<mc::exec_wait>(conf, command));
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
  mc::console("Running: ", conf.target_out);
  if ( conf.arch == recipes::gnu::__arch::wasm32 ) {
    if ( !recipes::gnu::target_runnable(conf) ) mc::cerror("missing WASM runner: ", recipes::gnu::__missing_for(conf));
    const auto result = recipes::gnu::run_target_timed(conf);
    mc::proc_exit(recipes::gnu::verdict_of(result.status));
  }
  mc::rexecute(conf.target_out);
}
