#pragma once

namespace micron
{
namespace math
{

constexpr float
fabs(float x)
{
  u32 i = *(u32 *)&x;
  i &= 0x7FFFFFFF;
  return *(float *)&i;
}

constexpr double
fabs64(double x)
{
  u64 i = *(u64 *)&x;
  i &= 0x7FFFFFFFFFFFFFFFULL;
  return *(double *)&i;
}

constexpr float
fneg(float x)
{
  u32 i = *(u32 *)&x;
  i ^= 0x80000000;
  return *(float *)&i;
}

constexpr double
fneg64(double x)
{
  u64 i = *(u64 *)&x;
  i ^= 0x8000000000000000ULL;
  return *(double *)&i;
}

constexpr float
fmax(float a, float b)
{
  return a - ((a - b) * float(a - b < 0));
}

constexpr float
fmin(float a, float b)
{
  return b + ((a - b) * float(a - b < 0));
}

constexpr double
fmax64(double a, double b)
{
  return a - ((a - b) * double(a - b < 0));
}

constexpr double
fmin64(double a, double b)
{
  return b + ((a - b) * double(a - b < 0));
}

constexpr float
fclamp(float x, float lo, float hi)
{
  return fmin(fmax(x, lo), hi);
}

constexpr double
fclamp64(double x, double lo, double hi)
{
  return fmin64(fmax64(x, lo), hi);
}

constexpr float
frnd(float x)
{
  return x + (x >= 0 ? 0.5f : -0.5f);
}

constexpr double
frnd64(double x)
{
  return x + (x >= 0 ? 0.5 : -0.5);
}

constexpr float
ffloor(float x)
{
  int i = int(x);
  return i > x ? i - 1 : i;
}

constexpr float
fceil(float x)
{
  int i = int(x);
  return i < x ? i + 1 : i;
}

constexpr double
ffloor64(double x)
{
  i64 i = i64(x);
  return i > x ? i - 1 : i;
}

constexpr double
fceil64(double x)
{
  i64 i = i64(x);
  return i < x ? i + 1 : i;
}

constexpr float
ffract(float x)
{
  return x - ffloor(x);
}

constexpr double
ffract64(double x)
{
  return x - ffloor64(x);
}

constexpr float
lerp(float a, float b, float t)
{
  return a + (b - a) * t;
}

constexpr double
lerp64(double a, double b, double t)
{
  return a + (b - a) * t;
}

constexpr float
fabsmax(float a, float b)
{
  return fmax(fabs(a), fabs(b));
}

constexpr float
fabsmin(float a, float b)
{
  return fmin(fabs(a), fabs(b));
}

constexpr double
fabsmax64(double a, double b)
{
  return fmax64(fabs64(a), fabs64(b));
}

constexpr double
fabsmin64(double a, double b)
{
  return fmin64(fabs64(a), fabs64(b));
}

constexpr int
fsign(float x)
{
  return (*(u32 *)&x) >> 31;
}

constexpr int
fsign64(double x)
{
  return (*(u64 *)&x) >> 63;
}

constexpr int
round_nearest(float x)
{
  return int(x + (x >= 0 ? 0.5f : -0.5f));
}

constexpr unsigned
div_const(unsigned a, unsigned d)
{
  constexpr unsigned m = (1ULL << 32) / d + 1;
  return (u64(a) * m) >> 32;
}

constexpr int
mul_const(int x, int k)
{
  int res = 0;
  for ( int i = 0; k; ++i ) {
    if ( k & 1 )
      res += x << i;
    k >>= 1;
  }
  return res;
}

constexpr i8
abs8(i8 x)
{
  i8 m = x >> 7;
  return (x + m) ^ m;
}

constexpr i16
abs16(i16 x)
{
  i16 m = x >> 15;
  return (x + m) ^ m;
}

constexpr i32
abs(i32 x)
{
  i32 m = x >> 31;
  return (x + m) ^ m;
}

constexpr i64
abs64(i64 x)
{
  i64 m = x >> 63;
  return (x + m) ^ m;
}

constexpr i8
sign8(i8 x)
{
  return (x >> 7) - (-x >> 7);
}

constexpr i16
sign16(i16 x)
{
  return (x >> 15) - (-x >> 15);
}

constexpr i32
sign(i32 x)
{
  return (x >> 31) - (-x >> 31);
}

constexpr i64
sign64(i64 x)
{
  return (x >> 63) - (-x >> 63);
}

constexpr int
lt64(i64 a, i64 b)
{
  return u64(a - b) >> 63;
}

constexpr i32
lt(i32 a, i32 b)
{
  return (unsigned)(a - b) >> 31;
}

constexpr i16
lt16(i16 a, i16 b)
{
  return (unsigned)(a - b) >> 15;
}

constexpr i8
lt8(i8 a, i8 b)
{
  return (unsigned)(a - b) >> 7;
}

constexpr int
gt64(i64 a, i64 b)
{
  return (u64(b - a) >> 63) ^ 1;
}

constexpr i32
gt(i32 a, i32 b)
{
  return (unsigned)(a - b) >> 31 ^ 1;
}

constexpr i16
gt16(i16 a, i16 b)
{
  return (unsigned)(a - b) >> 15 ^ 1;
}

constexpr i64
ge64(i64 a, i64 b)
{
  return ~((unsigned)(a - b) >> 63) & 1;
}

constexpr i32
ge(i32 a, i32 b)
{
  return ~((unsigned)(a - b) >> 31) & 1;
}

constexpr i16
ge16(i16 a, i16 b)
{
  return ~((unsigned)(a - b) >> 15) & 1;
}

constexpr i8
ge8(i8 a, i8 b)
{
  return ~((unsigned)(a - b) >> 7) & 1;
}

constexpr i64
le64(i64 a, i64 b)
{
  i64 d = a - b;
  return b + (d & (d >> 63));
}

constexpr i32
le(i32 a, i32 b)
{
  return ~((unsigned)(b - a) >> 31) & 1;
}

constexpr i16
le16(i16 a, i16 b)
{
  return ~((unsigned)(b - a) >> 15) & 1;
}

constexpr i8
le8(i8 a, i8 b)
{
  return ~((unsigned)(b - a) >> 7) & 1;
}

constexpr i64
min64(i64 a, i64 b)
{
  i64 d = a - b;
  return b + (d & (d >> 63));
}

constexpr i32
min(i32 a, i32 b)
{
  i32 d = a - b;
  return b + (d & (d >> 31));
}

constexpr i16
min16(i16 a, i16 b)
{
  int d = a - b;
  return b + (d & (d >> 15));
}

constexpr i8
min8(i8 a, i8 b)
{
  i8 d = a - b;
  return b + (d & (d >> 7));
}

constexpr i8
max8(i8 a, i8 b)
{
  i8 d = a - b;
  return a - (d & (d >> 7));
}

constexpr int
max16(i16 a, i16 b)
{
  i16 d = a - b;
  return a - (d & (d >> 15));
}

constexpr i32
max(i32 a, i32 b)
{
  i32 d = a - b;
  return a - (d & (d >> 31));
}

constexpr i64
max64(i64 a, i64 b)
{
  i64 d = a - b;
  return a - (d & (d >> 63));
}

constexpr int
eq8(i8 a, i8 b)
{
  return !(a ^ b);
}

constexpr int
eq16(i16 a, i16 b)
{
  return !(a ^ b);
}

constexpr int
eq(int a, int b)
{
  return !(a ^ b);
}

constexpr int
eq64(i64 a, i64 b)
{
  return !(a ^ b);
}

constexpr int
mod2(int a)
{
  return a & 1;
}

constexpr int
mod3(unsigned a)
{
  constexpr unsigned m = 0xAAAAAAABu;
  unsigned q = (u64(a) * m) >> 33;
  return a - q * 3;
}

constexpr int
mod5(unsigned a)
{
  constexpr unsigned m = 0xCCCCCCCDu;
  unsigned q = (u64(a) * m) >> 34;
  return a - q * 5;
}

constexpr int
mod7(unsigned a)
{
  constexpr unsigned m = 0x24924925u;
  unsigned q = (u64(a) * m) >> 35;
  return a - q * 7;
}

constexpr i8
clamp8(i8 x, i8 lo, i8 hi)
{
  return min8(max8(x, lo), hi);
}

constexpr i16
clamp16(i16 x, i16 lo, i16 hi)
{
  return min16(max16(x, lo), hi);
}

constexpr i32
clamp(i32 x, i32 lo, i32 hi)
{
  return min(max(x, lo), hi);
}

constexpr i64
clamp64(i64 x, i64 lo, i64 hi)
{
  return min64(max64(x, lo), hi);
}

constexpr i8
mod2_8(i8 x, int p)
{
  return x & ((1 << p) - 1);
}

constexpr i16
mod2_16(i16 x, int p)
{
  return x & ((1 << p) - 1);
}

constexpr i32
mod2(i32 x, int p)
{
  return x & ((1 << p) - 1);
}

constexpr i64
mod2_64(i64 x, int p)
{
  return x & ((1ULL << p) - 1);
}

constexpr i8
bit8(i8 x, int N)
{
  return (x >> N) & 1;
}

constexpr i16
bit16(i16 x, int N)
{
  return (x >> N) & 1;
}

constexpr i32
bit(i32 x, int N)
{
  return (x >> N) & 1;
}

constexpr i64
bit64(i64 x, int N)
{
  return (x >> N) & 1;
}

constexpr i8
mask8(i8 x, i8 mask)
{
  return !!(x & mask);
}

constexpr i16
mask16(i16 x, i16 mask)
{
  return !!(x & mask);
}

constexpr i32
mask(i32 x, i32 mask)
{
  return !!(x & mask);
}

constexpr i64
mask64(i64 x, i64 mask)
{
  return !!(x & mask);
}

constexpr int
popcount8(u8 x)
{
  x = x - ((x >> 1) & 0x55);
  x = (x & 0x33) + ((x >> 2) & 0x33);
  return (x + (x >> 4)) & 0x0F;
}

constexpr int
popcount16(u16 x)
{
  x = x - ((x >> 1) & 0x5555);
  x = (x & 0x3333) + ((x >> 2) & 0x3333);
  x = (x + (x >> 4)) & 0x0F0F;
  return (x + (x >> 8)) & 0x00FF;
}

constexpr int
popcount(u32 x)
{
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0F0F0F0F;
  x = x + (x >> 8);
  x = x + (x >> 16);
  return x & 0x3F;
}

constexpr int
popcount64(u64 x)
{
  x = x - ((x >> 1) & 0x5555555555555555ULL);
  x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
  x = x + (x >> 8);
  x = x + (x >> 16);
  x = x + (x >> 32);
  return x & 0x7F;
}

constexpr int
clz(u32 x)
{
  if ( x == 0 )
    return 32;
  int n = 0;
  if ( (x >> 16) == 0 ) {
    n += 16;
    x <<= 16;
  }
  if ( (x >> 24) == 0 ) {
    n += 8;
    x <<= 8;
  }
  if ( (x >> 28) == 0 ) {
    n += 4;
    x <<= 4;
  }
  if ( (x >> 30) == 0 ) {
    n += 2;
    x <<= 2;
  }
  if ( (x >> 31) == 0 )
    n += 1;
  return n;
}

constexpr int
clz64(u64 x)
{
  if ( x == 0 )
    return 64;
  int n = 0;
  if ( (x >> 32) == 0 ) {
    n += 32;
    x <<= 32;
  }
  if ( (x >> 48) == 0 ) {
    n += 16;
    x <<= 16;
  }
  if ( (x >> 56) == 0 ) {
    n += 8;
    x <<= 8;
  }
  if ( (x >> 60) == 0 ) {
    n += 4;
    x <<= 4;
  }
  if ( (x >> 62) == 0 ) {
    n += 2;
    x <<= 2;
  }
  if ( (x >> 63) == 0 )
    n += 1;
  return n;
}

constexpr u32
nextpow2(u32 x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

constexpr u64
nextpow2_64(u64 x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}

constexpr u8
reverse8(u8 x)
{
  x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
  x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
  x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
  return x;
}

constexpr u16
reverse16(u16 x)
{
  x = (x & 0xFF00) >> 8 | (x & 0x00FF) << 8;
  x = (x & 0xF0F0) >> 4 | (x & 0x0F0F) << 4;
  x = (x & 0xCCCC) >> 2 | (x & 0x3333) << 2;
  x = (x & 0xAAAA) >> 1 | (x & 0x5555) << 1;
  return x;
}

constexpr u32
reverse(u32 x)
{
  x = (x >> 16) | (x << 16);
  x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
  x = ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4);
  x = ((x & 0xCCCCCCCC) >> 2) | ((x & 0x33333333) << 2);
  x = ((x & 0xAAAAAAAA) >> 1) | ((x & 0x55555555) << 1);
  return x;
}

constexpr u64
reverse64(u64 x)
{
  x = (x >> 32) | (x << 32);
  x = ((x & 0xFFFF0000FFFF0000ULL) >> 16) | ((x & 0x0000FFFF0000FFFFULL) << 16);
  x = ((x & 0xFF00FF00FF00FF00ULL) >> 8) | ((x & 0x00FF00FF00FF00FFULL) << 8);
  x = ((x & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
  x = ((x & 0xCCCCCCCCCCCCCCCCULL) >> 2) | ((x & 0x3333333333333333ULL) << 2);
  x = ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1) | ((x & 0x5555555555555555ULL) << 1);
  return x;
}

constexpr int
parity8(u8 x)
{
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

constexpr int
parity16(u16 x)
{
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

constexpr int
parity(u32 x)
{
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

constexpr int
parity64(u64 x)
{
  x ^= x >> 32;
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

constexpr u32
swap_bits(u32 x, u32 mask, int shift)
{
  u32 t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}

constexpr u64
swap_bits64(u64 x, u64 mask, int shift)
{
  u64 t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}

};     // namespace math
};     // namespace micron
