#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "config.hh"

#include "batch.hh"

void
cicd_test(const auto &cfs)
{
  struct test_t {
    string_type name;
    mc::status_t stat;
  };

  mc::fvector<test_t> stats;
  stats.reserve(cfs.size());
  for ( const auto &conf : cfs ) {
    mc::set_color(mc::color::blue);
    mc::consoled("Building and testing: ");
    mc::set_color(mc::color::green);
    mc::console(conf.target);
    mc::set_color(mc::color::reset);

    {
      auto command = batch(conf);
      if ( int r = mc::wexitstatus(mc::execute<mc::exec_wait>(conf.compiler_path, command)); r != 0 )
        mc::cerror("Failed to compile");
    };
    mc::console("Testing: ", conf.target_out);

    auto start = mc::now();
    stats.emplace_back(conf.target_out, mc::execute<mc::exec_wait>(conf.target_out));
    auto end = mc::now();
    mc::console("Execution took: ", end - start, " milliseconds");
  }
  for ( auto &n : stats )
    mc::console("[ ", n.name, " returned: ", mc::wexitstatus(n.stat.status), " ]");
}
