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

template<is_string S = micron::string, char C = '\n'>
inline bool
__read_from(S &buf, const fd_t &handle)
{
  if ( handle.has_error() or !handle.open() ) return false;
  constexpr usize __heap_cap = 16u * 1024u * 1024u;
  usize needle = 0;
  for ( ;; ) {
    if constexpr ( requires { typename S::memory_type; } and micron::is_same_v<typename S::memory_type, stack_tag> ) {
      if ( needle >= buf.max_size() ) break;
    } else {
      if ( needle >= __heap_cap ) break;
    }
    char ch = 0;
    max_t r = io::fget_byte(&ch, handle);
    if ( r <= 0 ) break;
    buf.push_back(ch);
    if ( ch == C ) break;
    needle++;
  }
  return true;
}

// redirect from stdin to some other arbitrary FILE*
void
copy_to(const fd_t &handle)
{
}
};      // namespace io

// functions intended for reading from the terminal and retrieving the data
template<is_string S = micron::string, bool Echo = false>
void
terminal(S &bf)
{
  if ( !io::__read_from<S, '\n'>(bf, micron::io::stdin) ) exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo ) io::println(bf);
}

template<is_string S = micron::string, char End = '\n'>
void
from_terminal(S &bf)
{
  if ( !io::__read_from<S, End>(bf, micron::io::stdin) ) exc<except::io_error>("from_terminal(): failed to read from stream.");
}

template<is_string S = micron::string, char End = '\n'>
S
from_terminal(void)
{
  S bf{};
  if ( !io::__read_from<S, End>(bf, micron::io::stdin) ) exc<except::io_error>("from_terminal(): failed to read from stream.");
  return bf;
}

template<is_string S = micron::string, bool Echo = false>
S
terminal(void)
{
  S bf;
  if ( !io::__read_from<S, '\n'>(bf, micron::io::stdin) ) exc<except::io_error>("from_terminal(): failed to read from stream.");
  if constexpr ( Echo ) io::println(bf);
  return bf;
}

};      // namespace micron
