#pragma once

// for convenience

#include "constexprarray.hpp"

namespace micron
{
template <class T, usize N = 64> using constarray = constexpr_array<T, N>;
};
