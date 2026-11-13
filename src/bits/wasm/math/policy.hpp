// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../type_traits.hpp"

namespace micron::math::policy
{
struct faithful_tag;

// Full binary128 range and arithmetic precision. Transcendental error is measured
// over a fixed corpus; this tag makes no global faithful/correct-rounding promise.
struct extended_tag {
};

inline constexpr extended_tag extended{};
template<typename F> using default_tag = micron::conditional_t<sizeof(F) == 16, extended_tag, faithful_tag>;

template<typename F, typename P>
inline constexpr void
__validate_extended() noexcept
{
  static_assert(sizeof(F) != 16 || micron::is_same_v<P, extended_tag>,
                "micron: binary128 strict transcendental policies are unavailable; use policy::extended or the default");
}
}      // namespace micron::math::policy
