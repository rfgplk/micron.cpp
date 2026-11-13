// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/bits/__ctasserts.hpp"
#include "../../src/types.hpp"

#if !defined(__micron_arch_wasm32) || defined(__micron_arch_x86) || defined(__micron_arch_x86_any) || defined(__micron_os_linux)
#error "WebAssembly must select its own architecture and environment"
#endif
#if defined(__micron_x86_sse2) || defined(__micron_x86_avx2) || defined(__micron_arm_neon)
#error "WebAssembly cannot expose native ISA capabilities"
#endif
static_assert(__micron_arch == __micron_arch_wasm32 && __micron_width == 32);
static_assert(sizeof(void *) == 4 && sizeof(long) == 4);
static_assert(sizeof(long double) == 16 && alignof(long double) == 16);
static_assert(sizeof(__micron_guard_t) == 4);
static_assert(micron::is_integral_v<__int128> && micron::is_integral_v<unsigned __int128>);
static_assert(__ct_type_checker::check());
#if defined(__wasm_simd128__) && !defined(MICRON_NO_SIMD)
static_assert(__micron_simd_width == 128);
#else
static_assert(__micron_simd_width == 0);
#endif

int
main()
{
  return 1;
}
