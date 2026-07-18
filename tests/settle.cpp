//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// behavioural suite for the auto-await layer (src/settle_fwd.hpp): io::echo / echof / print
// drive coroutines, futures and threads to completion before printing them

#include "../src/io/echo.hpp"
#include "../src/io/pecho.hpp"

#include "../src/memory/pointers/unique.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/sync/future.hpp"
#include "../src/sync/promises.hpp"
#include "../src/tasks/coroutine/routine.hpp"
#include "../src/thread/threads.hpp"
#include "../src/vector/vector.hpp"

namespace mc = micron;
using namespace micron;

static int fails = 0;

static void
report(const char *what, const mc::string &got, const char *want)
{
  ++fails;
  io::error("FAIL ", what, ": got \"", got, "\" want \"", want, "\"\n");
}

template<typename Fn>
static mc::string
capture(Fn &&fn)
{
  i32 fds[2]{};
  posix::pipe2(fds, 0);
  fd_t w{ fds[1] };
  fn(w);
  posix::close(fds[1]);
  mc::string out;
  char buf[8192];
  for ( ;; ) {
    max_t r = posix::read(fds[0], buf, sizeof(buf));
    if ( r <= 0 ) break;
    for ( max_t i = 0; i < r; ++i ) out.push_back(buf[i]);
  }
  posix::close(fds[0]);
  return out;
}

static void
eq(const char *what, const mc::string &got, const char *want)
{
  mc::string expect(want);
  expect.push_back('\n');
  if ( !(got == expect) ) report(what, got, want);
}

static int
co_add(int a, int b)
{
  mc::coro::yield();
  return a + b;
}

