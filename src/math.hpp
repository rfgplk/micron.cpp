//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// main math include header
//
// NOTE: re-exports the public porcelain symbols into micron:: namespace
//
// so it's possible to write  micron::sin(x); instead of micron::math::sin et al

#include "math/activation.hpp"
#include "math/arith.hpp"
#include "math/bits.hpp"
#include "math/bits/impl.hpp"
#include "math/blas/blas.hpp"
#include "math/constants.hpp"
#include "math/cr.hpp"
#include "math/dd64.hpp"
#include "math/dispatch.hpp"
#include "math/generic.hpp"
#include "math/ieee.hpp"
#include "math/linalg.hpp"
#include "math/log.hpp"
#include "math/mk.hpp"
#include "math/numeric.hpp"
#include "math/policy.hpp"
#include "math/reduce.hpp"
#include "math/rng.hpp"
#include "math/splines.hpp"
#include "math/sqrt.hpp"

namespace micron
{

// trig
using math::acos;
using math::asin;
using math::atan;
using math::atan2;
using math::cos;
using math::sin;
using math::sincos;
using math::tan;

// CORDIC trig
using math::acos_cordic;
using math::asin_cordic;
using math::atan2_cordic;
using math::atan_cordic;
using math::cos_cordic;
using math::sin_cordic;
using math::sincos_cordic;
using math::tan_cordic;

// hyp
using math::acosh;
using math::asinh;
using math::atanh;
using math::cosh;
using math::sinh;
using math::tanh;

// exp / log
using math::exp;
using math::exp10;
using math::exp2;
using math::expm1;
using math::log;
using math::log10;
using math::log1p;
using math::log2;

// pow
using math::cbrt;
using math::hypot;
using math::pow;
using math::rsqrt;
using math::sqrt;

// round
using math::ceil;
using math::floor;
using math::nearbyint;
using math::rint;
using math::round;
using math::trunc;

// rem
using math::fmod;
using math::remainder;

// manip
using math::copysign;
using math::fabs;
using math::frexp;
using math::ilogb;
using math::ldexp;
using math::logb;
using math::nextafter;

// special
using math::erf;
using math::erfc;
using math::j0;
using math::j1;
using math::lgamma;
using math::tgamma;
using math::y0;
using math::y1;

// fused
using math::fms;
using math::fnma;

};     // namespace micron
