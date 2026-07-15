#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

#include "../recipes/gnu/config.hh"

#include "../recipes/gnu/batch.hh"
#include "../recipes/gnu/qemu.hh"

template<typename T = void>
int
cicd_test(const auto &cfs)
  requires(recipes::__using_gnu)
{
  using namespace recipes::gnu;

  struct test_t {
    string_type name;
    int rc;
  };

  if ( cfs.empty() ) return 0;

  // a cross target we can't build or launch here is skipped, not failed
  if ( !target_runnable(cfs[0]) ) {
    mc::set_color(mc::color::yellow);
    mc::console("Skipping ", cfs.size(), " target(s): missing ", __missing_for(cfs[0]));
    mc::set_color(mc::color::reset);
    return 0;
  }

  int failed = 0;
  mc::fvector<test_t> stats;
  stats.reserve(cfs.size());
  for ( const auto &conf : cfs ) {
    mc::set_color(mc::color::blue);
    mc::consoled("Building and testing: ");
    mc::set_color(mc::color::green);
    mc::console(conf.target);
    mc::set_color(mc::color::reset);

    {
      auto command = recipes::gnu::batch(conf);
      if ( int r = mc::wexitstatus(mc::execute<mc::exec_wait>(conf.compiler_path, command)); r != 0 ) {
        mc::set_color(mc::color::red);
        mc::console("[ ", conf.target, " failed to compile ]");
        mc::set_color(mc::color::reset);
        ++failed;
        continue;
      }
    };
    mc::console(needs_emulation(conf) ? "Emulating: " : "Testing: ", conf.target_out);

    auto start = mc::now();
    auto status = run_target_timed<mc::exec_wait>(conf);      // --timeout <sec>, else identical to run_target
    auto end = mc::now();
    stats.emplace_back(conf.target_out, verdict_of(status.status));
    if ( end - start > 1000 )
      mc::console("Execution took: ", (end - start) / 1000, " seconds");
    else
      mc::console("Execution took: ", (end - start), " milliseconds");
  }

  for ( auto &n : stats ) {
    const bool ok = (n.rc == __snowball_pass);
    if ( !ok ) ++failed;
    mc::set_color(ok ? mc::color::green : mc::color::red);
    mc::console("[ ", n.name, " returned: ", n.rc, "  ", decode_snowball(n.rc), " ]");
    mc::set_color(mc::color::reset);
  }
  if ( failed != 0 ) {
    mc::set_color(mc::color::red);
    mc::console(failed, " of ", cfs.size(), " test(s) FAILED");
    mc::set_color(mc::color::reset);
  }
  return failed;
}
