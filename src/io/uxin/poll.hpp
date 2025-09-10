//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../memory/actions.hpp"
#include "reader.hpp"

namespace micron
{
namespace uxin
{

using poll_flag = int;

template <typename... Args>
  requires((micron::same_as<Args, raw_input_t> && ...))
poll_flag
poll(const device_t &dev, Args &&...raw_in)
{
  auto for_every = [](const input_event &event, raw_input_t &__raw_in) {
    if ( __raw_in.mask_type == event.type ) {
      for ( const auto &mask : __raw_in.button_mask ) {
        if ( mask == event.code ) {
          // pressed
          if ( __raw_in.callback )
            __raw_in.callback(event.code, event.value);
          if ( event.value ) {
            if ( __raw_in.actuate_callback )
              __raw_in.actuate_callback(event.code, event.value);
          } else {
            if ( __raw_in.release_callback )
              __raw_in.release_callback(event.code, event.value);
            // unpressed
          }
        }
      }
    }
  };

  input_event event_buf;
  for ( ;; ) {
    if ( !read_raw(dev, event_buf) )
      return -1;
    (for_every(event_buf, raw_in), ...);
  }
  return 0;
}
};
};
