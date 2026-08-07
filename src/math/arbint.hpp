//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// arbitrary-precision integers
//
//   micron::math::arbint<>                     signed, dynamic width, heap allocd
//   micron::math::arbuint<>                    unsigned, likewise
//   micron::math::arbint<2048>                 signed, magnitude capped at 2048 bits, inline stack allocd, constexpr
//   micron::math::arbint<256, solver::comba>   multiply tier pinned instead of size dispatched
//
// multiplication ladder:
// -> Comba
// -> Karatsuba
// -> Toom-Cook
// -> Nussbaumer, picked by a constexpr

#include "arbint/convert.hpp"
#include "arbint/div.hpp"
#include "arbint/div_dc.hpp"
#include "arbint/div_mu.hpp"
#include "arbint/gcd.hpp"
#include "arbint/gcd_base.hpp"
#include "arbint/gcdext.hpp"
#include "arbint/hgcd.hpp"
#include "arbint/hgcd2.hpp"
#include "arbint/limb.hpp"
#include "arbint/modular.hpp"
#include "arbint/mont.hpp"
#include "arbint/mpn_core.hpp"
#include "arbint/mul.hpp"
#include "arbint/mul_basecase.hpp"
#include "arbint/mul_karatsuba.hpp"
#include "arbint/mul_toom.hpp"
#include "arbint/number.hpp"
#include "arbint/powm.hpp"
#include "arbint/signed.hpp"
#include "arbint/storage.hpp"
#include "arbint/tags.hpp"
#include "arbint/thresholds.hpp"
#include "arbint/traits.hpp"
#include "arbint/unsigned.hpp"

namespace micron
{

using math::arbint;
using math::arbuint;

namespace solver = math::solver;

};      // namespace micron
