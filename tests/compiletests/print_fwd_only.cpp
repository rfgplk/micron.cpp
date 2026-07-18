//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/bits/__print.hpp"
#include "../../src/settle_fwd.hpp"

// if either header ever grows a real include of the heavy runtimes, these fire
#if defined(_MICRON_CORO_ROUTINE_HPP) || defined(_MICRON_THREADS_HPP)
#error "settle_fwd.hpp must not pull in the coroutine or thread runtime"
#endif

static_assert(!micron::settling<int>, "a scalar is not settleable");
static_assert(!micron::any_settling<int, const char *>, "plain args are not settleable");
static_assert(micron::__print::kind_of_v<int> == micron::__print::kind::none, "scalars stay off the container path");

int
main(void)
{
  return 1;      // duck success sentinel
}
