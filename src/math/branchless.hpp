#pragma once

namespace micron
{
namespace math
{

constexpr float
fabs(float x)
{
  uint32_t i = *(uint32_t *)&x;
  i &= 0x7FFFFFFF;
  return *(float *)&i;
}
constexpr double
fabs64(double x)
{
  uint64_t i = *(uint64_t *)&x;
  i &= 0x7FFFFFFFFFFFFFFFULL;
  return *(double *)&i;
}
constexpr float
fneg(float x)
{
  uint32_t i = *(uint32_t *)&x;
  i ^= 0x80000000;
  return *(float *)&i;
}
constexpr double
fneg64(double x)
{
  uint64_t i = *(uint64_t *)&x;
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
  int64_t i = int64_t(x);
  return i > x ? i - 1 : i;
}
constexpr double
fceil64(double x)
{
  int64_t i = int64_t(x);
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
  return (*(uint32_t *)&x) >> 31;
}
constexpr int
fsign64(double x)
{
  return (*(uint64_t *)&x) >> 63;
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
  return (uint64_t(a) * m) >> 32;
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
constexpr int8_t
abs8(int8_t x)
{
  int8_t m = x >> 7;
  return (x + m) ^ m;
}
constexpr int16_t
abs16(int16_t x)
{
  int16_t m = x >> 15;
  return (x + m) ^ m;
}
constexpr int32_t
abs(int32_t x)
{
  int32_t m = x >> 31;
  return (x + m) ^ m;
}
constexpr int64_t
abs64(int64_t x)
{
  int64_t m = x >> 63;
  return (x + m) ^ m;
}
constexpr int8_t
sign8(int8_t x)
{
  return (x >> 7) - (-x >> 7);
}
constexpr int16_t
sign16(int16_t x)
{
  return (x >> 15) - (-x >> 15);
}
constexpr int32_t
sign(int32_t x)
{
  return (x >> 31) - (-x >> 31);
}
constexpr int64_t
sign64(int64_t x)
{
  return (x >> 63) - (-x >> 63);
}
constexpr int
lt64(int64_t a, int64_t b)
{
  return uint64_t(a - b) >> 63;
}
constexpr int32_t
lt(int32_t a, int32_t b)
{
  return (unsigned)(a - b) >> 31;
}
constexpr int16_t
lt16(int16_t a, int16_t b)
{
  return (unsigned)(a - b) >> 15;
}
constexpr int8_t
lt8(int8_t a, int8_t b)
{
  return (unsigned)(a - b) >> 7;
}
constexpr int
gt64(int64_t a, int64_t b)
{
  return (uint64_t(b - a) >> 63) ^ 1;
}
constexpr int32_t
gt(int32_t a, int32_t b)
{
  return (unsigned)(a - b) >> 31 ^ 1;
}
constexpr int16_t
gt16(int16_t a, int16_t b)
{
  return (unsigned)(a - b) >> 15 ^ 1;
}
constexpr int64_t
ge64(int64_t a, int64_t b)
{
  return ~((unsigned)(a - b) >> 63) & 1;
}
constexpr int32_t
ge(int32_t a, int32_t b)
{
  return ~((unsigned)(a - b) >> 31) & 1;
}
constexpr int16_t
ge16(int16_t a, int16_t b)
{
  return ~((unsigned)(a - b) >> 15) & 1;
}
constexpr int8_t
ge8(int8_t a, int8_t b)
{
  return ~((unsigned)(a - b) >> 7) & 1;
}
constexpr int64_t
le64(int64_t a, int64_t b)
{
  int64_t d = a - b;
  return b + (d & (d >> 63));
}
constexpr int32_t
le(int32_t a, int32_t b)
{
  return ~((unsigned)(b - a) >> 31) & 1;
}
constexpr int16_t
le16(int16_t a, int16_t b)
{
  return ~((unsigned)(b - a) >> 15) & 1;
}
constexpr int8_t
le8(int8_t a, int8_t b)
{
  return ~((unsigned)(b - a) >> 7) & 1;
}
constexpr int64_t
min64(int64_t a, int64_t b)
{
  int64_t d = a - b;
  return b + (d & (d >> 63));
}
constexpr int32_t
min(int32_t a, int32_t b)
{
  int32_t d = a - b;
  return b + (d & (d >> 31));
}
constexpr int16_t
min16(int16_t a, int16_t b)
{
  int d = a - b;
  return b + (d & (d >> 15));
}
constexpr int8_t
min8(int8_t a, int8_t b)
{
  int8_t d = a - b;
  return b + (d & (d >> 7));
}
constexpr int8_t
max8(int8_t a, int8_t b)
{
  int8_t d = a - b;
  return a - (d & (d >> 7));
}
constexpr int
max16(int16_t a, int16_t b)
{
  int16_t d = a - b;
  return a - (d & (d >> 15));
}
constexpr int32_t
max(int32_t a, int32_t b)
{
  int32_t d = a - b;
  return a - (d & (d >> 31));
}
constexpr int64_t
max64(int64_t a, int64_t b)
{
  int64_t d = a - b;
  return a - (d & (d >> 63));
}
constexpr int
eq8(int8_t a, int8_t b)
{
  return !(a ^ b);
}
constexpr int
eq16(int16_t a, int16_t b)
{
  return !(a ^ b);
}
constexpr int
eq(int a, int b)
{
  return !(a ^ b);
}
constexpr int
eq64(int64_t a, int64_t b)
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
  unsigned q = (uint64_t(a) * m) >> 33;
  return a - q * 3;
}
constexpr int
mod5(unsigned a)
{
  constexpr unsigned m = 0xCCCCCCCDu;
  unsigned q = (uint64_t(a) * m) >> 34;
  return a - q * 5;
}
constexpr int
mod7(unsigned a)
{
  constexpr unsigned m = 0x24924925u;
  unsigned q = (uint64_t(a) * m) >> 35;
  return a - q * 7;
}
constexpr int8_t
clamp8(int8_t x, int8_t lo, int8_t hi)
{
  return min8(max8(x, lo), hi);
}
constexpr int16_t
clamp16(int16_t x, int16_t lo, int16_t hi)
{
  return min16(max16(x, lo), hi);
}
constexpr int32_t
clamp(int32_t x, int32_t lo, int32_t hi)
{
  return min(max(x, lo), hi);
}
constexpr int64_t
clamp64(int64_t x, int64_t lo, int64_t hi)
{
  return min64(max64(x, lo), hi);
}
constexpr int8_t
mod2_8(int8_t x, int p)
{
  return x & ((1 << p) - 1);
}
constexpr int16_t
mod2_16(int16_t x, int p)
{
  return x & ((1 << p) - 1);
}
constexpr int32_t
mod2(int32_t x, int p)
{
  return x & ((1 << p) - 1);
}
constexpr int64_t
mod2_64(int64_t x, int p)
{
  return x & ((1ULL << p) - 1);
}
constexpr int8_t
bit8(int8_t x, int N)
{
  return (x >> N) & 1;
}
constexpr int16_t
bit16(int16_t x, int N)
{
  return (x >> N) & 1;
}
constexpr int32_t
bit(int32_t x, int N)
{
  return (x >> N) & 1;
}
constexpr int64_t
bit64(int64_t x, int N)
{
  return (x >> N) & 1;
}
constexpr int8_t
mask8(int8_t x, int8_t mask)
{
  return !!(x & mask);
}
constexpr int16_t
mask16(int16_t x, int16_t mask)
{
  return !!(x & mask);
}
constexpr int32_t
mask(int32_t x, int32_t mask)
{
  return !!(x & mask);
}
constexpr int64_t
mask64(int64_t x, int64_t mask)
{
  return !!(x & mask);
}
constexpr int
popcount8(uint8_t x)
{
  x = x - ((x >> 1) & 0x55);
  x = (x & 0x33) + ((x >> 2) & 0x33);
  return (x + (x >> 4)) & 0x0F;
}
constexpr int
popcount16(uint16_t x)
{
  x = x - ((x >> 1) & 0x5555);
  x = (x & 0x3333) + ((x >> 2) & 0x3333);
  x = (x + (x >> 4)) & 0x0F0F;
  return (x + (x >> 8)) & 0x00FF;
}
constexpr int
popcount(uint32_t x)
{
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0F0F0F0F;
  x = x + (x >> 8);
  x = x + (x >> 16);
  return x & 0x3F;
}
constexpr int
popcount64(uint64_t x)
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
clz(uint32_t x)
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
clz64(uint64_t x)
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
constexpr uint32_t
nextpow2(uint32_t x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}
constexpr uint64_t
nextpow2_64(uint64_t x)
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
constexpr uint8_t
reverse8(uint8_t x)
{
  x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
  x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
  x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
  return x;
}
constexpr uint16_t
reverse16(uint16_t x)
{
  x = (x & 0xFF00) >> 8 | (x & 0x00FF) << 8;
  x = (x & 0xF0F0) >> 4 | (x & 0x0F0F) << 4;
  x = (x & 0xCCCC) >> 2 | (x & 0x3333) << 2;
  x = (x & 0xAAAA) >> 1 | (x & 0x5555) << 1;
  return x;
}
constexpr uint32_t
reverse(uint32_t x)
{
  x = (x >> 16) | (x << 16);
  x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
  x = ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4);
  x = ((x & 0xCCCCCCCC) >> 2) | ((x & 0x33333333) << 2);
  x = ((x & 0xAAAAAAAA) >> 1) | ((x & 0x55555555) << 1);
  return x;
}
constexpr uint64_t
reverse64(uint64_t x)
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
parity8(uint8_t x)
{
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}
constexpr int
parity16(uint16_t x)
{
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}
constexpr int
parity(uint32_t x)
{
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}
constexpr int
parity64(uint64_t x)
{
  x ^= x >> 32;
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}
constexpr uint32_t
swap_bits(uint32_t x, uint32_t mask, int shift)
{
  uint32_t t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}
constexpr uint64_t
swap_bits64(uint64_t x, uint64_t mask, int shift)
{
  uint64_t t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}

};
};
