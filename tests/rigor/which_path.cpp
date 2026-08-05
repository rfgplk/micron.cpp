//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: which_into()'s returned length.
//
// __which_join inserts the '/' separator CONDITIONALLY -- a PATH element that already ends in one
// does not get a second -- but the caller returned an unconditional dlen + 1 + clen. For any
// trailing-slash PATH component (legal, and common in shell rc files) that is one byte too many, so
// a caller that trusts the length gets the NUL plus a byte of whatever followed it: an exec that
// fails with ENOENT, or launches with a corrupted path.
//
// The invariant is simply that the returned length is the length of the string that was written.

#include "../../src/linux/process/which.hpp"
#include "../../src/string/strings.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

constexpr const char *DIR = "/var/tmp/micron_which_t";
constexpr const char *TOOL = "micron_which_tool";

usize
slen(const char *p) noexcept
{
  usize n = 0;
  while ( p[n] != 0 ) ++n;
  return n;
}

void
mkdir_p(const char *p) noexcept
{
  micron::syscall(SYS_mkdirat, -100, p, 0755);
}

bool
make_tool() noexcept
{
  mkdir_p(DIR);
  char full[256];
  usize k = 0;
  for ( const char *p = DIR; *p; ++p ) full[k++] = *p;
  full[k++] = '/';
  for ( const char *p = TOOL; *p; ++p ) full[k++] = *p;
  full[k] = 0;
  const long fd = micron::syscall(SYS_openat, -100, full, micron::posix::o_create | micron::posix::o_wronly | micron::posix::o_trunc, 0755);
  if ( fd < 0 ) return false;
  micron::syscall(SYS_write, fd, "#!/bin/sh\n", 10);
  micron::syscall(SYS_close, fd);
  micron::syscall(SYS_fchmodat, -100, full, 0755, 0);
  return micron::posix::is_executable(full);
}

// which_into reads PATH out of environ, so hand it one of ours
char *g_env[2] = { nullptr, nullptr };
char g_path[512]{};
char **g_saved = nullptr;

void
set_path(const char *value) noexcept
{
  usize k = 0;
  for ( const char *p = "PATH="; *p; ++p ) g_path[k++] = *p;
  for ( const char *p = value; *p; ++p ) g_path[k++] = *p;
  g_path[k] = 0;
  g_env[0] = g_path;
  g_env[1] = nullptr;
  environ = g_env;
}

// the whole property, in one line: what came back is the length of what was written
bool
resolves(const char *path_value, const char *cmd, char *out, usize cap, max_t &ret) noexcept
{
  set_path(path_value);
  ret = micron::which_into(cmd, out, cap);
  return ret >= 0 && static_cast<usize>(ret) == slen(out);
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== WHICH_INTO PATH JOIN RIGOR ===");

  g_saved = environ;
  require(make_tool());

  char out[512]{};
  max_t r = 0;

  test_case("a PATH element with no trailing slash reports its own length");
  {
    require(resolves(DIR, TOOL, out, sizeof(out), r));
    sb::print("  ", out, " -> ", r);
  }
  end_test_case();

  test_case("a PATH element WITH a trailing slash reports its own length");
  {
    char with[256];
    usize k = 0;
    for ( const char *p = DIR; *p; ++p ) with[k++] = *p;
    with[k++] = '/';
    with[k] = 0;
    require(resolves(with, TOOL, out, sizeof(out), r));
    // the separator was not inserted, so the joined length is one SHORTER than dlen + 1 + clen
    require(static_cast<usize>(r) == slen(DIR) + 1 + slen(TOOL));
    require(out[r] == 0);
    sb::print("  ", out, " -> ", r);
  }
  end_test_case();

  test_case("a doubled trailing slash still reports its own length");
  {
    char with[256];
    usize k = 0;
    for ( const char *p = DIR; *p; ++p ) with[k++] = *p;
    with[k++] = '/';
    with[k++] = '/';
    with[k] = 0;
    require(resolves(with, TOOL, out, sizeof(out), r));
    require(out[r] == 0);
  }
  end_test_case();

  test_case("the trailing-slash element resolves the same wherever it sits in PATH");
  {
    char both[512];
    usize k = 0;
    for ( const char *p = "/nonexistent_micron_dir:"; *p; ++p ) both[k++] = *p;
    for ( const char *p = DIR; *p; ++p ) both[k++] = *p;
    both[k++] = '/';
    for ( const char *p = ":/usr/bin"; *p; ++p ) both[k++] = *p;
    both[k] = 0;
    require(resolves(both, TOOL, out, sizeof(out), r));
    require(out[r] == 0);
  }
  end_test_case();

  test_case("an explicit path bypasses PATH and still reports its own length");
  {
    char full[256];
    usize k = 0;
    for ( const char *p = DIR; *p; ++p ) full[k++] = *p;
    full[k++] = '/';
    for ( const char *p = TOOL; *p; ++p ) full[k++] = *p;
    full[k] = 0;
    set_path("/nowhere");
    r = micron::which_into(full, out, sizeof(out));
    require(r >= 0);
    require(static_cast<usize>(r) == slen(out));
  }
  end_test_case();

  test_case("failures stay failures");
  {
    set_path(DIR);
    require(micron::which_into("micron_definitely_not_a_tool_xyzzy", out, sizeof(out)) == -micron::error::no_entry);
    require(out[0] == 0);
    char tiny[4]{};
    require(micron::which_into(TOOL, tiny, sizeof(tiny)) == -micron::error::name_too_long);
    require(micron::which_into(nullptr, out, sizeof(out)) == -micron::error::invalid_arg);
    require(micron::which_into("", out, sizeof(out)) == -micron::error::invalid_arg);
    require(micron::which_into(TOOL, out, 0) == -micron::error::invalid_arg);
  }
  end_test_case();

  environ = g_saved;
  sb::print("=== WHICH_INTO PATH JOIN PASSED ===");
  return 1;
}
