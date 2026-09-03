//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <micron/attach/entry.hpp>

#include <micron/attach/cont_args.hpp>

extern "C" i64
mx_continue_main(const micron_cont_args *a) noexcept
{
  return static_cast<i64>(a->user_len);
}
