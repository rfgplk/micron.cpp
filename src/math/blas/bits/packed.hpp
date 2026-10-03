//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Packed kernels borrow both native float/double and GCC _Float32/_Float64 storage.
// Scalar accesses need the same alias permission as the intrinsic vector loads.

namespace micron::math::blas
{
using __packed_f32 [[gnu::may_alias]] = float;
using __packed_f64 [[gnu::may_alias]] = double;
};      // namespace micron::math::blas
