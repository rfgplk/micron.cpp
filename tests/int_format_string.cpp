//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/string/strings.hpp"
#include "../src/io/console.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"
#include "../src/string/string_view.hpp"
#include "../src/string/unistring.hpp"
#include "../src/string/radixstring.hpp"

#include "snowball/snowball.hpp"

int
main(void)
{
    // testing vs stack, heap allocations kill perf
    mc::sstring<256, schar> buf{};
    for(usize i = 0; i < 1'000'000'000; ++i)
        buf = mc::move(micron::int_to_string_stack<usize, schar, 256>(i));
    mc::console(buf);
    return 1;
};
