//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <micron/io/console.hpp>
#include <micron/linux/process/environ.hpp>
#include <micron/linux/process/spawn_basic.hpp>
#include <micron/syscall.hpp>

namespace
{
struct rlim_pair {
  unsigned long cur, max;
};

constexpr int rlimit_nproc = 6;
constexpr int eagain = 11;
}      // namespace

int
main(void)
{

  {
    micron::pid_t pid = 0;
    char *const argv[] = { (char *)"/bin/true", nullptr };
    const int e = micron::spawn(pid, "/bin/true", argv, environ);
    if ( e != 0 ) {
      micron::io::echof("control spawn failed with errno {}; cannot run this test\n", e);
      return 0;
    }
    int st = 0;
    micron::waitpid(static_cast<int>(pid), &st, 0);
  }

  rlim_pair rl{ 0, 0 };
  if ( micron::syscall(SYS_prlimit64, 0, rlimit_nproc, &rl, nullptr) != 0 ) {
    micron::io::echof("could not lower RLIMIT_NPROC; cannot run this test\n");
    return 0;
  }

  micron::pid_t pid = 0;
  char *const argv[] = { (char *)"/bin/true", nullptr };
  const int e = micron::spawn(pid, "/bin/true", argv, environ);
  if ( e != eagain ) {
    micron::io::echof("spawn returned {}, expected a positive EAGAIN ({})\n", e, eagain);
    return 0;
  }

  const int f = micron::try_fork();
  if ( f != -eagain ) {
    micron::io::echof("try_fork returned {}, expected -EAGAIN ({})\n", f, -eagain);
    return 0;
  }

  micron::io::echof("refused clone reported by both, process survived\n");
  return 1;
}
