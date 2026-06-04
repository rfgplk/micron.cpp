//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector.hpp"

#include "sort.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// extraction adaptor
//   produce a SORTED micron::vector from container kinds that cannot sort in
//   place (maps unordered/keyed, trees already ordered, heaps extract-only)

namespace micron
{
namespace sort
{

template<typename C>
concept __range_harvestable = !is_extractable_heap<C> && requires(const micron::remove_cvref_t<C> &c) {
  c.begin();
  c.end();
  { *c.begin() };
};

template<typename C>
concept __for_each_harvestable = !is_extractable_heap<C> && requires {
  typename micron::remove_cvref_t<C>::value_type;
} && !requires(const micron::remove_cvref_t<C> &c) {
  c.begin();
  c.end();
} && requires(const micron::remove_cvref_t<C> &c) { c.for_each([](const typename micron::remove_cvref_t<C>::value_type &) { }); };

template<__range_harvestable C, typename Cmp>
auto
sorted(const C &c, Cmp comp) -> micron::vector<micron::remove_cvref_t<decltype(*c.begin())>>
{
  using E = micron::remove_cvref_t<decltype(*c.begin())>;
  micron::vector<E> out;
  for ( auto it = c.begin(); it != c.end(); ++it ) out.push_back(*it);
  sort(out, comp);
  return out;
}

template<__range_harvestable C>
auto
sorted(const C &c) -> micron::vector<micron::remove_cvref_t<decltype(*c.begin())>>
{
  return sorted(c, [](const auto &a, const auto &b) { return a < b; });
}

template<__for_each_harvestable C, typename Cmp>
auto
sorted(const C &c, Cmp comp) -> micron::vector<typename micron::remove_cvref_t<C>::value_type>
{
  using E = typename micron::remove_cvref_t<C>::value_type;
  micron::vector<E> out;
  c.for_each([&](const E &e) { out.push_back(e); });
  sort(out, comp);
  return out;
}

template<__for_each_harvestable C>
auto
sorted(const C &c) -> micron::vector<typename micron::remove_cvref_t<C>::value_type>
{
  return sorted(c, [](const auto &a, const auto &b) { return a < b; });
}

template<typename C, typename Out, typename Cmp>
  requires(__range_harvestable<C> || __for_each_harvestable<C>)
void
sort_into(const C &c, Out &out, Cmp comp)
{
  auto s = sorted(c, comp);
  for ( auto &e : s ) out.push_back(e);
}

template<typename C, typename Out>
  requires(__range_harvestable<C> || __for_each_harvestable<C>)
void
sort_into(const C &c, Out &out)
{
  auto s = sorted(c);
  for ( auto &e : s ) out.push_back(e);
}

template<is_extractable_heap H>
auto
drain_sorted(H &h) -> micron::vector<typename micron::remove_cvref_t<H>::value_type>
{
  using E = typename micron::remove_cvref_t<H>::value_type;
  micron::vector<E> out;
  while ( h.size() > 0 ) {
    if constexpr ( requires { h.extract_min(); } )
      out.push_back(h.extract_min());
    else if constexpr ( requires { h.get(); } )
      out.push_back(h.get());
    else
      out.push_back(h.pop());
  }
  return out;
}

};      // namespace sort
};      // namespace micron
