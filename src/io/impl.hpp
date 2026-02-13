//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
namespace io
{
namespace format
{
const char *
bytes_to_hex(byte *ptr, size_t len, char *frm)
{
  const char *hex = "0123456789ABCDEF";
  char *buf = frm;
  for ( size_t i = 0; i < len; i++ ) {
    *frm = hex[*ptr >> 4];
    *frm = hex[*ptr & 0x0F];
    ++buf;
    ++ptr;
  }
  return frm;
}
const char *
bool_to_char(bool n, char *frm)
{
  if ( n )
    frm = const_cast<char *>("true");
  else
    frm = const_cast<char *>("false");
  return frm;
}
const char *
int64_to_char(i64 n, char *frm)
{
  char *buf = frm;
  i64 ipart = static_cast<i64>(n);
  bool is_neg = n < 0;
  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
  }

  char istr[31];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf = '\0';
  return frm;
}

const char *
uint64_to_char(u64 n, char *frm)
{
  char *buf = frm;
  u64 ipart = static_cast<u64>(n);

  char istr[31];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf = '\0';
  return frm;
}
const char *
uint_to_char(u32 n, char *frm)
{
  char *buf = frm;
  u32 ipart = static_cast<u32>(n);

  char istr[31];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf = '\0';
  return frm;
}
const char *
int_to_char(i32 n, char *frm)
{
  char *buf = frm;
  i32 ipart = static_cast<i64>(n);
  bool is_neg = n < 0;
  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
  }

  char istr[31];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf = '\0';
  return frm;
}

const char *
f64_to_char(f64 n, char *frm)
{
  char *buf = frm;
  i64 ipart = static_cast<i64>(n);
  i64 fpart = static_cast<i64>((n - (float)ipart) * (i32)1e6);
  bool is_neg = n < 0;

  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
    fpart = -fpart;
  }

  char istr[24];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf++ = '.';     // decimal

  char fstr[24];
  char *fptr = fstr;
  do {
    *fptr++ = static_cast<char>('0' + (fpart % 10));
    fpart /= 10;
  } while ( fpart );
  while ( fptr != fstr )
    *buf++ = *--fptr;

  *buf = '\0';
  return frm;
}
const char *
f32_to_char(f32 n, char *frm)
{
  char *buf = frm;
  i32 ipart = static_cast<i32>(n);
  i32 fpart = static_cast<i32>((n - (float)ipart) * (i32)1e6);
  bool is_neg = n < 0;

  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
    fpart = -fpart;
  }

  char istr[16];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf++ = '.';     // decimal

  char fstr[8];
  char *fptr = fstr;
  do {
    *fptr++ = static_cast<char>('0' + (fpart % 10));
    fpart /= 10;
  } while ( fpart );
  while ( fptr != fstr )
    *buf++ = *--fptr;

  *buf = '\0';
  return frm;
}
const char *
flong_to_char(flong n, char *frm)
{
  char *buf = frm;
  i64 ipart = static_cast<i64>(n);
  i64 fpart = static_cast<i64>((n - ipart) * 1e6);
  bool is_neg = n < 0;

  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
    fpart = -fpart;
  }

  char istr[50];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf++ = '.';     // decimal

  char fstr[60];
  char *fptr = fstr;
  do {
    *fptr++ = static_cast<char>('0' + (fpart % 10));
    fpart /= 10;
  } while ( fpart );
  while ( fptr != fstr )
    *buf++ = *--fptr;

  *buf = '\0';
  return frm;
}
const char *
f128_to_char(f128 n, char *frm)
{
  char *buf = frm;
  i64 ipart = static_cast<i64>(n);
  i64 fpart = static_cast<i64>((n - ipart) * 1e6);
  bool is_neg = n < 0;

  if ( is_neg ) {
    *buf++ = '-';
    ipart = -ipart;
    fpart = -fpart;
  }

  char istr[50];
  char *iptr = istr;
  do {
    *iptr++ = static_cast<char>('0' + (ipart % 10));
    ipart /= 10;
  } while ( ipart );
  while ( iptr != istr ) {
    *buf++ = *--iptr;
  }

  *buf++ = '.';     // decimal

  char fstr[40];
  char *fptr = fstr;
  do {
    *fptr++ = static_cast<char>('0' + (fpart % 10));
    fpart /= 10;
  } while ( fpart );
  while ( fptr != fstr )
    *buf++ = *--fptr;

  *buf = '\0';
  return frm;
}
template <typename T>
const char *
ptr_to_char(T *ptr, char *frm)
{
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
  char *buf = frm;
  *buf++ = '0';
  *buf++ = 'x';

  for ( max_t i = sizeof(uintptr_t) * 2 - 1; i >= 0; i-- ) {
    int n = (addr >> (i * 4)) & 0xF;
    *buf++ = static_cast<char>((n < 10) ? ('0' + n) : ('a' + (n - 10)));
  }
  *buf = '\0';
  return frm;
};
};     // namespace format
};     // namespace io
};     // namespace micron
