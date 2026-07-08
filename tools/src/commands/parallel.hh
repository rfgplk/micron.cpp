#pragma once

#include "linux/process/exec.hpp"

#include "chrono.hpp"

#include "io/console.hpp"

#include "linux/std.hpp"
#include "std.hpp"

// NOTE: process-level parallelism only
#include "linux/sys/sysfs.hpp"

#include "../recipes/gnu/config.hh"
#include "../recipes/gnu/qemu.hh"

#include "build.hh"

template<typename T = void>
u32
parallel_jobs(const auto &cfs)
  requires(recipes::__using_gnu)
{
  u32 jobs = 0;
  for ( const auto &conf : cfs )
    if ( conf.jobs ) {
      jobs = conf.jobs;
      break;
    }
  if ( !jobs ) jobs = static_cast<u32>(mc::posix::sysfs::cpu::online_count());
  return jobs ? jobs : 1;
}

template<typename F>
mc::vector<mc::status_t>
run_parallel(usize count, u32 jobs, F &&spawn_at)
  requires(recipes::__using_gnu)
{
  mc::vector<mc::status_t> stats;
  stats.reserve(count);
  usize next = 0;
  usize running = 0;
  auto fill = [&]() {
    while ( running < jobs and next < count ) {
      stats.push_back(spawn_at(next++));
      ++running;
    }
  };
  fill();
  while ( running > 0 ) {
    int wstat = 0;
    auto pid = mc::wait(&wstat);
    if ( pid <= 0 ) break;      // ECHILD: nothing left to reap
    for ( auto &s : stats )
      if ( s.pid == pid ) {
        s.status = wstat;
        s.pid = -pid;      // mark reaped so a recycled pid can't double-match
        break;
      }
    --running;
    fill();
  }
  return stats;
}

template<typename T = void>
int
build_parallel(auto &cfs)
  requires(recipes::__using_gnu)
{
  if ( cfs.empty() ) return 0;
  const u32 jobs = parallel_jobs(cfs);
  mc::set_color(mc::color::blue);
  mc::consoled("Building ");
  mc::set_color(mc::color::green);
  mc::consoled(cfs.size());
  mc::set_color(mc::color::blue);
  mc::consoled(" targets in parallel with ");
  mc::set_color(mc::color::green);
  mc::consoled(jobs);
  mc::set_color(mc::color::blue);
  mc::console(" jobs");
  mc::set_color(mc::color::reset);
  auto start = mc::now();

  auto stats = run_parallel(cfs.size(), jobs, [&](usize i) { return build<mc::exec_continue>(cfs[i]); });

  auto end = mc::now();
  int failed = 0;
  for ( usize i = 0; i < cfs.size(); i++ ) {
    if ( int r = mc::wexitstatus(stats[i].status); r != 0 ) {
      ++failed;
      mc::set_color(mc::color::red);
      mc::console("[ FAILED ", cfs[i].target, " exit: ", r, " ]");
    }
  }
  mc::set_color(failed ? mc::color::red : mc::color::green);
  mc::consoled(cfs.size() - static_cast<usize>(failed), " of ", cfs.size(), " targets built; ");
  mc::set_color(mc::color::yellow);
  if ( end - start > 1000 )
    mc::console("compilation took: ", (end - start) / 1000, " seconds");
  else
    mc::console("compilation took: ", (end - start), " milliseconds");
  mc::set_color(mc::color::reset);
  return failed ? 1 : 0;
}

template<typename T = void>
int
cicd_test_parallel(const auto &cfs)
  requires(recipes::__using_gnu)
{
  using namespace recipes::gnu;
  if ( cfs.empty() ) return 0;
  if ( !target_runnable(cfs[0]) ) {
    mc::set_color(mc::color::yellow);
    mc::console("Skipping ", cfs.size(), " target(s): missing ", __missing_for(cfs[0]));
    mc::set_color(mc::color::reset);
    return 0;
  }
  const u32 jobs = parallel_jobs(cfs);
  mc::set_color(mc::color::blue);
  mc::consoled("Building and testing ");
  mc::set_color(mc::color::green);
  mc::consoled(cfs.size());
  mc::set_color(mc::color::blue);
  mc::consoled(" targets in parallel with ");
  mc::set_color(mc::color::green);
  mc::consoled(jobs);
  mc::set_color(mc::color::blue);
  mc::console(" jobs");
  mc::set_color(mc::color::reset);

  auto built = run_parallel(cfs.size(), jobs, [&](usize i) { return build<mc::exec_continue>(cfs[i]); });

  int failed = 0;
  mc::vector<usize> runnable;
  for ( usize i = 0; i < cfs.size(); i++ ) {
    if ( mc::wexitstatus(built[i].status) != 0 ) {
      mc::set_color(mc::color::red);
      mc::console("[ ", cfs[i].target, " failed to compile ]");
      mc::set_color(mc::color::reset);
      ++failed;
    } else
      runnable.push_back(i);
  }

  auto start = mc::now();
  auto ran = run_parallel(runnable.size(), jobs, [&](usize i) { return run_target<mc::exec_continue>(cfs[runnable[i]]); });
  auto end = mc::now();

  for ( usize i = 0; i < runnable.size(); i++ ) {
    const int rc = verdict_of(ran[i].status);
    const bool ok = (rc == __snowball_pass);
    if ( !ok ) ++failed;
    mc::set_color(ok ? mc::color::green : mc::color::red);
    mc::console("[ ", cfs[runnable[i]].target_out, " returned: ", rc, "  ", decode_snowball(rc), " ]");
    mc::set_color(mc::color::reset);
  }
  mc::set_color(mc::color::yellow);
  if ( end - start > 1000 )
    mc::console("Execution took: ", (end - start) / 1000, " seconds");
  else
    mc::console("Execution took: ", (end - start), " milliseconds");
  mc::set_color(mc::color::reset);
  if ( failed != 0 ) {
    mc::set_color(mc::color::red);
    mc::console(failed, " of ", cfs.size(), " test(s) FAILED");
    mc::set_color(mc::color::reset);
  }
  return failed;
}
