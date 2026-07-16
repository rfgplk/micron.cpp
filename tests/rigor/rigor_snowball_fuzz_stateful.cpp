//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball_fuzz.hpp"

namespace sbf = snowball::fuzzing;

namespace demo
{
inline bool g_open[8] = {};
inline bool g_ever[8] = {};

inline int
open_file()
{
  for ( int i = 0; i < 8; ++i )
    if ( !g_open[i] ) {
      g_open[i] = true;
      g_ever[i] = true;
      return i;
    }
  return -1;
}

inline void
write_file(int fd, int n)
{
  (void)n;
  if ( fd < 0 || fd >= 8 ) return;
  if ( g_ever[fd] && !g_open[fd] ) throw "use-after-close";
}

inline void
close_file(int fd)
{
  if ( fd >= 0 && fd < 8 ) g_open[fd] = false;
}
}      // namespace demo

int
main()
{
  sbf::scenario s;
  auto fd = s.resource<int>("fd");
  s.call("open", +[]() -> int { return demo::open_file(); }).produces(fd);
  s.call("write", +[](int f, int n) { demo::write_file(f, n); }).consumes(fd).arg(sbf::range<int>(0, 100));
  s.call("close", +[](int f) { demo::close_file(f); }).consumes(fd);

  auto rep = s.run({ .seed = 0xC0FFEE, .iterations = 4000, .max_calls = 12, .minimize = true, .abort_on_failure = false });
  if ( !rep.found_failure ) {
    snowball::error("expected to find the use-after-close bug");
    return 1;
  }
  snowball::print("rigor_snowball_fuzz_stateful: found + minimized use-after-close");
  return rep.minimized.calls.size() <= 3 ? 0 : 1;
}
