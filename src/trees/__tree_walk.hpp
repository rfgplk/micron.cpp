//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"

namespace micron
{

//  keep going, do not descned into this node's subtree, abort traversal
enum class walk_ctl { continue_, skip_children, stop };

// traversal order for trees that support more than one (rb_tree)
enum class traversal_order { inorder, preorder, postorder, level };

namespace __impl
{
template<class Fn, class... A>
constexpr walk_ctl
invoke_walk(Fn &fn, A &&...a)
{
  using R = micron::invoke_result_t<Fn, A...>;
  if constexpr ( micron::is_same_v<R, void> ) {
    fn(micron::forward<A>(a)...);
    return walk_ctl::continue_;
  } else if constexpr ( micron::is_same_v<R, walk_ctl> ) {
    return fn(micron::forward<A>(a)...);
  } else {
    return fn(micron::forward<A>(a)...) ? walk_ctl::continue_ : walk_ctl::stop;
  }
}
};      // namespace __impl

};      // namespace micron
