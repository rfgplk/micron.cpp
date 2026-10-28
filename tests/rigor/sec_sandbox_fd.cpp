// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sec/sandbox.hpp"
#include "../../src/string/sstring.hpp"
#include "../snowball/snowball.hpp"

namespace
{
const char *inside = nullptr, *outside = nullptr;
int borrowed = -1;
bool keep = false;

int
probe()
{
  using namespace micron::posix;
  const int fd = open(inside, o_rdonly | o_cloexec, 0);
  if ( fd < 0 ) return 77;
  close(fd);
  if ( open(outside, o_rdonly | o_cloexec, 0) >= 0 ) return 78;
  const bool inherited = fcntl(borrowed, f_getfd, 0) >= 0;
  return inherited == keep ? 31 : 79;
}

void
make_file(const char *path)
{
  using namespace micron::posix;
  const int fd = open(path, o_wronly | o_create | o_excl | o_cloexec, 0600);
  sb::require_true(fd >= 0);
  sb::require_true(write(fd, "x", 1) == 1 && close(fd) == 0);
}
}      // namespace

int
main()
{
  using namespace micron;
  using namespace micron::posix;
  using namespace micron::sec;
  sstring<128> base("/tmp/micron-sandbox-fd-");
  base.try_append_int(getpid());
  auto original = base;
  original.try_append("/original");
  auto renamed = base;
  renamed.try_append("/renamed");
  auto file = renamed;
  file.try_append("/file");
  auto denied = original;
  denied.try_append("/file");
  sb::test_case("pinned Landlock directories survive path replacement and leave no setup descriptor");
  sb::require_true(landlock::available());
  sb::require_true(mkdir(base.data(), 0700) == 0 && mkdir(original.data(), 0700) == 0);
  const int root = open(original.data(), o_path | o_directory | o_cloexec, 0);
  sb::require_true(root >= 3 && dup2(root, 57) == 57);
  close(root);
  borrowed = 57;
  sandbox s;
  s.landlock_fd(borrowed, landlock::read_only).landlock_strict();
  sb::require_true(rename(original.data(), renamed.data()) == 0 && mkdir(original.data(), 0700) == 0);
  make_file(file.data());
  make_file(denied.data());
  inside = file.data();
  outside = denied.data();
  auto child = s.run(probe);
  int result = 0;
  if ( child.fault ) micron::console("sandbox stage ", u32(child.fault.where), " errno ", child.fault.err);
  sb::require_true(child.ok() && waitpid(child.pid, &result, 0) == child.pid && wifexited(result) && wexitstatus(result) == 31);
  sb::require_true(fcntl(borrowed, f_getfd, 0) >= 0);
  keep = true;
  s.keep_dir_fd(borrowed);
  child = s.run(probe);
  sb::require_true(child.ok() && waitpid(child.pid, &result, 0) == child.pid && wifexited(result) && wexitstatus(result) == 31);
  close(borrowed);
  sb::end_test_case();

  sb::test_case("invalid descriptors and unsupported required Landlock rights cannot reach the body");
  sandbox bad;
  bad.landlock_fd(-1, landlock::read_only);
  child = bad.run(probe);
  sb::require_true(child.pid < 0 && child.fault.where == stage::landlock);
  sandbox strict;
  strict.landlock_handled(static_cast<landlock::access_fs>(u64{ 1 } << 63)).landlock_strict();
  child = strict.run(probe);
  sb::require_true(child.pid < 0 && child.fault.where == stage::landlock && child.fault.err == -error::op_not_supported);
  sb::end_test_case();
  unlink(file.data());
  unlink(denied.data());
  rmdir(original.data());
  rmdir(renamed.data());
  rmdir(base.data());
  return 1;
}
