//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../numerics.hpp"

namespace micron
{

static const usize npos = numeric_limits<usize>::max();
};

using const_schar = const char;
using schar = char;     // TODO: think about removing this
using wide = wchar_t;
using unicode8 = char8_t;
using unicode16 = char16_t;
using unicode32 = char32_t;

constexpr const char _null_str[1] = "";
constexpr const wide _null_wstr[1] = L"";
constexpr const char8_t _null_u8str[1] = u8"";
constexpr const unicode16 _null_u16str[1] = u"";
constexpr const unicode32 _null_u32str[1] = U"";
