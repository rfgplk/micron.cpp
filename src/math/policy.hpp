//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{
namespace math
{
namespace policy
{

// ieee     = strict IEEE 754 result
// cr       = rounded via Ziv onion-peeling
// faithful = default; error < 1 ulp
// fast     = <= 4 ulp via bipartite tables mainly

struct ieee_tag {
};

struct cr_tag {
};

struct faithful_tag {
};

struct fast_tag {
};

inline constexpr ieee_tag ieee{};
inline constexpr cr_tag cr{};
inline constexpr faithful_tag faithful{};
inline constexpr fast_tag fast{};

template<typename T>
concept policy_tag = micron::is_same_v<T, ieee_tag> or micron::is_same_v<T, cr_tag> or micron::is_same_v<T, faithful_tag>
                     or micron::is_same_v<T, fast_tag>;

};      // namespace policy
};      // namespace math
};      // namespace micron
