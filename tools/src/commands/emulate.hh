#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "../recipes/gnu/config.hh"

#include "../recipes/gnu/batch.hh"

template <typename T = void>
__attribute__((noreturn)) void
build_and_emulate(const recipes::gnu::config_t &conf)
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
    mc::abort();
  }
  mc::set_color(mc::color::reset);
  mc::console("Emulating: ", conf.target_out);
  // NOTE: currently arm only, we'll extend it later to more archs
  mc::uprocess_t emu_p("/usr/bin/qemu-arm-static", "-L", "/usr/gcc-linaro/arm-none-linux-gnueabihf/libc", conf.target_out.c_str());
  mc::rexecute(emu_p);
}
