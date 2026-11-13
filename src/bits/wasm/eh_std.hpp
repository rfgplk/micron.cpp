//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace std
{
class exception
{
public:
  exception() noexcept = default;
  exception(const exception &) noexcept = default;
  exception &operator=(const exception &) noexcept = default;
  virtual ~exception() = default;

  virtual const char *
  what() const noexcept
  {
    return "std::exception";
  }
};

class bad_cast: public exception
{
public:
  const char *
  what() const noexcept override
  {
    return "std::bad_cast";
  }
};

class bad_typeid: public exception
{
public:
  const char *
  what() const noexcept override
  {
    return "std::bad_typeid";
  }
};
}      // namespace std
