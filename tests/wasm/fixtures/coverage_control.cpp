// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.

#ifndef MICRON_COVERAGE_CONTROL
#define MICRON_COVERAGE_CONTROL 1
#endif

#if MICRON_COVERAGE_CONTROL == 5 || MICRON_COVERAGE_CONTROL == 6 || MICRON_COVERAGE_CONTROL == 9 || MICRON_COVERAGE_CONTROL == 10
using case_value = double;
#elif MICRON_COVERAGE_CONTROL == 7
using case_value = unsigned long long;
#else
using case_value = unsigned;
#endif

#if MICRON_COVERAGE_CONTROL == 8
using skip_value = double;
#else
using skip_value = unsigned;
#endif

extern "C" __attribute__((export_name("__micron_wasm_test_cases"))) case_value
__micron_wasm_test_cases()
{
#if MICRON_COVERAGE_CONTROL == 0
  return 0;
#elif MICRON_COVERAGE_CONTROL == 2
  return 0x80000000u;
#elif MICRON_COVERAGE_CONTROL == 3
  return 0xffffffffu;
#elif MICRON_COVERAGE_CONTROL == 5
  return 0.5;
#elif MICRON_COVERAGE_CONTROL == 6
  return 0x1p32;
#elif MICRON_COVERAGE_CONTROL == 9
  return __builtin_nan("");
#elif MICRON_COVERAGE_CONTROL == 10
  return -2147483649.0;
#else
  return 1;
#endif
}

extern "C" __attribute__((export_name("__micron_wasm_test_skips"))) skip_value
__micron_wasm_test_skips()
{
#if MICRON_COVERAGE_CONTROL == 4
  return 0x80000000u;
#elif MICRON_COVERAGE_CONTROL == 8
  return 0.5;
#else
  return 0;
#endif
}

int
main()
{
  return 1;
}
