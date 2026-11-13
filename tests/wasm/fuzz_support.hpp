//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/bits/wasm/host.hpp"
#include "../../src/types.hpp"

#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0x243f6a88u
#endif
#ifndef MICRON_WASM_FUZZ_STEPS
#define MICRON_WASM_FUZZ_STEPS 4096
#endif

static_assert(MICRON_WASM_FUZZ_SEED > 0 && MICRON_WASM_FUZZ_SEED <= 0xffffffffull);
static_assert(MICRON_WASM_FUZZ_STEPS > 0 && MICRON_WASM_FUZZ_STEPS <= 1000000);

namespace wasm_fuzz
{
inline constexpr u32 seed = MICRON_WASM_FUZZ_SEED;
inline constexpr u32 steps = MICRON_WASM_FUZZ_STEPS;
inline u64 checks = 0;
inline thread_local u32 step = 0;

inline u32
next(u32 &state) noexcept
{
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

inline u64
next64(u32 &state) noexcept
{
  const u64 high = next(state);
  return (high << 32) | next(state);
}

inline void
number(u64 value)
{
  char text[24];
  unsigned pos = sizeof(text);
  do {
    text[--pos] = char('0' + value % 10);
    value /= 10;
  } while ( value );
  micron::wasm::write_diag(text + pos, sizeof(text) - pos);
}

inline bool
require(bool condition, const char *expression, unsigned line)
{
  __atomic_fetch_add(&checks, u64(1), __ATOMIC_RELAXED);
  if ( condition ) return true;
  micron::wasm::write_diag("wasm-fuzz failure: ");
  micron::wasm::write_diag(expression);
  micron::wasm::write_diag(" line=");
  number(line);
  micron::wasm::write_diag(" seed=");
  number(seed);
  micron::wasm::write_diag(" step=");
  number(step);
  micron::wasm::write_diag(" configured_steps=");
  number(steps);
  micron::wasm::write_diag("\n");
  return false;
}

inline int
finish(const char *name)
{
  micron::wasm::write_diag("wasm-fuzz ");
  micron::wasm::write_diag(name);
  micron::wasm::write_diag(": ");
  number(__atomic_load_n(&checks, __ATOMIC_RELAXED));
  micron::wasm::write_diag(" checks seed=");
  number(seed);
  micron::wasm::write_diag(" steps=");
  number(steps);
  micron::wasm::write_diag("\n");
  return 1;
}
}      // namespace wasm_fuzz

#define WASM_FUZZ_CHECK(expression) ::wasm_fuzz::require(bool(expression), #expression, __LINE__)

extern "C" __attribute__((export_name("__micron_wasm_test_cases"))) unsigned
__micron_wasm_test_cases()
{
  const u64 count = __atomic_load_n(&wasm_fuzz::checks, __ATOMIC_RELAXED);
  return count > 0xffffffffull ? 0xffffffffu : unsigned(count);
}

extern "C" __attribute__((export_name("__micron_wasm_test_skips"))) unsigned
__micron_wasm_test_skips()
{
  return 0;
}
