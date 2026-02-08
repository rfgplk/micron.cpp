#pragma once

// for convenience

#include "constexprarray.hpp"

namespace micron
{
template <class T, size_t N = 64> using constarray = constexpr_array<T, N>;
};
