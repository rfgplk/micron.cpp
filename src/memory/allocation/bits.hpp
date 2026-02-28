#pragma once

namespace micron
{
template <usize Sz = page_size>
constexpr inline usize
to_page(usize n)
{
  if ( n % Sz != 0 )
    n += Sz - (n % Sz);
  return n;
}

template <u32 G>
inline constexpr usize
to_granularity(usize n)
{
  if ( n % G != 0 )
    n += G - (n % G);
  return n;
}
};     // namespace micron