int
main(void)
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // coroutines

  {
    // the headline case: echo finishes the routine, then prints result()
    auto r = mc::coro::spin(co_add, 40, 2);
    eq("routine<Y, int>", capture([&](fd_t w) { io::echo(w, r); }), "42");
  }
  {
    auto r = mc::coro::spin([] { mc::coro::yield(); });
    eq("routine<Y, void>", capture([&](fd_t w) { io::echo(w, r); }), "<routine done>");
  }
  {
    // a routine yielding several times must be driven all the way, not one jump
    auto r = mc::coro::spin([] {
      for ( int i = 0; i < 5; ++i ) mc::coro::yield();
      return 9;
    });
    eq("multi-yield routine", capture([&](fd_t w) { io::echo(w, r); }), "9");
  }
  {
    auto r = mc::coro::spin([] { return mc::string("hi"); });
    eq("non-trivial result", capture([&](fd_t w) { io::echo(w, r); }), "hi");
  }
  {
    // a result that is ITSELF printable goes through the container renderer
    auto r = mc::coro::spin([] {
      mc::vector<i32> v;
      v.push_back(1);
      v.push_back(2);
      return v;
    });
    eq("container result", capture([&](fd_t w) { io::echo(w, r); }), "{ 1, 2 }");
  }
  {
    // a MOVE-ONLY result must survive __then's const & rebinding without being copied
    auto r = mc::coro::spin([] { return mc::unique_pointer<i32>(99); });
    eq("move-only result", capture([&](fd_t w) { io::echo(w, r); }), "99");
  }
  {
    // settling and non-settling arguments mix, in any position
    auto r = mc::coro::spin([] { return 7; });
    eq("mixed args", capture([&](fd_t w) { io::echo(w, "n=", r, " ok"); }), "n=7 ok");
  }
  {
    // two awaitables in one call settle left to right
    auto a = mc::coro::spin([] { return 1; });
    auto b = mc::coro::spin([] { return 2; });
    eq("two awaitables", capture([&](fd_t w) { io::echo(w, a, ",", b); }), "1,2");
  }
  {
    // result() is single-shot; settling at the ENTRY POINT means a repeated {0} formats the
    // already-settled value instead of reading the routine twice
    auto r = mc::coro::spin([] { return 5; });
    eq("echof settles once, formats twice", capture([&](fd_t w) { io::echof(w, "{0}+{0}={1}", r, 10); }), "5+5=10");
  }
  {
    // a format string with no placeholder must still drive the routine
    bool ran = false;
    auto r = mc::coro::spin([&] {
      ran = true;
      return 1;
    });
    (void)capture([&](fd_t w) { io::echof(w, "none", r); });
    if ( !ran ) report("echof with no placeholder still drives", mc::string("not run"), "run");
  }
  {
    auto r = mc::coro::spin([] { return 3; });
    auto s = mc::format::format("v={}", r);
    if ( !(s == mc::hstring<schar>("v=3")) ) report("format() settles", mc::string(s.c_str()), "v=3");
  }
  {
    auto r = mc::coro::spin([] { return 4; });
    eq("echon settles", capture([&](fd_t w) { io::echon(w, r); }) + mc::string("\n"), "4");
  }
  {
    // exercises formatter<settle_note> - the format engine's half of the void-result path,
    // which the printk overload does not cover
    auto r = mc::coro::spin([] { mc::coro::yield(); });
    eq("echof on a void routine", capture([&](fd_t w) { io::echof(w, "[{}]", r); }), "[<routine done>]");
  }
  {
    // and with a width spec, to prove padding is applied to the note like any other value
    auto r = mc::coro::spin([] { });
    eq("echof pads a settle_note", capture([&](fd_t w) { io::echof(w, "{:>16}", r); }), "  <routine done>");
  }
  {
    auto r = mc::coro::spin([] { });
    auto s = mc::format::format("{}", r);
    if ( !(s == mc::hstring<schar>("<routine done>")) ) report("format() void routine", mc::string(s.c_str()), "<routine done>");
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // futures

  {
    mc::promise<int> p;
    mc::future<int> f = p.get_future();
    p.set_value(11);
    eq("future<int>", capture([&](fd_t w) { io::echo(w, f); }), "11");
  }
  {
    mc::promise<void> p;
    mc::future<void> f = p.get_future();
    p.set_value();
    eq("future<void>", capture([&](fd_t w) { io::echo(w, f); }), "<future ready>");
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // threads
  //
  // a thread's result is type-erased at runtime, so the bare handle prints a status line and
  // the value needs an explicit as<R>

  {
    auto_thread<> t{ [] { } };
    auto got = capture([&](fd_t w) { io::echo(w, t); });
    // "<thread N joined>\n" - N is a live tid, so only the shape is assertable
    bool ok = got.size() > sizeof("<thread  joined>") && got[0] == '<';
    for ( usize i = 0; i < sizeof("<thread ") - 1 && ok; ++i ) ok = got[i] == "<thread "[i];
    if ( !ok ) report("thread status line", got, "<thread N joined>");
  }
  {
    // as<R> reads result<R>() before joining, then joins
    auto_thread<> t{ [] { return 21; } };
    eq("as<int>(thread)", capture([&](fd_t w) { io::echo(w, mc::as<int>(t)); }), "21");
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // pecho re-entrancy
  //
  // settling happens BEFORE __plock is taken. finish() runs the routine body on THIS thread,
  // so a routine that prints from inside itself would deadlock on the non-recursive lock if
  // the settle happened under it. reaching the line after this is the assertion.

  {
    auto r = mc::coro::spin([] {
      io::pecho("  (printed from inside the routine)");
      return 1;
    });
    max_t n = io::pecho(r);
    if ( n <= 0 ) report("pecho settles outside its lock", mc::string("deadlock or error"), ">0");
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // non-settling paths must be untouched

  {
    mc::vector<i32> v;
    v.push_back(1);
    eq("plain container still prints", capture([&](fd_t w) { io::echo(w, v); }), "{ 1 }");
  }
  {
    eq("plain scalars still print", capture([&](fd_t w) { io::echo(w, "x=", 42); }), "x=42");
  }
  {
    // an fd_t in NON-leading position is still data, even alongside a settled value
    auto r = mc::coro::spin([] { return 1; });
    eq("fd_t stays data next to a settle", capture([&](fd_t w) { io::echo(w, r, " ", fd_t{ 7 }); }), "1 7");
  }

  if ( fails == 0 ) {
    posix::write(1, "ALL PASS\n", 9);
    return 1;      // duck's success sentinel is 1; 6 is failure
  }
  return 6;
}
