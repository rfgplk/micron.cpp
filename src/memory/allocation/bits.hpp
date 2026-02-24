#pragma once

namespace micron
{
template <size_t Sz = page_size>
constexpr inline size_t
to_page(size_t n)
{
  if ( n % Sz != 0 )
    n += Sz - (n % Sz);
  return n;
}

template <u32 G>
inline constexpr size_t
to_granularity(size_t n)
{
  if ( n % G != 0 )
    n += G - (n % G);
  return n;
}
};     // namespace micron
