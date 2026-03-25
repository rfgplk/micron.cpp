//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../algorithm/memory.hpp"
#include "../slice.hpp"
#include "../string/strings.hpp"
#include "../types.hpp"
#include "io.hpp"
#include "stdout.hpp"

namespace micron
{
namespace io
{

template <is_string S = micron::string, char C = '\n'>
inline bool
__read_from(S &buf, const fd_t &handle)
{
  if ( handle.has_error() or !handle.open() )
    return false;
  // micron::slice<char> buf(4096);
  usize needle = 0;
  buf.push_back('0');
  io::fget_byte(buf.end() - 1, handle);
  while ( buf[needle] != C )     // stop char to look for, if any
  {
    if constexpr ( requires { typename S::memory_type; } and micron::is_same_v<typename S::memory_type, stack_tag> ) {
      if ( needle >= buf.max_size() )
        break;
    }
    buf.push_back('0');
    io::fget_byte(buf.end() - 1, handle);
    needle++;
  }
  return true;
}

// redirect from stdin to some other arbitrary FILE*
void
copy_to(const fd_t &handle)
{
}
};     // namespace io

// functions intended for reading from the terminal and retrieving the data
template <is_string S = micron::string, bool Echo = false>
void
terminal(S &bf)
{
  if ( !io::__read_from<S, '\n'>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo )
    io::println(bf);
}

template <is_string S = micron::string, char End = '\n'>
void
from_terminal(S &bf)
{
  if ( !io::__read_from<S, End>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
}

template <is_string S = micron::string, char End = '\n'>
S
from_terminal(void)
{
  S bf{};
  if ( !io::__read_from<S, End>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  return bf;
}

template <is_string S = micron::string, bool Echo = false>
S
terminal(void)
{
  S bf;
  if ( !io::__read_from<S, '\n'>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo )
    io::println(bf);
  return bf;
}

};     // namespace micron
