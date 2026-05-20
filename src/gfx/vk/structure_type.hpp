//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{
namespace gfx
{
namespace vk
{

template<typename T> struct structure_type_of;

template<typename T> inline constexpr auto structure_type_of_v = structure_type_of<T>::value;

};      // namespace vk
};      // namespace gfx
};      // namespace micron
