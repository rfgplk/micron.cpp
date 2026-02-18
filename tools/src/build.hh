#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "config.hh"

#include "batch.hh"

int
build(const config_t &conf)
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

  {
    auto command = batch(conf);
    mc::console("with command: ", command);
    mc::console("with command: ", command.size());
    mc::set_color(mc::color::reset);
    mc::execute<mc::exec_wait>(conf.compiler_path, command);
  };
  auto end = mc::now();
  mc::set_color(mc::color::yellow);
  mc::console("Compilation took: ", end - start, " milliseconds");
  mc::set_color(mc::color::reset);
  return 0;
}
