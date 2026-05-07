//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Gauss-Kronrod 15/7 pair on [-1, 1]

#include "../../../../types.hpp"
#include "../../../ieee.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace coeff
{
namespace gk
{

template <ieee754_floating F> struct gk_15_7 {
  static constexpr usize half = 8;
  static constexpr bool has_zero = true;

  static constexpr F nodes[8] = {
    F(0.0L),
    F(0.207784955007898467600689403773245L),
    F(0.405845151377397166906606412076961L),
    F(0.586087235467691130294144838258730L),
    F(0.741531185599394439863864773280788L),
    F(0.864864423359769072789712788640926L),
    F(0.949107912342758524526189684047851L),
    F(0.991455371120812639206854697526329L),
  };

  static constexpr F wk[8] = {
    F(0.209482141084727828012999174891714L), F(0.204432940075298892414161999234649L), F(0.190350578064785409913256402421014L),
    F(0.169004726639267902826583426598550L), F(0.140653259715525918745189590510238L), F(0.104790010322250183839876322541518L),
    F(0.063092092629978553290700663189204L), F(0.022935322010529224963732008058970L),
  };

  // Gauss-7 weights: non-zero only at Kronrod indices that coincide
  // with a Gauss node (every other one starting at 0).
  static constexpr F wg[8] = {
    F(0.417959183673469387755102040816327L),     // x = 0
    F(0.0L),
    F(0.381830050505118944950369775488975L),     // x ≈ 0.40585
    F(0.0L),
    F(0.279705391489276667901467771423780L),     // x ≈ 0.74153
    F(0.0L),
    F(0.129484966168869693270611432679082L),     // x ≈ 0.94911
    F(0.0L),                                     // Kronrod-only
  };
};

};     // namespace gk
};     // namespace coeff
};     // namespace integrate
};     // namespace math
};     // namespace micron
