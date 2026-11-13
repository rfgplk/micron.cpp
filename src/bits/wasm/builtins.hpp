//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

extern "C" {
__attribute__((weak, used)) __int128
__multi3(__int128 a, __int128 b) noexcept
{
  struct words {
    unsigned long long word[2];
  };

  const auto x = __builtin_bit_cast(words, a), y = __builtin_bit_cast(words, b);
  words r{};
  const unsigned long long al = static_cast<unsigned>(x.word[0]), ah = x.word[0] >> 32;
  const unsigned long long bl = static_cast<unsigned>(y.word[0]), bh = y.word[0] >> 32;
  const unsigned long long p0 = al * bl, p1 = al * bh, p2 = ah * bl, p3 = ah * bh;
  const unsigned long long mid = (p0 >> 32) + static_cast<unsigned>(p1) + static_cast<unsigned>(p2);
  r.word[0] = (mid << 32) | static_cast<unsigned>(p0);
  r.word[1] = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32) + x.word[1] * y.word[0] + x.word[0] * y.word[1];
  return __builtin_bit_cast(__int128, r);
}

__attribute__((weak, used)) void *
memcpy(void *d, const void *s, __SIZE_TYPE__ n)
{
  auto *out = static_cast<volatile unsigned char *>(d);
  const auto *in = static_cast<const volatile unsigned char *>(s);
  for ( __SIZE_TYPE__ i = 0; i < n; ++i ) out[i] = in[i];
  return d;
}

__attribute__((weak, used)) void *
memmove(void *d, const void *s, __SIZE_TYPE__ n)
{
  auto *out = static_cast<volatile unsigned char *>(d);
  const auto *in = static_cast<const volatile unsigned char *>(s);
  if ( reinterpret_cast<__UINTPTR_TYPE__>(d) < reinterpret_cast<__UINTPTR_TYPE__>(s) )
    for ( __SIZE_TYPE__ i = 0; i < n; ++i ) out[i] = in[i];
  else
    for ( __SIZE_TYPE__ i = n; i; --i ) out[i - 1] = in[i - 1];
  return d;
}

__attribute__((weak, used)) void *
memset(void *d, int c, __SIZE_TYPE__ n)
{
  auto *out = static_cast<volatile unsigned char *>(d);
  for ( __SIZE_TYPE__ i = 0; i < n; ++i ) out[i] = static_cast<unsigned char>(c);
  return d;
}

__attribute__((weak, used)) int
memcmp(const void *a, const void *b, __SIZE_TYPE__ n)
{
  const auto *x = static_cast<const unsigned char *>(a), *y = static_cast<const unsigned char *>(b);
  for ( __SIZE_TYPE__ i = 0; i < n; ++i )
    if ( x[i] != y[i] ) return static_cast<int>(x[i]) - static_cast<int>(y[i]);
  return 0;
}
}
