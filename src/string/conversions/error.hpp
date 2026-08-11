//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../errno.hpp"
#include "../../types.hpp"

namespace micron
{
namespace format
{

struct parse_error {
  i32 code = 0;

  constexpr parse_error() = default;

  constexpr explicit parse_error(i32 e) : code(e < 0 ? static_cast<i32>(0u - static_cast<u32>(e)) : e) { }

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

// no valid number, or trailing garbage after one
constexpr static const parse_error parse_malformed{ micron::error::invalid_arg };
// a well-formed literal whose magnitude is not representable
constexpr static const parse_error parse_out_of_range{ micron::error::not_representable };

};      // namespace format
};      // namespace micron
