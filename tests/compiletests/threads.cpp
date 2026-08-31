//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/thread/threads.hpp"
#include "../../src/atomic/atomic.hpp"
#include "../../src/mutex/barrier.hpp"
#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/mutex.hpp"
#include "../../src/mutex/once.hpp"
#include "../../src/parallel/algo.hpp"
#include "../../src/parallel/for.hpp"
#include "../../src/stdthread.hpp"
#include "../../src/sync/channel.hpp"
#include "../../src/sync/event_count.hpp"
#include "../../src/sync/futex.hpp"
#include "../../src/sync/latch.hpp"
#include "../../src/sync/promises.hpp"
#include "../../src/sync/semaphore.hpp"
#include "../../src/thread/arena.hpp"
#include "../../src/thread/pool.hpp"

namespace
{

static_assert(sizeof(micron::auto_thread<>) < micron::auto_thread_stack_size, "auto_thread must not embed its worker stack");

micron::atomic_token<u64> g_n{ 0 };

void
body(void)
{
  g_n.fetch_add(1, micron::memory_order_relaxed);
}

void
body_arg(i64 v)
{
  g_n.fetch_add(static_cast<u64>(v), micron::memory_order_relaxed);
}

void
thread_types(void)
{
  {
    micron::thread<> t{ body };
    (void)t.alive();
    (void)t.thread_id();
    (void)t.can_join();
    (void)t.try_join();
    t.join();
  }
  {
    micron::auto_thread<> t{ body_arg, static_cast<i64>(1) };
    (void)t.alive();
    (void)t.try_join();
    t.join();
  }
  {

    micron::thread<> probe{ body };
    probe.join();
  }
  {
    micron::thread<> t{ body };
    t.dismiss();
  }
}

void
solo_api(void)
{
  auto t = micron::solo::spawn<micron::auto_thread<>>(body);
  (void)micron::solo::is_joinable(t);
  (void)micron::is_alive_ptr(t);
  micron::solo::try_join(t);
  micron::solo::join(t);

  auto u = micron::solo::spawn<micron::thread<>>(body_arg, static_cast<i64>(2));
  micron::solo::dismiss(u);

  auto v = micron::solo::spawn<micron::auto_thread<>>(body);
  micron::solo::sleep(v);
  micron::solo::awaken(v);
  micron::solo::terminate(v);
  micron::solo::dismiss(v);

  (void)micron::threads_available();
  (void)micron::cpu_count();
  micron::yield();
}

void
pools(void)
{
  micron::start_concurrent_pools();
  micron::async(body);
  micron::async(body_arg, static_cast<i64>(3));
  if ( micron::__global_threadpool != nullptr ) {
    (void)micron::__global_threadpool->count();
    micron::__global_threadpool->join_all(1);
  }
  micron::end_concurrent_pools();
}

micron::mutex g_m;
micron::fast_mutex g_fm;
micron::queuing_mutex g_qm;
micron::spin_lock g_sl;
micron::recursive_lock g_rl;

void
locks(void)
{
  {
    micron::lock_guard<micron::mutex> g{ g_m };
    (void)g;
  }
  {
    micron::auto_guard<micron::mutex> g{ g_m };
    (void)g;
  }
  {
    micron::unique_lock<micron::lock_starts::locked, micron::mutex> g{ g_m };
    (void)g;
  }
  {
    micron::scoped_lock g{ g_qm };
    (void)g;
  }
  g_fm.lock();
  g_fm.unlock();
  g_sl.lock();
  g_sl.unlock();
  g_rl.lock();
  g_rl.lock();
  g_rl.unlock();
  g_rl.unlock();

  full_barrier();
  read_barrier();
  write_barrier();
}

void
sync_prims(void)
{
  micron::latch l{ 1 };
  l.count_down();
  (void)l.try_wait();
  l.wait();

  micron::barrier b{ 1 };
  b.arrive_and_wait();

  micron::basic_semaphore s{ 1 };
  (void)s.try_wait();
  (void)s.value();
  s.reset(1);

  micron::event_count ec;
  const auto key = ec.prepare_wait();
  ec.cancel_wait();
  (void)key;
  ec.notify_one();
  ec.notify_all();

  micron::channel<i64> ch;
  ch >> static_cast<i64>(1);
  (void)!ch;

  micron::atomic_token<u32> word{ 0 };
  micron::wake_futex(word.ptr(), 1);

  {
    micron::promise<i64> p;
    auto f = p.get_future();
    p.set_value(4);
    (void)f.get();
  }
  {
    micron::futex_promise<i64> p;
    auto f = p.get_future();
    p.set_value(5);
    (void)f.get();
  }
  {

    micron::futex_promise<micron::vector<i64>> p;
    auto f = p.get_future();
    micron::vector<i64> v(2);
    p.set_value(micron::move(v));
    (void)f.get().size();
  }

  micron::do_once<+[]() { g_n.fetch_add(1, micron::memory_order_relaxed); }> once;
  (void)&once;
}

void
parallel_algos(void)
{
  micron::vector<i64> v(8);
  for ( usize i = 0; i < v.size(); ++i ) v[i] = static_cast<i64>(i);

  micron::parallel_for<[](micron::vector<i64>::iterator x) { *x += 1; }>(v);
  micron::fork_for<[](micron::vector<i64>::iterator x) { *x += 1; }>(v);
}

}      // namespace

int
main(void)
{
  thread_types();
  solo_api();
  pools();
  locks();
  sync_prims();
  parallel_algos();
  return 1;
}
