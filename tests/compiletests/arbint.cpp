

#include "../../src/math/arbint.hpp"

constexpr u64
fold()
{
  micron::math::arbuint<256> a(1234567891011ull);
  a *= micron::math::arbuint<256>(999983u);
  a /= micron::math::arbuint<256>(999983u);
  micron::math::arbuint<100> w(0u);
  w -= micron::math::arbuint<100>(1u);

  const micron::math::arbuint<256> m(1000003u);
  const micron::math::arbuint<256> pm = micron::math::powmod(micron::math::arbuint<256>(7u), micron::math::arbuint<256>(5u), m);
  const micron::math::arbuint<256> gg = micron::math::gcd(micron::math::arbuint<256>(1071u), micron::math::arbuint<256>(462u));

  return static_cast<u64>(a) + w.popcount() + static_cast<u64>(pm) + static_cast<u64>(gg);
}

static_assert(fold() == 1234567891011ull + 100ull + 16807ull + 21ull, "arbint must be constexpr-clean on every target");

int
main()
{
  using micron::math::arbint;
  using micron::math::arbuint;

  arbuint<> u(123456789u), v(987654321u);
  u += v;
  u -= v;
  u *= v;
  u /= v;
  u %= v;
  u <<= 3;
  u >>= 2;
  u &= v;
  u |= v;
  u ^= v;
  ++u;
  --u;
  u = u + v - v * arbuint<>(1u);
  volatile bool ord = (u < v) || (u >= v) || (u == 1u) || (2u > v);

  arbint<> a(-42), b(9001);
  a += b;
  a -= b;
  a *= b;
  a /= b;
  a %= b;
  a <<= 5;
  a >>= 3;
  a &= b;
  a |= b;
  a ^= b;
  a = -a;
  a = ~a;
  a = micron::math::abs(a);
  a = micron::math::pow(a, 3);

  arbuint<2048> k(7u);
  k = micron::math::sqr(k);
  arbint<512> s(-7);
  s = s * arbint<512>(3) + arbint<512>(1);
  arbuint<521> odd(1u);
  odd -= arbuint<521>(2u);

  arbuint<256, micron::math::solver::comba> pc(3u);
  arbuint<0, micron::math::solver::basecase> pb(5u);
  pc = pc * pc;
  pb = pb * pb;

  const arbuint<> dm(1000003u);
  micron::math::montgomery<arbuint<>> mgd(dm);
  micron::math::barrett<arbuint<>> brd(dm);
  const arbuint<2048> bm(1000003u);
  micron::math::montgomery<arbuint<2048>> mgb(bm);
  micron::math::barrett<arbuint<2048>> brb(bm);
  micron::math::barrett<arbuint<521>> brp(arbuint<521>(1000003u));

  const arbuint<> mf = mgd.to_form(arbuint<>(12345u));
  const arbuint<> mr = mgd.from_form(mf);
  const arbuint<> mm2 = mgd.mul(mf, mf);
  const arbuint<> ms = mgd.sqr(mf);
  const arbuint<> mp1 = mgd.pow(arbuint<>(7u), arbuint<>(5u));
  const arbuint<> mp2 = mgd.pow(arbuint<>(7u), 5ull);
  const arbuint<> br1 = brd.reduce(arbuint<>(2000009u));
  const arbuint<> br2 = brd.mul(arbuint<>(11u), arbuint<>(13u));
  const arbuint<> br3 = brd.sqr(arbuint<>(11u));
  const arbuint<> br4 = brd.pow(arbuint<>(7u), arbuint<>(5u));
  const arbuint<> br5 = brd.pow(arbuint<>(7u), 5ull);

  const arbuint<> pw1 = micron::math::powmod(arbuint<>(7u), arbuint<>(5u), dm);
  const arbuint<> pw2 = micron::math::powmod(arbuint<>(7u), 5ull, dm);
  const arbuint<> mm3 = micron::math::mulmod(arbuint<>(11u), arbuint<>(13u), dm);
  const arbuint<> sq3 = micron::math::sqrmod(arbuint<>(11u), dm);
  const arbint<> spw = micron::math::powmod(arbint<>(-7), arbint<>(5), arbint<>(1000003));

  micron::math::montgomery<arbuint<256, micron::math::solver::comba>> mgc(arbuint<256, micron::math::solver::comba>(1000003u));
  const auto mgcv = mgc.pow(arbuint<256, micron::math::solver::comba>(7u), 5ull);

  const arbuint<> g1 = micron::math::gcd(arbuint<>(1071u), arbuint<>(462u));
  const arbuint<> l1 = micron::math::lcm(arbuint<>(21u), arbuint<>(6u));
  const auto [iv, iok] = micron::math::invmod(arbuint<>(3u), dm);
  const arbint<> g2 = micron::math::gcd(arbint<>(-12), arbint<>(8));
  const arbint<> l2 = micron::math::lcm(arbint<>(-4), arbint<>(6));
  const auto [siv, siok] = micron::math::invmod(arbint<>(-3), arbint<>(1000003));

  const arbuint<521> g3 = micron::math::gcd(arbuint<521>(1071u), arbuint<521>(462u));

  char buf[512];
  const usize n = micron::math::to_chars(buf, sizeof buf, u, 16);
  arbuint<> parsed;
  const bool ok = micron::math::from_chars(parsed, buf, n, 16);
  const micron::string str = micron::math::to_string(a);

  micron::arbint<> re(1);
  micron::arbuint<> reu(1u);

  return static_cast<int>(static_cast<u64>(u) + static_cast<u64>(k) + static_cast<u64>(pc) + static_cast<u64>(odd.popcount())
                          + static_cast<u64>(n) + static_cast<u64>(ok) + str.size() + static_cast<u64>(ord) + static_cast<u64>(s.size())
                          + static_cast<u64>(re.size()) + static_cast<u64>(reu.size()) + static_cast<u64>(parsed.size())
                          + static_cast<u64>(pb.size()) + fold())
         & 0x7f;
}
