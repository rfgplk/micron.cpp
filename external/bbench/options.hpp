
//          Copyright David Lucius Severus 2024-.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <micron/types.hpp>

namespace bbench
{

struct benchmark_opts {
  u32 detail = 1;
  u32 delay_ms = 0;
  u32 timeout_ms = 0;
  bool inherit = true;
  bool scale = true;
  bool pinned = false;
  bool excl_kernel = true;
  bool excl_user = false;
  const char *event_csv = nullptr;
  const char *pre = nullptr;
  const char *post = nullptr;
};

};      // namespace bbench
