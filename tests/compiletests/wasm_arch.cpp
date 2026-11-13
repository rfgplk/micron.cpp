// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/types.hpp"
#if defined(__micron_arch_wasm32)
#include "../../src/bits/__ctasserts.hpp"
static_assert(__ct_type_checker::check());
static_assert(sizeof(void *) == 4 && sizeof(long double) == 16);
#if defined(__micron_arch_x86_any) || defined(__micron_os_linux)
#error "wasm architecture selection leaked a native backend"
#endif
#endif
int
main()
{
  return 1;
}
