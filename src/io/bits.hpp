//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../linux/io.hpp"

namespace micron
{

namespace io
{
using posix::fd_t;

struct error_t {
  i32 code = 0;

  constexpr error_t() = default;

  constexpr explicit error_t(i32 e) : code(e < 0 ? static_cast<i32>(0u - static_cast<u32>(e)) : e) { }

  const char *
  message() const
  {
    return micron::error::get_errno(code);
  }

  constexpr explicit
  operator bool() const
  {
    return code != 0;
  }
};
};      // namespace io

};      // namespace micron
