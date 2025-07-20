#pragma once

namespace micron {
constexpr int default_stack_size = 10 * 1024 * 1024;        // 10MB default on Linux
constexpr int auto_thread_stack_size = 2 * 1024 * 1024;     // 2MB
constexpr int small_stack_size = 1024 * 1024;               // 1MB
constexpr int micro_stack_size = 256 * 1024;                // 256KB
};
