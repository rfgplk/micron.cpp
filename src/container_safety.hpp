#pragma once

namespace micron
{
typedef unsigned char Safe;
typedef double Unsafe;
template <typename T>
concept is_safe = std::same_as<T, Safe>;
template <typename T>
concept is_unsafe = std::same_as<T, Unsafe>;
constexpr bool
safe(auto s)
{
  return std::is_same_v<decltype(s), Safe>;
};
};     // namespace micron
