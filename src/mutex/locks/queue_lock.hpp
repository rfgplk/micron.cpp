//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../sync/yield.hpp"

namespace micron
{

class scoped_lock
{
  queuing_mutex *mtx = nullptr;
  mcs_node node;
  bool held = false;

public:
  explicit scoped_lock(queuing_mutex &m) : mtx(&m), held(false)
  {
    m(node);
    held = true;
  }

  scoped_lock() = default;

  ~scoped_lock()
  {
    if ( held && mtx )
      mtx->unlock(node);
  }

  void
  acquire(queuing_mutex &m)
  {
    mtx = &m;
    m(node);
    held = true;
  }

  bool
  try_acquire(queuing_mutex &m)
  {
    mtx = &m;
    held = m.try_lock(node);
    return held;
  }

  void
  release()
  {
    if ( held && mtx ) {
      mtx->unlock(node);
      held = false;
    }
  }

  bool
  owns() const noexcept
  {
    return held;
  }

  scoped_lock(const scoped_lock &) = delete;
  scoped_lock(scoped_lock &&) = delete;
  scoped_lock &operator=(const scoped_lock &) = delete;
  scoped_lock &operator=(scoped_lock &&) = delete;
};

using queue_lock = queuing_mutex;

}     // namespace micron
