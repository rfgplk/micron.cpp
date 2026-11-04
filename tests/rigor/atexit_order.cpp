//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/exit.hpp"

namespace
{
int trace[128]{};
usize used = 0;
int fini_calls = 0;
int owner_a, owner_b;

void
require(bool condition)
{
  if ( !condition ) micron::sys_group_exit(6);
}

void
emit(void *arg)
{
  require(used < 128);
  trace[used++] = static_cast<int>(reinterpret_cast<uintptr_t>(arg));
}

void
push(int tag, void *owner = nullptr)
{
  require(micron::__exit_internal::__push(emit, reinterpret_cast<void *>(static_cast<uintptr_t>(tag)), owner) == 0);
}

void
last()
{
  emit(reinterpret_cast<void *>(6));
}

void
nested(void *)
{
  emit(reinterpret_cast<void *>(5));
  require(micron::atexit(last) == 0);
}

void
extend(void *)
{
  emit(reinterpret_cast<void *>(3));
  push(4);
  require(micron::__exit_internal::__push(nested, nullptr, &owner_a) == 0);
}

void
extend_owner(void *)
{
  emit(reinterpret_cast<void *>(8));
  push(9, &owner_b);
  push(10, &owner_a);
}

void
reenter(void *)
{
  emit(reinterpret_cast<void *>(11));
  push(12);
  micron::__drain_atexit_table();
  require(fini_calls == 0);
  emit(reinterpret_cast<void *>(13));
}

template<usize N>
void
check(const int (&expected)[N])
{
  micron::__drain_atexit_table();
  require(used == N);
  for ( usize i = 0; i < N; ++i ) require(trace[i] == expected[i]);
  micron::__drain_atexit_table();
  require(used == N);
  require(fini_calls == 0);
  used = 0;
}
}      // namespace

__attribute__((destructor)) void
count_fini()
{
  ++fini_calls;
}

int
main()
{
  // Cross a publication word boundary before extending the stack twice.
  for ( int i = 0; i < 63; ++i ) push(0);
  micron::__drain_atexit_table();
  require(used == 63);
  used = 0;
  push(1);
  push(2);
  require(micron::__exit_internal::__push(extend, nullptr) == 0);
  check({ 3, 5, 6, 4, 2, 1 });

  push(7, &owner_a);
  require(micron::__exit_internal::__push(extend_owner, nullptr, &owner_a) == 0);
  micron::__exit_internal::__finalize_dso(&owner_a);
  require(used == 3 && trace[0] == 8 && trace[1] == 10 && trace[2] == 7);
  micron::__exit_internal::__finalize_dso(&owner_a);
  check({ 8, 10, 7, 9 });

  require(micron::__exit_internal::__push(reenter, nullptr) == 0);
  check({ 11, 12, 13 });
  return 1;
}
