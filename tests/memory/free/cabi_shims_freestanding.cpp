//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Freestanding C-ABI shim gate. GCC in a freestanding environment is
// entitled to emit calls to memcpy/memmove/memset; the large aggregate
// copies below force exactly that. Linking this file with
//     ./bin/duck build tests/memory/free/cabi_shims_freestanding.cpp -k -o bin/memory/free
// (-k = -ffreestanding -nostdlib) succeeds only if micron's extern "C"
// shims in src/memory/cmemory/ satisfy those references.
// The file also builds and passes hosted (calls land in glibc instead).

#include "../../../src/memory/memory.hpp"
#include "../../../src/std.hpp"

extern "C" {
void *memcpy(void *, const void *, __SIZE_TYPE__) noexcept;
void *memmove(void *, const void *, __SIZE_TYPE__) noexcept;
void *memset(void *, int, __SIZE_TYPE__) noexcept;
}

namespace
{

struct big {
  char x[4096];
};

big g_a, g_b;
volatile unsigned g_sink;

typedef void *(*cpy_t)(void *, const void *, __SIZE_TYPE__);
typedef void *(*set_t)(void *, int, __SIZE_TYPE__);

cpy_t volatile g_cpy = &::memcpy;
cpy_t volatile g_mov = reinterpret_cast<cpy_t>(&::memmove);
set_t volatile g_set = &::memset;

}      // namespace

int
main()
{
  // compiler-emitted calls: 4 KiB aggregate copy + zero-init
  for ( unsigned i = 0; i < sizeof(g_a.x); ++i ) g_a.x[i] = static_cast<char>(i * 13 + 7);
  g_b = g_a;
  big z{};
  g_sink = static_cast<unsigned char>(g_b.x[513]) + static_cast<unsigned char>(z.x[100]);

  // explicit calls through the C ABI symbols
  cpy_t c = g_cpy;
  set_t s = g_set;
  cpy_t m = g_mov;
  s(g_b.x, 0xA5, sizeof(g_b.x));
  c(g_a.x, g_b.x, sizeof(g_a.x));
  m(g_a.x + 1, g_a.x, sizeof(g_a.x) - 1);
  for ( unsigned i = 0; i < sizeof(g_a.x); ++i )
    if ( static_cast<unsigned char>(g_a.x[i]) != 0xA5 ) return 0;

  return 1;      // snowball pass sentinel
}
