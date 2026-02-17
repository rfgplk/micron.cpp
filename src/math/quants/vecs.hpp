//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../__special/initializer_list"
#include "../../types.hpp"

#include "v_types/vec2.hpp"

#include "v_types/vec3.hpp"

#include "v_types/vec4.hpp"

#include "v_types/vec8.hpp"

#include "v_types/vec16.hpp"

namespace micron
{
using vec2 = vector_2<f32>;
using vec3 = vector_3<f32>;
using vec4 = vector_4<f32>;
using quat = vector_4<f32>;
using vec8 = vector_8<f32>;
using vec16 = vector_16<f32>;
using vec2f = vec2;
using vec3f = vec3;
using vec4f = vec4;
using vec8f = vec8;
using vec16f = vec16;

using dvec2 = vector_2<f64>;
using dvec3 = vector_3<f64>;
using dvec4 = vector_4<f64>;
using dvec8 = vector_8<f64>;
using dquat = vector_4<f64>;
using dvec16 = vector_16<f64>;
using vec2d = dvec2;
using vec3d = dvec3;
using vec4d = dvec4;
using vec8d = dvec8;
using vec16d = dvec16;

using dlvec2 = vector_2<f128>;
using dlvec3 = vector_3<f128>;
using dlvec4 = vector_4<f128>;
using dlvec8 = vector_8<f128>;
using dlquat = vector_4<f128>;
using dlvec16 = vector_16<f128>;
using vec2dl = dlvec2;
using vec3dl = dlvec3;
using vec4dl = dlvec4;
using vec8dl = dlvec8;
using vec16dl = dlvec16;

};
