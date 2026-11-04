// Copyright (c) 2026 David Lucius Severus. Boost Software License 1.0.
#pragma once
#include "../bits/__compilers.hpp"

// C ABI fallback for compiler-generated string operations in a freestanding image.
extern "C" [[gnu::weak, gnu::used]] __micron_optimize_no_tree_loop_distribute __SIZE_TYPE__
strlen(const char *s) noexcept
{
  __SIZE_TYPE__ n = 0;
  while ( s[n] ) ++n;
  return n;
}
