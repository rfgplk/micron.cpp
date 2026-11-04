//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/exit.hpp"

#if defined(__micron_freestanding)
namespace
{
int trace[32]{};
usize used = 0;

void
require(bool condition)
{
  if ( !condition ) micron::sys_group_exit(6);
}

void
emit(int tag)
{
  require(used < 32);
  trace[used++] = tag;
}

void
leaf()
{
  emit(30);
}

void
older()
{
  emit(100);
}

void
fini_tail()
{
  emit(42);
}

template<int N> void touch();

template<int N> struct probe {
  ~probe()
  {
    emit(10 + N);
    if constexpr ( N > 0 )
      require(micron::atexit(touch<N - 1>) == 0);
    else
      require(micron::atexit(leaf) == 0);
  }
};

template<int N>
void
touch()
{
  emit(20 + N);
  thread_local probe<N> p;
  (void)&p;
}

struct fini_probe {
  ~fini_probe()
  {
    emit(41);
    require(micron::atexit(fini_tail) == 0);
  }
};

void
release_storage() noexcept
{
  emit(50);
  micron::__dynamic_tls_exit_hook = nullptr;
}
}      // namespace

__attribute__((destructor)) void
fini()
{
  emit(40);
  thread_local fini_probe p;
  (void)&p;
}
#endif

int
main()
{
#if defined(__micron_freestanding)
  touch<3>();
  used = 0;
  require(micron::atexit(older) == 0);
  micron::__dynamic_tls_exit_hook = release_storage;
  micron::__run_exit_sequence();
  constexpr int expected[] = { 13, 22, 100, 12, 21, 11, 20, 10, 30, 40, 41, 42, 50 };
  require(used == sizeof(expected) / sizeof(expected[0]));
  for ( usize i = 0; i < used; ++i ) require(trace[i] == expected[i]);
  micron::__run_exit_sequence();
  require(used == sizeof(expected) / sizeof(expected[0]));
#else
  constexpr char message[] = "SKIP: exit_tls_late needs Micron's freestanding C++ TLS registry\n";
  micron::syscall(SYS_write, 1, message, sizeof(message) - 1);
#endif
  return 1;
}
