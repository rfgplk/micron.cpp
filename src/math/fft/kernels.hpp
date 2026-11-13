//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "packet.hpp"

namespace micron::math::fft::__impl
{
// each packet lane is an ind. butterfly
// NOTE: no horizontal reductions or materialized real/imaginary arrays are needed between loads and stores
template<usize R, bool Inverse, typename P, fft_scalar F>
[[gnu::always_inline]] inline void
__codelet(const typename P::value *v, complex<F> *out, usize stride) noexcept
{
  if constexpr ( R == 2 ) {
    P::store(out, P::add(v[0], v[1]));
    P::store(out + stride, P::sub(v[0], v[1]));
  } else if constexpr ( R == 3 ) {
    auto sum = P::add(v[1], v[2]);
    auto center = P::sub(v[0], P::scale(sum, F(0.5)));
    auto difference
        = P::template rotate<Inverse>(P::scale(P::sub(v[1], v[2]), F(0.866025403784438646763723170752936183471402626905190314L)));
    P::store(out, P::add(v[0], sum));
    P::store(out + stride, P::add(center, difference));
    P::store(out + 2 * stride, P::sub(center, difference));
  } else if constexpr ( R == 4 ) {
    auto a = P::add(v[0], v[2]), b = P::sub(v[0], v[2]);
    auto c = P::add(v[1], v[3]), d = P::template rotate<Inverse>(P::sub(v[1], v[3]));
    P::store(out, P::add(a, c));
    P::store(out + 2 * stride, P::sub(a, c));
    P::store(out + stride, P::add(b, d));
    P::store(out + 3 * stride, P::sub(b, d));
  } else if constexpr ( R == 5 ) {
    constexpr F c1 = F(0.309016994374947424102293417182819058860154589902881431L);
    constexpr F c2 = F(-0.809016994374947424102293417182819058860154589902881431L);
    constexpr F s1 = F(0.951056516295153572116439333379382143405698634125750222L);
    constexpr F s2 = F(0.587785252292473129168705954639072768597652437643145991L);
    auto p = P::add(v[1], v[4]), q = P::add(v[2], v[3]);
    auto d = P::sub(v[1], v[4]), e = P::sub(v[2], v[3]);
    P::store(out, P::add(P::add(v[0], p), q));
    auto a = P::add(v[0], P::add(P::scale(p, c1), P::scale(q, c2)));
    auto c = P::template rotate<Inverse>(P::add(P::scale(d, s1), P::scale(e, s2)));
    P::store(out + stride, P::add(a, c));
    P::store(out + 4 * stride, P::sub(a, c));
    auto b = P::add(v[0], P::add(P::scale(p, c2), P::scale(q, c1)));
    auto f = P::template rotate<Inverse>(P::sub(P::scale(d, s2), P::scale(e, s1)));
    P::store(out + 2 * stride, P::add(b, f));
    P::store(out + 3 * stride, P::sub(b, f));
  } else if constexpr ( R == 8 ) {
    constexpr F h = F(0.707106781186547524400844362104849039284835937688474037L);
    auto a = P::add(v[0], v[4]), b = P::sub(v[0], v[4]);
    auto c = P::add(v[2], v[6]), d = P::template rotate<Inverse>(P::sub(v[2], v[6]));
    auto e0 = P::add(a, c), e2 = P::sub(a, c), e1 = P::add(b, d), e3 = P::sub(b, d);
    a = P::add(v[1], v[5]);
    b = P::sub(v[1], v[5]);
    c = P::add(v[3], v[7]);
    d = P::template rotate<Inverse>(P::sub(v[3], v[7]));
    auto t = P::add(a, c);
    P::store(out, P::add(e0, t));
    P::store(out + 4 * stride, P::sub(e0, t));
    t = P::template rotate<Inverse>(P::sub(a, c));
    P::store(out + 2 * stride, P::add(e2, t));
    P::store(out + 6 * stride, P::sub(e2, t));
    t = P::add(b, d);
    t = P::scale(P::add(t, P::template rotate<Inverse>(t)), h);
    P::store(out + stride, P::add(e1, t));
    P::store(out + 5 * stride, P::sub(e1, t));
    t = P::sub(b, d);
    t = P::scale(P::sub(P::template rotate<Inverse>(t), t), h);
    P::store(out + 3 * stride, P::add(e3, t));
    P::store(out + 7 * stride, P::sub(e3, t));
  } else {
    static_assert(R == 7);
    constexpr F c[3]
        = { F(0.623489801858733530525004884004239810632274730896402106L), F(-0.222520933956314404288902564496794759466355568764544955L),
            F(-0.900968867902419126236102319507445051165919162131857151L) };
    constexpr F s[3]
        = { F(0.781831482468029808708444526674057750232334518708687529L), F(0.974927912181823607018131682993931217232785800619997438L),
            F(0.433883739117558120475768332848358754609990727787459876L) };
    typename P::value sums[3], differences[3];
    auto dc = v[0];
    for ( usize j = 0; j < 3; ++j ) {
      sums[j] = P::add(v[j + 1], v[6 - j]);
      differences[j] = P::sub(v[j + 1], v[6 - j]);
      dc = P::add(dc, sums[j]);
    }
    P::store(out, dc);
    for ( usize l = 1; l <= 3; ++l ) {
      auto a = P::add(v[0], P::scale(sums[0], c[l - 1]));
      auto b = P::scale(differences[0], s[l - 1]);
      for ( usize j = 1; j < 3; ++j ) {
        usize phase = (l * (j + 1)) % 7, index = phase > 3 ? 6 - phase : phase - 1;
        a = P::add(a, P::scale(sums[j], c[index]));
        b = P::add(b, P::scale(differences[j], phase > 3 ? -s[index] : s[index]));
      }
      b = P::template rotate<Inverse>(b);
      P::store(out + l * stride, P::add(a, b));
      P::store(out + (7 - l) * stride, P::sub(a, b));
    }
  }
}

template<usize R, bool Inverse, typename P, fft_scalar F>
[[gnu::always_inline]] inline void
__stage_packet(complex<F> *out, const complex<F> *twiddles, usize m, usize k) noexcept
{
  typename P::value v[R];
  v[0] = P::load(out + k);
  for ( usize j = 1; j < R; ++j ) {
    auto w = P::load(twiddles + (j - 1) * m + k);
    if constexpr ( Inverse ) w = P::conjugate(w);
    v[j] = P::mul(P::load(out + j * m + k), w);
  }
  __codelet<R, Inverse, P>(v, out + k, m);
}

template<usize R, bool Inverse, fft_scalar F>
void
__stage(complex<F> *out, const complex<F> *twiddles, usize m, usize begin, usize end) noexcept
{
  using P = __complex_packet<F>;
  if ( begin >= end ) return;
  const usize vector_end = end - (end - begin) % P::width;
  usize k = begin;
  for ( ; k < vector_end; k += P::width ) __stage_packet<R, Inverse, P>(out, twiddles, m, k);
  if constexpr ( P::width > 1 )
    for ( usize tail = 0; tail < (end - begin) % P::width; ++tail )
      __stage_packet<R, Inverse, __scalar_packet<F>>(out, twiddles, m, vector_end + tail);
}

template<usize R, bool Inverse, fft_scalar F>
[[gnu::noinline]] void
__leaf_large(const complex<F> *in, complex<F> *out, usize stride) noexcept
{
  using P = __scalar_packet<F>;
  typename P::value v[R];
  for ( usize j = 0; j < R; ++j ) v[j] = in[j * stride];
  __codelet<R, Inverse, P>(v, out, 1);
}

template<usize R, bool Inverse, fft_scalar F>
[[gnu::always_inline]] inline void
__leaf(const complex<F> *in, complex<F> *out, usize stride) noexcept
{
  if constexpr ( R > 4 ) {
    // NOTE: keep the larger codelets live ranges and stack frame out of the recursive radix-2/4 dispatcher
    // they are reached only at the leaves
    __leaf_large<R, Inverse>(in, out, stride);
  } else {
    using P = __scalar_packet<F>;
    typename P::value v[R] = { in[0], in[stride] };
    if constexpr ( R >= 3 ) v[2] = in[2 * stride];
    if constexpr ( R == 4 ) v[3] = in[3 * stride];
    __codelet<R, Inverse, P>(v, out, 1);
  }
}
};      // namespace micron::math::fft::__impl
