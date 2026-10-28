//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../xkbcommon.hpp"
#include "basic_wayland_source.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Compatibility policy for existing users of the system XKB boundary.
namespace micron::uxin
{
struct system_keymap_decoder: xkb::keymap_state {
  bool
  prepare() noexcept
  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      (void)xkb::xkb_lib();
      return true;
    } catch ( ... ) {
      return false;
    }
#else
    (void)xkb::xkb_lib();
    return true;
#endif
  }

  i32
  load_map(raw_slice<const char> bytes) noexcept
  {
    if ( !bytes.len || bytes.ptr[bytes.len - 1] ) return -micron::error::invalid_arg;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      load(bytes.ptr);
      return 0;
    } catch ( ... ) {
      return -micron::error::invalid_arg;
    }
#else
    load(bytes.ptr);
    return 0;
#endif
  }
};

using wayland_source = basic_wayland_source<system_keymap_decoder>;
}      // namespace micron::uxin
