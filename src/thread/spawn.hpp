#pragma once

#include "arena.hpp"
#include "pool.hpp"

namespace micron {

// new thread
void go();

// the two functions are aliases of each other
void go_rightnow();
void now();
// new proc
// fork and run process at path location specified by T
template <is_string T, is_string... R>
void run(const T &t, const R &...args) {
  process(t, args...);
}
//new thread
void spawn();

};
