#pragma once

#include <sys/sysinfo.h>     // sysinfo

namespace micron
{
// this is a wrapper around Linux spec. fcalls providing necessary system info
// again NO idea why the STL doesn't have this
struct resources {
  size_t memory;
  size_t free_memory;
  size_t shared_memory;
  size_t buffer_memory;
  size_t total_swap;
  size_t free_swap;
  size_t procs;
  size_t mem_unit;
  resources() {
    __impl_rs();
  }
  resources& operator()(void) {
    __impl_rs(); // update
    return *this;
  }
  size_t operator[](void) {
    return memory;
  }
private:
  inline void
  __impl_rs(void) {
    struct sysinfo info;
    ::sysinfo(&info);
    memory = info.totalram;
    free_memory = info.freeram;
    shared_memory = info.sharedram;
    buffer_memory = info.bufferram;
    total_swap = info.totalswap;
    free_swap = info.freeswap;
    procs = info.procs;
    mem_unit = info.mem_unit;
  }
};
};
