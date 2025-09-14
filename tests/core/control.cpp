//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/control.hpp"
#include "../../src/io/console.hpp"
#include "../src/thread/signal.hpp"

#include "../../src/std.hpp"
void
test(int)
{
  mc::console("Caught sig_abrt");
}


int validate_sigset(const micron::sigset_t *s) {
    // first word: only bits 0..(_NSIG-1)
    unsigned long mask = s->__val[0] & ~((1UL << (64-1)) - 1);
    if (mask) return 0; // invalid bits in first word

    // all remaining words must be zero
    for (int i = 1; i < 16; i++) {
        if (s->__val[i] != 0) return 0;
    }
    return 1;
}


int check_sigset(const micron::sigset_t *set) {
    for (int i = 0; i < 16; i++) { // 16 × unsigned long = 128 bytes
        unsigned long mask = set->__val[i];
        // for last word, only lower bits may be set
        if (i == 15) mask &= ~((1UL << ((64-1) % 64 + 1)) - 1);
        if (mask != 0) return 0; // invalid high bits set
    }
    return 1; // valid
}

int
main(void)
{
  mc::console("Sleeping for one second...");
  mc::ssleep(1);
  micron::signal s(micron::signals::abort);
  micron::sigset_t& ss = s.get_signal();
  mc::console("mask() result: ", s.mask());
  mc::console("on_signal() result: ", s.on_signal(micron::signals::alarm, test));
  s();
  mc::console("Mask");
  return 0;
}
