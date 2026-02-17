//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
namespace io
{

// NOTE: this is encap. as a struct so we have an easy unified way of managing fd related errors

struct fd_t {
  i32 fd;
  ~fd_t() = default;
  fd_t(void) : fd{} {};
  fd_t(i32 x) : fd(x) {};
  fd_t(const fd_t &) = default;
  fd_t(fd_t &&) = default;
  fd_t &operator=(const fd_t &) = default;
  fd_t &operator=(fd_t &&) = default;

  inline bool
  closed() const
  {
    return fd == -1;
  }

  inline bool
  open() const
  {
    return fd >= 0;
  }

  inline auto
  has_error() const -> u32
  {
    if ( fd < 0 )
      return fd * -1;
    return 0;
  }

  inline void
  reset()
  {
    fd = -1;
  }

  bool
  operator==(const fd_t &o) const
  {
    return (fd == o.fd);
  }
};
};

};
