//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{
namespace fp
{

struct division_by_zero_error {
  const char *
  what() const noexcept
  {
    return "fp: division by zero";
  }
};

struct empty_container_error {
  const char *
  what() const noexcept
  {
    return "fp: operation on empty container";
  }
};

struct index_out_of_bounds_error {
  const char *
  what() const noexcept
  {
    return "fp: index out of bounds";
  }
};

struct bad_zip_error {
  const char *
  what() const noexcept
  {
    return "fp: zip of containers with mismatched sizes";
  }
};

struct traverse_error {
  const char *
  what() const noexcept
  {
    return "fp: traverse: inner function returned error branch";
  }
};

};     // namespace fp
};     // namespace micron
