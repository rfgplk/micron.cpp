//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "__visit_kv.hpp"

namespace micron
{
namespace __impl
{

template<typename __m_out>
constexpr __m_out
make_empty_like(usize hint)
{
  if constexpr ( requires { __m_out(hint); } )
    return __m_out(hint ? hint : usize{ 1 });
  else
    return __m_out{};
}

template<typename __m_out, typename __m_src, typename K, typename Xf>
__m_out
rebuild_map(const __m_src &src, K keep, Xf xf)
{
  if constexpr ( is_persistent_map<__m_out> ) {
    __m_out out{};
    __impl::visit_kv(src, [&](const auto &k, const auto &v) {
      if ( keep(k, v) ) out = out.insert(k, xf(k, v));
    });
    return out;
  } else {
    __m_out out = make_empty_like<__m_out>(src.size());
    __impl::visit_kv(src, [&](const auto &k, const auto &v) {
      if ( keep(k, v) ) out.insert(k, xf(k, v));
    });
    return out;
  }
}

inline constexpr auto __kv_value = [](const auto &, const auto &v) -> const auto & { return v; };

}      // namespace __impl
}      // namespace micron
