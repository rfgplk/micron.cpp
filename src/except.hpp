//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{
namespace except
{
class _base_exception
{
public:
  _base_exception() noexcept = default;
  virtual ~_base_exception() = default;

  virtual const char *
  what() const noexcept
  {
    return "_base_exception";
  };
};
// unclean but the only way to avoid senseless boilterplate ;c
#define MICRON_EXCEPTION_TEMP(_str)                                                                                     \
  class _str : public _base_exception                                                                                   \
  {                                                                                                                     \
    const char *_what;                                                                                                  \
                                                                                                                        \
  public:                                                                                                               \
    explicit _str(const char *w) : _what(w) {}                                                                          \
    const char *                                                                                                        \
    what() const noexcept override                                                                                      \
    {                                                                                                                   \
      return _what;                                                                                                     \
    }                                                                                                                   \
  };
MICRON_EXCEPTION_TEMP(domain_error)
MICRON_EXCEPTION_TEMP(invalid_argument)
MICRON_EXCEPTION_TEMP(length_error)
MICRON_EXCEPTION_TEMP(logic_error)
MICRON_EXCEPTION_TEMP(out_of_range_error)
MICRON_EXCEPTION_TEMP(overflow_error)
MICRON_EXCEPTION_TEMP(range_error)
MICRON_EXCEPTION_TEMP(runtime_error)
MICRON_EXCEPTION_TEMP(underflow_error)
MICRON_EXCEPTION_TEMP(future_error)
MICRON_EXCEPTION_TEMP(system_error)
MICRON_EXCEPTION_TEMP(io_error)
MICRON_EXCEPTION_TEMP(format_error)

// micron non-standard errors
MICRON_EXCEPTION_TEMP(standard_error)
MICRON_EXCEPTION_TEMP(library_error)
MICRON_EXCEPTION_TEMP(hardware_error)
MICRON_EXCEPTION_TEMP(memory_error)
MICRON_EXCEPTION_TEMP(filesystem_error)
};
using domain = except::domain_error;
using invalid = except::invalid_argument;
using length = except::length_error;
using logic = except::logic_error;
using oor = except::out_of_range_error;
using overflow = except::overflow_error;
using range_error = except::range_error;
using runtime = except::runtime_error;
using underflow = except::underflow_error;
using future = except::future_error;
using system = except::system_error;
using io_err = except::io_error;
using format_err = except::format_error;
using standard = except::standard_error;
using library = except::library_error;
using hardware = except::hardware_error;
using fs_error = except::filesystem_error;
};
