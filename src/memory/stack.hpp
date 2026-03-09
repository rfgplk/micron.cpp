#pragma once

#include "stack_constants.hpp"

namespace micron
{
// currently we'll rely on the pthread bookkeeping implementation to get this quickly

// TODO: implement this via _start

auto get_stack(void) -> stack_t;

// in __threads.hpp

auto
get_stack_start(void) -> addr_t *
{
  return get_stack().start;
}

auto
get_stack_size(void) -> usize
{
  return get_stack().size;
}
};     // namespace micron
