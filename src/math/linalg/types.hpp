//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../matrix/dynmat.hpp"
#include "../matrix/mat.hpp"
#include "../quants/dynvec.hpp"
#include "../quants/quat.hpp"
#include "../quants/vec.hpp"

namespace micron
{
namespace math
{
namespace linalg
{

using micron::math::arith_scalar;

template<typename T, usize N> using vec = micron::math::vec<T, N>;

template<typename T, usize R, usize C> using mat = micron::math::mat<T, R, C>;

template<typename F> using quat = micron::math::quat<F>;

template<typename T> using dynmat = micron::math::dynmat<T>;

template<typename T> using dynvec = micron::math::dynvec<T>;

};      // namespace linalg
};      // namespace math
};      // namespace micron
