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

template <char C = '\n'>
inline bool
__read_from(micron::string &bufout, const fd_t &handle)
{
  if ( handle.has_error() or !handle.open() )
    return false;
  micron::slice<char> buf(4096);
  usize needle = 0;
  io::fget_byte(&buf[needle], handle);
  while ( buf[needle] != C )     // stop char to look for, if any
  {
    if ( needle >= 4095 ) {
      bufout += buf;
      buf.reset();
      needle = 0;
    }
    io::fget_byte(&buf[++needle], handle);
  }
  bufout.append(buf, needle);
  return true;
}

// redirect from stdin to some other arbitrary FILE*
void
copy_to(const fd_t &handle)
{
}
};     // namespace io

// functions intended for reading from the terminal and retrieving the data
template <bool Echo = false>
void
terminal(micron::string &bf)
{
  if ( !io::__read_from<'\n'>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo )
    io::println(bf);
}

template <char End = '\n'>
void
from_terminal(micron::string &bf)
{
  if ( !io::__read_from<End>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
}

template <char End = '\n'>
micron::string
from_terminal(void)
{
  micron::string bf{};
  if ( !io::__read_from<End>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  return bf;
}

template <bool Echo = false>
micron::string
terminal(void)
{
  micron::string bf;
  if ( !io::__read_from<'\n'>(bf, micron::io::stdin) )
    exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo )
    io::println(bf);
  return bf;
}

};     // namespace micron
