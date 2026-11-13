//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#pragma once

// Sanitizer failures must not collide with snowball's success sentinel (1).
extern "C" const char *
__asan_default_options()
{
  return "exitcode=77:detect_leaks=0";
}

extern "C" const char *
__ubsan_default_options()
{
  return "exitcode=77:halt_on_error=1";
}

[[maybe_unused]] static long double
absolute(long double x)
{
  return x < 0 ? -x : x;
}

[[maybe_unused]] static long double
sine(long double x)
{
  constexpr long double pi = 3.1415926535897932384626433832795028841971693993751L;
  while ( x > pi ) x -= 2 * pi;
  while ( x < -pi ) x += 2 * pi;
  long double term = x, result = x;
  for ( unsigned j = 1; j < 60; ++j ) {
    term *= -x * x / ((2 * j) * (2 * j + 1));
    result += term;
  }
  return result;
}

[[maybe_unused]] static long double
cosine(long double x)
{
  constexpr long double pi = 3.1415926535897932384626433832795028841971693993751L;
  return sine(x + pi / 2);
}

static u64 random_state = 0x9e3779b97f4a7c15ULL;

[[maybe_unused]] static long double
sample()
{
  random_state ^= random_state >> 12;
  random_state ^= random_state << 25;
  random_state ^= random_state >> 27;
  return static_cast<long double>(i32((random_state * 0x2545f4914f6cdd1dULL) >> 32)) / 2147483648.0L;
}

template<typename F>
[[maybe_unused]] static long double
epsilon()
{
  if constexpr ( micron::is_same_v<F, float> ) return micron::numeric_limits<float>::epsilon();
  if constexpr ( micron::is_same_v<F, double> ) return micron::numeric_limits<double>::epsilon();
  return __micron_ldbl_epsilon;
}

template<typename F>
static void
check(F actual, long double expected, long double tolerance)
{
  if ( !(absolute(static_cast<long double>(actual) - expected) <= tolerance) ) {
    micron::io::println("actual ", actual, " expected ", expected, " tolerance ", tolerance);
    sb::require_true(false);
  }
}
