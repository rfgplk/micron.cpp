//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math.hpp"
#include "../../src/math/fft/parallel.hpp"
namespace ft = micron::math::fft;
static_assert(sizeof(micron::math::complex<double>) == 2 * sizeof(double));
static_assert(noexcept(micron::math::conj(micron::math::complex<double>{})));
static_assert(ft::fft_scalar<float> && ft::fft_scalar<double> && ft::fft_scalar<long double>);
static_assert(ft::__impl::__epsilon<float>() == 0x1p-23f);
static_assert(ft::__impl::__epsilon<double>() == 0x1p-52);
#if defined(__micron_ldbl_binary64)
static_assert(ft::__impl::__epsilon<long double>() == 0x1p-52L);
#elif defined(__micron_ldbl_x87_80)
static_assert(ft::__impl::__epsilon<long double>() == 0x1p-63L);
#elif defined(__micron_ldbl_binary128)
static_assert(ft::__impl::__epsilon<long double>() == 0x1p-112L);
#endif

template<typename F>
void
fft_compile(F *real, micron::math::complex<F> *data)
{
  ft::plan<F> p(17);
  ft::workspace<F> w(p);
  p.execute(data, data, w.view());
  ft::descriptor<F> d({ 3, 8 });
  d.kind = ft::domain::real_forward;
  ft::plan<F> r(d);
  ft::workspace<F> rw(r);
  r.execute(real, data, rw.view());
  ft::r2r_plan<F> cosine(8, ft::r2r_kind::dct_iv);
  ft::workspace<F> cw(cosine);
  cosine.execute(real, real, cw.view());
}

template void fft_compile<float>(float *, micron::math::complex<float> *);
template void fft_compile<double>(double *, micron::math::complex<double> *);
template void fft_compile<long double>(long double *, micron::math::complex<long double> *);
