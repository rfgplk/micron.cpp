//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/types.hpp"
#include "../../src/io/term/ansi.hpp"

constexpr bool escape(const char *s, char suffix)
{
  return s[0] == '\033' && s[1] == suffix && s[2] == '\0';
}

static_assert(escape(micron::ansi::C1[micron::ansi::nel], 'E'));
static_assert(escape(micron::ansi::FE[micron::ansi::decsc], '7'));
static_assert(escape(micron::ansi::FE[micron::ansi::decrc], '8'));
static_assert(escape(micron::ansi::FE[micron::ansi::back_index], '6'));
static_assert(escape(micron::ansi::FE[micron::ansi::fwd_index], '9'));

int main()
{
  return 1;
}
