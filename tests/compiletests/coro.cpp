//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;

namespace
{

micron::task<i64>
leaf(i64 v)
{
  co_return v;
}

micron::task<void>
leaf_void(void)
{
  co_return;
}

micron::task<micron::vector<i64>>
leaf_vec(usize n)
{
  micron::vector<i64> v(n);
  co_return v;
}

micron::generator<i64>
gen(i64 n)
{
  for ( i64 i = 0; i < n; ++i ) co_yield i;
}

micron::task<i64>
shapes(i64 n)
{
  i64 a = 0;
  coro::eventual<i64> e;
  coro::eventual<micron::vector<i64>> ev;

  co_await coro::fork[&a, leaf](n);
  co_await coro::call(&e, leaf)(n);
  co_await coro::fork(&ev, leaf_vec)(usize(4));
  co_await coro::call(coro::discard, leaf)(n);
  const i64 b = co_await coro::call(leaf, n);
  co_await coro::join;

  auto vv = micron::move(ev).operator*();
  co_return a + b + micron::move(e).operator*() + static_cast<i64>(vv.size());
}

micron::task<i64>
spawns(usize n)
{
  auto xs = co_await coro::spawn_many(n, [](usize i) -> micron::task<i64> { co_return static_cast<i64>(i); });
  auto vs = co_await coro::spawn_many(n, [](usize i) -> micron::task<micron::vector<i64>> { co_return co_await leaf_vec(i); });
  auto t = co_await coro::when_all(leaf(1), leaf_vec(2));
  auto t2 = co_await coro::spawn_tuple(leaf(3), leaf(4));

  i64 slot = 0;
  co_await coro::fork_task(&slot, leaf(5));
  co_await coro::join;

  co_return static_cast<i64>(xs.size() + vs.size() + micron::get<1>(t).size()) + micron::get<0>(t2) + slot;
}

coro::async_mutex g_m;
coro::async_rwlock g_rw;
coro::manual_reset_event g_ev;
coro::async_semaphore g_sem{ 1 };
coro::async_condvar g_cv;

micron::task<i64>
prims(void)
{
  co_await g_m.lock();
  (void)g_m.try_lock();
  g_m.unlock();
  {
    auto g = co_await coro::scoped_lock(g_m);
    (void)g.owns_lock();
  }
  co_await g_rw.lock_shared();
  g_rw.unlock_shared();
  co_await g_rw.lock();
  g_rw.unlock();

  g_ev.set();
  co_await g_ev;
  (void)g_ev.is_set();
  g_ev.reset();

  co_await g_sem.acquire();
  (void)g_sem.try_acquire();
  (void)g_sem.try_acquire_many(2);
  g_sem.release(1);
  g_sem.abort();

  coro::async_latch latch{ 1 };
  latch.count_down();
  (void)latch.try_wait();
  co_await latch;

  coro::async_barrier bar{ 1 };
  co_await bar.arrive_and_wait();

  g_cv.notify_one();
  g_cv.notify_all();
  co_return 0;
}

micron::task<i64>
cancels(void)
{
  coro::cancellation_source src;
  auto tok = src.token();
  (void)tok.cancelled();
  (void)static_cast<bool>(tok);
  co_await coro::cancelpoint();
  src.cancel();
  (void)src.cancelled();

  coro::fork_group grp;
  grp.spawn(leaf, static_cast<i64>(1));
  (void)grp.outstanding();
  grp.request_cancel();
  (void)grp.cancelled();
  co_await grp.join();
  co_return 0;
}

micron::task<i64>
channels(void)
{
  i64 out = 0;
  micron::async_channel<i64> ac;
  co_await ac.push(1);
  (void)co_await ac.pull();
  (void)ac.try_pull(out);
  ac.close();
  (void)ac.is_closed();

  micron::unbounded_channel<i64> uc;
  uc.push(2);
  (void)uc.try_pull(out);
  uc.close();

  micron::sync_channel<i64> sc;
  (void)sc.try_push(3);
  (void)sc.try_pull(out);
  sc.close();
  co_return 0;
}

micron::task<i64>
timers(void)
{
  co_await coro::sleep_for_ms(0);
  co_await coro::reschedule();
  co_await coro::reschedule_fair();
  co_return 0;
}

micron::task<max_t>
io_surface(void)
{
  (void)cio::available();

  cio::fd_io s{ 0 };
  char buf[8]{};
  (void)co_await s.read_some(buf, sizeof(buf));
  (void)co_await s.write_some(buf, sizeof(buf));
  (void)co_await s.read(buf, sizeof(buf));
  (void)co_await s.write(buf, sizeof(buf));

  auto f = co_await cio::open_file(micron::io::path_t{ "/dev/null" }, micron::io::modes::read);
  if ( f.valid() ) {
    (void)co_await f.read_some(buf, sizeof(buf));
    (void)co_await f.write_some(buf, sizeof(buf));
    (void)co_await f.sync();
    (void)co_await f.each_line([](const micron::string &) { });
    f.close();
  }

  (void)co_await cio::read_file<micron::string>(micron::io::path_t{ "/dev/null" });
  (void)co_await cio::exists(micron::io::path_t{ "/dev/null" });
  (void)co_await cio::splice(0, 1, 0);
  (void)co_await cio::write_out(buf, 0);
  (void)co_await cio::write_err(buf, 0);

  (void)co_await coro::io::nop();
  (void)co_await coro::io::read(0, buf, sizeof(buf));
  (void)co_await coro::io::write(1, buf, 0);
  (void)co_await coro::io::poll(0, micron::uring::poll_in);
  (void)(co_await (coro::io::nop() | coro::io::after(1000)));

  (void)cio::wave::available();
  cio::wave w;
  w.begin(-100);
  (void)w.push("x");
  (void)w.full();
  (void)w.size();
  (void)co_await w.run();
  w.clear();
  co_return 0;
}

void
routines(void)
{
  auto r = coro::spin<i64>(
      [](i64 v) -> i64 {
        coro::yield();
        return v;
      },
      static_cast<i64>(1));
  (void)r.started();
  (void)r.alive();
  (void)r.done();
  (void)r.stack_size();
  coro::jump(r);
  coro::dismiss(r);
  (void)coro::in_routine();
}

void
entry(void)
{
  coro::start_coroutine_runtime(1);
  (void)coro::sync_wait(shapes(1));
  (void)coro::sync_wait(spawns(2));
  (void)coro::sync_wait(prims());
  (void)coro::sync_wait(cancels());
  (void)coro::sync_wait(channels());
  (void)coro::sync_wait(timers());
  (void)coro::sync_wait(io_surface());
  coro::detach(leaf_void());
  {
    auto fut = coro::schedule(leaf(1));
    (void)fut.get();
    auto vfut = coro::schedule(leaf_vec(1));
    (void)vfut.get();
  }
  for ( auto v : gen(2) ) (void)v;
  routines();
  (void)coro::io_pending();
  coro::stop_coroutine_runtime();
}

}      // namespace

int
main(void)
{
  entry();
  return 1;
}
