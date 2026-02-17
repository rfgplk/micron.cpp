#pragma once

#include "../src/linux/sys/__threads.hpp"

#include "stack_constants.hpp"

namespace micron
{
// currently we'll rely on the pthread bookkeeping implementation to get this quickly

// TODO: implement this via _start

auto
get_stack(void) -> stack_t
{
  pthread_attr_t attr;
  pthread::get_attrs(pthread::self(), attr);
  addr_t *ptr = nullptr;
  size_t sz = 0;
  pthread::get_stack_thread(attr, ptr, sz);
  pthread_attr_destroy(&attr);
  return { ptr, sz };
};

auto
get_stack_start(void) -> addr_t *
{
  return get_stack().start;
}

auto
get_stack_size(void) -> size_t
{
  return get_stack().size;
}
};
