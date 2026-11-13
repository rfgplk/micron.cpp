//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../../src/bits/eh/cxa_eh_globals.hpp"
#include "../../src/bits/wasm/eh_std.hpp"
#include "../../src/bits/wasm/runtime.hpp"
#if defined(__micron_eh)
struct base {
  int value = 17;

  virtual ~base() { }
};

struct left {
  int l = 4;

  virtual ~left() { }
};

struct derived: left, base {
  int d = 9;
};

struct virtual_base {
  int v = 21;

  virtual ~virtual_base() { }
};

struct virtual_left: virtual virtual_base {
};

struct virtual_right: virtual virtual_base {
};

struct diamond: virtual_left, virtual_right {
};

struct private_base: private base {
};

struct ambiguous_left: base {
};

struct ambiguous_right: base {
};

struct ambiguous: ambiguous_left, ambiguous_right {
};

static int destroyed = 0;

struct scope {
  ~scope() { ++destroyed; }
};

__attribute__((noinline)) static void
raise_int()
{
  scope s;
  throw 42;
}

__attribute__((noinline)) static void
rethrow_int()
{
  try {
    raise_int();
  } catch ( ... ) {
    throw;
  }
}

#endif
int
main()
{
#if defined(__micron_eh)
  try {
    rethrow_int();
    return 6;
  } catch ( int v ) {
    if ( v != 42 || destroyed != 1 ) return 6;
  }
  try {
    throw derived{};
  } catch ( const base &v ) {
    if ( v.value != 17 ) return 6;
  } catch ( ... ) {
    return 6;
  }
  try {
    throw diamond{};
  } catch ( const virtual_base &v ) {
    if ( v.v != 21 ) return 6;
  } catch ( ... ) {
    return 6;
  }
  try {
    throw private_base{};
  } catch ( const base & ) {
    return 6;
  } catch ( ... ) {
  }
  try {
    throw ambiguous{};
  } catch ( const base & ) {
    return 6;
  } catch ( ... ) {
  }
  int number = 3;
  try {
    throw &number;
  } catch ( const int *p ) {
    if ( p != &number ) return 6;
  } catch ( ... ) {
    return 6;
  }
  const int *constant = &number;
  try {
    throw constant;
  } catch ( int * ) {
    return 6;
  } catch ( const int *p ) {
    if ( p != constant ) return 6;
  }
  try {
    throw &number;
  } catch ( void *p ) {
    if ( p != &number ) return 6;
  } catch ( ... ) {
    return 6;
  }
  base unrelated;
  try {
    (void)dynamic_cast<derived &>(unrelated);
    return 6;
  } catch ( const std::bad_cast & ) {
  }
  base *null_object = nullptr;
  try {
    (void)typeid(*null_object);
    return 6;
  } catch ( const std::bad_typeid & ) {
  }
  try {
    throw nullptr;
  } catch ( int base::*member ) {
    if ( member != nullptr ) return 6;
  } catch ( ... ) {
    return 6;
  }
  try {
    throw nullptr;
  } catch ( void (base::*member)() ) {
    if ( member != nullptr ) return 6;
  } catch ( ... ) {
    return 6;
  }
  derived object;
  base *base_pointer = &object;
  if ( dynamic_cast<derived *>(base_pointer) != &object || dynamic_cast<left *>(base_pointer) != static_cast<left *>(&object) ) return 6;
  ambiguous multiple;
  base *one_base = static_cast<ambiguous_left *>(&multiple);
  if ( dynamic_cast<ambiguous *>(one_base) != &multiple ) return 6;
  diamond diamond_object;
  virtual_base *virtual_pointer = &diamond_object;
  if ( dynamic_cast<diamond *>(virtual_pointer) != &diamond_object ) return 6;
  try {
    throw &object;
  } catch ( base *p ) {
    if ( p != static_cast<base *>(&object) ) return 6;
  } catch ( ... ) {
    return 6;
  }
  try {
    throw nullptr;
  } catch ( int *p ) {
    if ( p ) return 6;
  } catch ( ... ) {
    return 6;
  }
  if ( __cxa_get_globals()->uncaughtExceptions || __cxa_get_globals()->caughtExceptions ) return 6;
#endif
  return 1;
}
