#pragma once

namespace micron
{
constexpr int default_stack_size = 10 * 1024 * 1024;        // 10MB default on Linux
constexpr int thread_stack_size = 1 * 1024 * 1024;          // 1MB
constexpr int auto_thread_stack_size = 1 * 512 * 512;       // 262kB
constexpr int concurrent_thread_stack_size = 512 * 512;     // 262kB
constexpr int small_stack_size = 1024 * 1024;               // 1MB
constexpr int micro_stack_size = 256 * 1024;                // 256KB

struct stack_t {
  addr_t *start;
  size_t size;
};

};     // namespace micron
