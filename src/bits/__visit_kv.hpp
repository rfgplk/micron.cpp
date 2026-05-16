//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

namespace micron
{
namespace __impl
{

// a uniform visitor over any micron map class
// defaults to for_each where fn is invocable with (const K&, V&)
template<typename M, typename Fn>
__attribute__((always_inline)) constexpr void
visit_kv(M &m, Fn &&fn)
{
  using K = typename micron::remove_cvref_t<M>::key_type;
  using V_ = typename micron::remove_cvref_t<M>::mapped_type;
  using V = micron::conditional_t<micron::is_const_v<M>, const V_, V_>;
  if constexpr ( requires { m.for_each([&](const K &, V &) { }); } ) {
    m.for_each([&](const K &k, V &v) { fn(k, v); });
  } else {
    auto it = m.begin();
    auto en = m.end();
    for ( ; it != en; ++it ) {
      auto &&e = *it;
      if constexpr ( requires {
                       e.key;
                       e.value;
                     } ) {
        fn(e.key, e.value);
      } else if constexpr ( requires {
                              e.a;
                              e.b;
                            } ) {
        fn(e.a, e.b);
      }
    }
  }
}

}      // namespace __impl
}      // namespace micron
