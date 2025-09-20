// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../linux/sysinfo.hpp"
#include "config.hpp"

namespace abc
{

inline auto
check_oom(void) -> bool
{
  if constexpr ( __default_oom_enable ) {
    micron::resources rs;
    size_t total_ram = rs.total_memory;
    size_t free_ram = rs.free_memory;
    if ( ((float)free_ram / (float)total_ram) <= __default_oom_limit_error ) {
      return true;
    } else if ( ((float)free_ram / (float)total_ram) <= __default_oom_limit_warn ) {
      // TODO: add an actual warning here, false for now
      return false;
    }
  }
  return false;
}

};
