//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../../src/linux/std.hpp"
#include "../../src/mutex/spin_lock.hpp"
#include "../../src/std.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/sync/contract.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/vector/vector.hpp"

#include "../../src/attributes.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"
int global = 4;

mc::spin_lock sl;

volatile int __global = 0;

int
fn_loop(void)
{
  mc::vector<i32> vec;
  for ( i32 i = 0; i < 100000000; ++i ) {
    vec.push_back(i);
    __global = i;
    // mc::console(vec.back());
  }
  return 100;
}

int
fn(int a, int b, int c, int d, int e)
{
  mc::console("A should be: ", a, ", ", b, ", ", c, ", ", d, ", ", e);
  mc::console("Global is: ", static_cast<int>(__global));     // should be different each time
  return 4;
}

int
fn_objs(const micron::vector<int> &a)
{
  mc::console("Size is: ", a.size());
  return 2;
}
int
fn_ptrs_all(int **a, int **b, int **c)
{
  mc::console(a);
  mc::console(b);
  mc::console(c);
  return 7;
}
int
fn_ptrs(int **a, int **b, int c)
{
  mc::console(a);
  mc::console(b);
  mc::console(c);
  return 1;
}

int
fn_forever(void)
{
  for ( int i = 1;; ) {
    mc::console(i++);
    mc::sleep(500);
  }
  return 0;
}
int
fn_void(void)
{
  mc::console("Void!");
  return 1001;
}
#include "../../src/sync/when.hpp"

int
main(void)
{

  enable_scope()
  {
    // mc::when(t, 6);
    // mc::as_thread(fn, t);
    mc::auto_thread th(fn_loop);
    mc::solo::wait_for(th);
    mc::solo::join(th);
    mc::console(th.result<int>());
    mc::console(th.stats().ru_utime.tv_sec);
    mc::console(th.stats().ru_stime.tv_sec);
    mc::console(th.stats().ru_maxrss);
    mc::console("Done");
    return 0;
  }
  disable_scope()
  {
    int t = 6;
    // mc::when(t, 6);
    // mc::as_thread(fn, t);
    mc::thread th(fn, 5, 6, 7, 8, 9);
    mc::ssleep(1);
    mc::console("Parent is: ", mc::gettid());
    mc::ssleep(5);
    mc::console("Will unlock now!");
    sl.unlock();
    mc::sleep(1000);
    mc::console("Starting new thread");
    th[fn, 1, 2, 3, 4, 5];
    sl.unlock();
    th.try_join();
    return 0;
  }
  disable_scope()
  {
    mc::yield();
    auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5, 6, 7, 8, 9);
    mc::ssleep(1);
    // mc::console("Global is ", global);
    mc::solo::yield(t);
    mc::ssleep(2);
    // t->sleep();
    // mc::console("Thread Asleep!");
    // mc::ssleep(2);
    // t->awaken();
    mc::solo::join(t);
  }

  disable_scope()
  {
    mc::auto_thread th(fn_loop);
    mc::auto_thread th_2(fn_loop);
    mc::auto_thread th_3(fn, 1, 5, 10, 25, 50);
    mc::auto_thread th_4(fn_void);
    mc::auto_thread th_5(fn_loop);
    mc::ssleep(2);
    int res = th.result<int>();
    res = th_2.result<int>();
    res = th_3.result<int>();
    res = th_4.result<int>();
    res = th_5.result<int>();
    mc::console(res);
  }
  disable_scope()
  {
    mc::auto_thread th(fn, 5, 6, 7, 8, 9);
    // auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5,6,7,8,9);
  }

  disable_scope()
  {
    mc::auto_thread th(fn_forever);
    mc::ssleep(1);
    mc::solo::sleep(th);
    // auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5,6,7,8,9);
  }
  disable_scope()
  {
    mc::yield();
    mc::console("Global is ", global);
    auto t = mc::solo::spawn<mc::auto_thread<>>(fn, 5, 6, 7, 8, 9);
    mc::console("Spawned");
    mc::ssleep(1);
    mc::solo::yield(t);
    // mc::ssleep(2);
    t->sleep();
    mc::console("Thread Asleep!");
    mc::ssleep(2);
    t->awaken();
    mc::solo::join(t);
  }
  disable_scope()
  {
    int var = 5;
    int *a = &var;
    int **b = &a;
    micron::vector<int> vec;
    vec.resize(10);
    auto t = mc::solo::spawn<mc::thread<>>(fn_void);
    auto p = mc::solo::spawn<mc::thread<>>(fn, 1, 5, 6, 2, 15);
    auto d = mc::solo::spawn<mc::thread<>>(fn_ptrs, nullptr, b, 5);
    auto e = mc::solo::spawn<mc::thread<>>(fn_objs, vec);
    mc::solo::join(t, p, d, e);
    // mc::console("Joined with return code: ", mc::solo::join(t));
  }
  return 1;
}
