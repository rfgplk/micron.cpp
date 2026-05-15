//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"

namespace micron
{
namespace math
{
namespace blas
{

namespace op
{
struct none {
};

struct trans {
};

struct conj_trans {
};

template<typename T>
concept op_tag = micron::same_as<T, none> or micron::same_as<T, trans> or micron::same_as<T, conj_trans>;
};      // namespace op

namespace side
{
struct left {
};

struct right {
};

template<typename T>
concept side_tag = micron::same_as<T, left> or micron::same_as<T, right>;
};      // namespace side

namespace uplo
{
struct lower {
};

struct upper {
};

template<typename T>
concept uplo_tag = micron::same_as<T, lower> or micron::same_as<T, upper>;
};      // namespace uplo

namespace diag
{
struct non_unit {
};

struct unit {
};

template<typename T>
concept diag_tag = micron::same_as<T, non_unit> or micron::same_as<T, unit>;
};      // namespace diag

};      // namespace blas
};      // namespace math
};      // namespace micron
