//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__exceptions.hpp"

namespace micron
{
namespace except
{

// errnos of exceptions, all offset by 0x7f00
// what error code will the bin abort with

constexpr static const u32 domain_errno = 0x7f00 + 1;
constexpr static const u32 invalid_errno = 0x7f00 + 2;
constexpr static const u32 length_errno = 0x7f00 + 3;
constexpr static const u32 logic_errno = 0x7f00 + 4;
constexpr static const u32 oor_errno = 0x7f00 + 5;
constexpr static const u32 overflow_errno = 0x7f00 + 6;
constexpr static const u32 range_errno = 0x7f00 + 7;
constexpr static const u32 runtime_errno = 0x7f00 + 8;
constexpr static const u32 underflow_errno = 0x7f00 + 9;
constexpr static const u32 future_errno = 0x7f00 + 10;
constexpr static const u32 system_errno = 0x7f00 + 11;
constexpr static const u32 io_errno = 0x7f00 + 12;
constexpr static const u32 format_errno = 0x7f00 + 13;
constexpr static const u32 standard_errno = 0x7f00 + 14;
constexpr static const u32 library_errno = 0x7f00 + 15;
constexpr static const u32 hardware_errno = 0x7f00 + 16;
constexpr static const u32 memory_errno = 0x7f00 + 17;
constexpr static const u32 thread_errno = 0x7f00 + 18;
constexpr static const u32 filesystem_errno = 0x7f00 + 19;

// NOTE: will get truncd to lowest 8 bits, offset is for compat. with errno

class __base_exception
{
public:
  __base_exception() noexcept = default;
  virtual ~__base_exception() = default;

  virtual const char *
  what() const noexcept
  {
    return "__base_exception";
  };
};

// unclean but the only way to avoid senseless boilterplate ;c
#define MICRON_EXCEPTION_TEMP(_str, _errcode)                                                                                              \
  class _str : public __base_exception                                                                                                     \
  {                                                                                                                                        \
    const char *__what;                                                                                                                    \
    static constexpr u32 __errcode = _errcode;                                                                                             \
                                                                                                                                           \
  public:                                                                                                                                  \
    explicit _str(const char *w) : __what(w) {}                                                                                            \
    const char *                                                                                                                           \
    what() const noexcept override                                                                                                         \
    {                                                                                                                                      \
      return __what;                                                                                                                       \
    }                                                                                                                                      \
    constexpr u32                                                                                                                          \
    which() const noexcept                                                                                                                 \
    {                                                                                                                                      \
      return __errcode;                                                                                                                    \
    }                                                                                                                                      \
  };

MICRON_EXCEPTION_TEMP(domain_error, domain_errno)
MICRON_EXCEPTION_TEMP(invalid_argument, invalid_errno)
MICRON_EXCEPTION_TEMP(length_error, length_errno)
MICRON_EXCEPTION_TEMP(logic_error, logic_errno)
MICRON_EXCEPTION_TEMP(out_of_range_error, oor_errno)
MICRON_EXCEPTION_TEMP(overflow_error, overflow_errno)
MICRON_EXCEPTION_TEMP(range_error, range_errno)
MICRON_EXCEPTION_TEMP(runtime_error, runtime_errno)
MICRON_EXCEPTION_TEMP(underflow_error, underflow_errno)
MICRON_EXCEPTION_TEMP(future_error, future_errno)
MICRON_EXCEPTION_TEMP(system_error, system_errno)
MICRON_EXCEPTION_TEMP(io_error, io_errno)
MICRON_EXCEPTION_TEMP(format_error, format_errno)

// micron non-standard errors
MICRON_EXCEPTION_TEMP(standard_error, standard_errno)
MICRON_EXCEPTION_TEMP(library_error, library_errno)
MICRON_EXCEPTION_TEMP(hardware_error, hardware_errno)
MICRON_EXCEPTION_TEMP(memory_error, memory_errno)
MICRON_EXCEPTION_TEMP(thread_error, thread_errno)
MICRON_EXCEPTION_TEMP(filesystem_error, filesystem_errno)
};     // namespace except

using domain = except::domain_error;
using invalid = except::invalid_argument;
using length = except::length_error;
using logic = except::logic_error;
using oor = except::out_of_range_error;
using overflow = except::overflow_error;
using range_error = except::range_error;
using runtime = except::runtime_error;
using underflow = except::underflow_error;
using future_err = except::future_error;
using system = except::system_error;
using io_err = except::io_error;
using format_err = except::format_error;
using standard = except::standard_error;
using library = except::library_error;
using hardware = except::hardware_error;
using memory_err = except::memory_error;
using thread_err = except::thread_error;
using fs_error = except::filesystem_error;
};     // namespace micron
