//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../maps/swiss.hpp"
#include "__set_common.hpp"

namespace micron
{

// swiss_set
//
// thin set adapter over stack_swiss_map<K, __set_empty_v, N, NH>; fixed capacity N, stack-allocated
template<typename K, usize N, usize NH = 16> class swiss_set
{
  using __map_t = stack_swiss_map<K, __set_empty_v, N, NH>;
  __map_t __m;

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  using key_type = K;
  using value_type = K;
  using size_type = usize;

  ~swiss_set() = default;
  swiss_set() = default;
  swiss_set(const swiss_set &) = default;
  swiss_set(swiss_set &&) noexcept = default;
  swiss_set &operator=(const swiss_set &) = default;
  swiss_set &operator=(swiss_set &&) noexcept = default;

  usize
  size() const noexcept
  {
    return __m.size();
  }

  bool
  empty() const noexcept
  {
    return __m.empty();
  }

  constexpr usize
  capacity() const noexcept
  {
    return N;
  }

  constexpr usize
  max_size() const noexcept
  {
    return N;
  }

  void
  clear() noexcept
  {
    __m.clear();
  }

  bool
  insert(const K &k)
  {
    return __m.insert(k, __set_empty_v{}).a;
  }

  bool
  insert(K &&k)
  {
    return __m.insert(micron::move(k), __set_empty_v{}).a;
  }

  template<typename... Args>
  bool
  emplace(Args &&...args)
  {
    K tmp(micron::forward<Args>(args)...);
    return insert(micron::move(tmp));
  }

  bool
  contains(const K &k) const
  {
    return __m.contains(k);
  }

  usize
  count(const K &k) const
  {
    return __m.count(k);
  }

  bool
  erase(const K &k)
  {
    return __m.erase(k);
  }
};

};      // namespace micron
