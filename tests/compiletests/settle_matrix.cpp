//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/echo.hpp"
#include "../../src/io/file.hpp"
#include "../../src/io/pecho.hpp"

#include "../../src/std.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/sync/future.hpp"
#include "../../src/sync/promises.hpp"
#include "../../src/tasks/coroutine/routine.hpp"
#include "../../src/thread/threads.hpp"

namespace mc = micron;
using namespace micron;

static auto
spun(void)
{
  return mc::coro::spin([] { return 5; });
}

int
main(void)
{
  mc::io::file dev("/dev/null", mc::io::rwc);
  fd_t fd = dev.fd();
  mc::io::stream<4096, 512> st;

  mc::io::echo("a", 1);
  mc::io::echon("a", 1);
  mc::io::echof("{}", 1);
  mc::io::echofn("{}", 1);
  mc::io::print("a", 1);
  mc::io::printn("a", 1);
  mc::io::println("a");
  mc::io::error("a");
  mc::io::errorn("a");
  mc::io::errorln("a");
  (void)mc::format::format("{}", 1);

  {
    auto r = spun();
    mc::io::echo(r);
  }
  {
    auto r = spun();
    mc::io::echon(r);
  }
  {
    auto r = spun();
    mc::io::echof("{}", r);
  }
  {
    auto r = spun();
    mc::io::echofn("{}", r);
  }
  {
    auto r = spun();
    mc::io::print(r);
  }
  {
    auto r = spun();
    mc::io::printn(r);
  }
  {
    auto r = spun();
    mc::io::println(r);
  }
  {
    auto r = spun();
    mc::io::error(r);
  }
  {
    auto r = spun();
    mc::io::errorn(r);
  }
  {
    auto r = spun();
    mc::io::errorln(r);
  }
  {
    auto r = spun();
    (void)mc::format::format("{}", r);
  }

  {
    auto r = spun();
    mc::io::echo(fd, r);
  }
  {
    auto r = spun();
    mc::io::echon(fd, r);
  }
  {
    auto r = spun();
    mc::io::echof(fd, "{}", r);
  }
  {
    auto r = spun();
    mc::io::echofn(fd, "{}", r);
  }

  {
    auto r = spun();
    mc::io::echo(dev, r);
  }
  {
    auto r = spun();
    mc::io::echofn(dev, "{}", r);
  }
  {
    auto r = spun();
    mc::io::echo(st, r);
  }
  {
    auto r = spun();
    mc::io::echof(st, "{}", r);
  }

  {
    auto r = spun();
    mc::io::echo("n=", r, " ", 1);
  }
  {
    auto r = spun();
    mc::io::echof(fd, "{} {} {}", 1, r, "x");
  }

  {
    auto r = spun();
    (void)mc::io::pecho(r);
  }
  {
    auto r = spun();
    (void)mc::io::pechon(r);
  }
  {
    auto r = spun();
    (void)mc::io::pechof("{}", r);
  }
  {
    auto r = spun();
    (void)mc::io::pechofn("{}", r);
  }
  {
    auto r = spun();
    (void)mc::io::pprint(r);
  }
  {
    auto r = spun();
    (void)mc::io::pprintln(r);
  }
  {
    auto r = spun();
    (void)mc::io::pprintn(r);
  }
  (void)mc::io::pecho("plain");
  (void)mc::io::pechof("{}", 1);

  {
    mc::promise<int> p;
    mc::future<int> f = p.get_future();
    p.set_value(1);
    mc::io::echo(f);
  }
  {
    mc::promise<void> p;
    mc::future<void> f = p.get_future();
    p.set_value();
    mc::io::echof("{}", f);
  }
  {
    auto_thread<> t{ [] { } };
    mc::io::echo(t);
  }
  {
    auto_thread<> t{ [] { return 1; } };
    mc::io::echo(mc::as<int>(t));
  }
  {
    auto r = mc::coro::spin([] { mc::coro::yield(); });      // void result
    mc::io::echo(r);
  }
  return 1;      // duck success sentinel
}
