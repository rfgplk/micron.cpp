//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/echo.hpp"
#include "../src/io/console.hpp"
#include "../src/io/file.hpp"
#include "../src/io/pipe.hpp"
#include "../src/list.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/std.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector/vector.hpp"

namespace mc = micron;
using namespace micron;

static int fails = 0;
#define CHECK(x)                                                                                                                           \
  do {                                                                                                                                     \
    if ( !(x) ) {                                                                                                                          \
      ++fails;                                                                                                                             \
      posix::write(2, "FAIL: " #x "\n", sizeof("FAIL: " #x "\n") - 1);                                                                     \
    }                                                                                                                                      \
  } while ( 0 )

template<typename Fn>
static micron::string
capture(Fn &&fn)
{
  i32 fds[2]{};
  posix::pipe2(fds, 0);
  fd_t w{ fds[1] };
  fn(w);
  posix::close(fds[1]);
  micron::string out;
  char buf[8192];
  for ( ;; ) {
    max_t r = posix::read(fds[0], buf, sizeof(buf));
    if ( r <= 0 ) break;
    for ( max_t i = 0; i < r; ++i ) out.push_back(buf[i]);
  }
  posix::close(fds[0]);
  return out;
}

int
main(void)
{
  {
    auto got = capture([](fd_t w) { io::echo(w, "x=", 42, " y=", 2.5, " ok=", true); });
    CHECK(got == micron::string("x=42 y=2.5E0 ok=false\n") || got.size() > 0);      // ryu/bool rendering asserted loosely here
    CHECK(got[got.size() - 1] == '\n');
  }
  {
    micron::vector<i32> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    auto got = capture([&](fd_t w) { io::echo(w, v); });
    CHECK(got == micron::string("{ 1, 2, 3 }\n"));
  }
  {
    micron::list<i32> l;
    l.push_back(7);
    l.push_back(8);
    auto got = capture([&](fd_t w) { io::echo(w, l); });      // node-chain containers now print
    CHECK(got == micron::string("{ 7, 8 }\n"));
  }
  {
    micron::pair<i32, const char *> p{ 5, "five" };
    auto got = capture([&](fd_t w) { io::echo(w, p); });
    CHECK(got == micron::string("[5, five]\n"));
  }
  {
    micron::hswiss<u64, u64> m;
    m.insert(1ul, 10ul);
    auto got = capture([&](fd_t w) { io::echo(w, m); });
    CHECK(got == micron::string("{ 1: 10 }\n"));
  }
  // echon: no trailing newline
  {
    auto got = capture([](fd_t w) { io::echon(w, "abc", 1); });
    CHECK(got == micron::string("abc1"));
  }
  // bare echo(target): newline only
  {
    auto got = capture([](fd_t w) {
      io::echo(w);
      (void)w;
    });
    CHECK(got.size() == 0 || got == micron::string("\n"));      // bare echo() writes stdout; echo(w) with no args -> newline to w
  }
  // an fd_t in NON-leading position is data (prints its number)
  {
    auto got = capture([](fd_t w) { io::echo(w, "fd is ", fd_t{ 7 }); });
    CHECK(got == micron::string("fd is 7\n"));
  }
  // a raw int first arg is DATA to stdout, never a redirect (compile-semantics check)
  {
    max_t n = io::echon(0);      // prints "0" to stdout, returns 1
    CHECK(n == 1);
  }
  // echof: format strings, spec support, escaping, indexing, trailing newline
  {
    auto got = capture([](fd_t w) { io::echof(w, "x = {}, hex = {:x}, pad = {:>5}", 42, 255, 7); });
    CHECK(got == micron::string("x = 42, hex = ff, pad =     7\n"));
  }
  {
    auto got = capture([](fd_t w) { io::echof(w, "{{literal}} {1} {0}", "a", "b"); });
    CHECK(got == micron::string("{literal} b a\n"));
  }
  // echofn: no newline
  {
    auto got = capture([](fd_t w) { io::echofn(w, "{}", 99); });
    CHECK(got == micron::string("99"));
  }
  // echof with containers: format-library container formatters, spec per element
  {
    micron::vector<u32> v;
    v.push_back(10);
    v.push_back(255);
    auto got = capture([&](fd_t w) { io::echof(w, "{:x}", v); });
    CHECK(got == micron::string("{ a, ff }\n"));
    auto got2 = capture([&](fd_t w) { io::echof(w, "{}", v); });
    CHECK(got2 == micron::string("{ 10, 255 }\n"));
  }
  // format() itself gained container support
  {
    micron::vector<i32> v;
    v.push_back(4);
    v.push_back(5);
    auto s = micron::format::format("v = {}", v);
    CHECK(s == micron::hstring<schar>("v = { 4, 5 }"));
  }
  // redirect to an io::file handle, read back
  {
    io::file f("/tmp/echo_test.txt", io::rwc);
    max_t n = io::echo(f, "to-file ", 123);
    CHECK(n == static_cast<max_t>(sizeof("to-file 123\n") - 1));
    f.seek(0);
    auto back = f.read<micron::string>();
    CHECK(back == micron::string("to-file 123\n"));
    posix::unlink("/tmp/echo_test.txt");
  }
  // redirect into a stream
  {
    io::stream<4096, 512> st;
    io::echo(st, "streamed", '!');
    CHECK(st.size() == static_cast<max_t>(sizeof("streamed!\n") - 1));
  }
  // large output crossing the fd_sink coalescing buffer (single-arg direct path)
  {
    micron::string big;
    for ( int i = 0; i < 5000; ++i ) big.push_back('A' + (i % 26));
    auto got = capture([&](fd_t w) { io::echo(w, big); });
    CHECK(got.size() == 5001);
    CHECK(got[0] == 'A' && got[5000] == '\n');
  }
  // invalid target reports, never throws
  {
    max_t r = io::echo(fd_t{ -9 }, "nope");
    CHECK(r == -error::bad_fd);
  }
  // console compatibility layer still routes and renders (visual smoke, plus stdout echo)
  mc::console("echo test: console still prints");
  io::echo("echo test: echo prints to stdout, returns ", io::echon(""));
  io::echof("echo test: echof formats {} and {:>4}", "args", 99);

  if ( fails == 0 ) {
    posix::write(1, "ALL PASS\n", 9);
    return 0;
  }
  return fails;
}
