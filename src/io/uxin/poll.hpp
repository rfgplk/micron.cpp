//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../memory/actions.hpp"

#include "bits.hpp"
#include "reader.hpp"

#include "../../control.hpp"

namespace micron
{
namespace uxin
{

using poll_flag = int;

input_event
get_event(const device_t &dev)
{
  input_event event_buf;
  if ( !read_raw(dev, event_buf) )
    return {};
  return event_buf;
}

template <umax_t S = 250, auto Fn = get_event, typename... Args>
  requires((micron::same_as<Args, input_packet_t> && ...))
poll_flag
poll(const device_t &dev, Args &&...raw_in)
{
  if ( dev.bound_fd.has_error() )
    return -1;
  auto for_every = [](const input_event &event, input_packet_t &__raw_in) {
    if ( __raw_in.mask_type == event.type ) {
      for ( const auto &mask : __raw_in.button_mask ) {
        if ( mask == event.code ) {
          // pressed
          if ( __raw_in.callback )
            __raw_in.callback(event.time, event.code, event.value);
          if ( event.value ) {
            if ( __raw_in.actuate_callback )
              __raw_in.actuate_callback(event.time, event.code, event.value);
          } else {
            if ( __raw_in.release_callback )
              __raw_in.release_callback(event.time, event.code, event.value);
            // unpressed
          }
        }
      }
    }
  };
  pollfd pfd = make_poll(dev.bound_fd);
  for ( ;; ) {
    int res = micron::poll_for(pfd, S);
    if ( res < 0 ) {
      return -1;
    } else if ( !res )
      continue;
    if ( pfd.revents & poll_in ) {
      input_event ev = Fn(dev);
      (for_every(ev, raw_in), ...);
    }
  }
  return 0;
}

template <umax_t S = 250, auto Fn = get_event, typename... Args>
  requires((micron::same_as<input_packet_t, Args> && ...))
poll_flag
poll(input_t &inp, Args &&...raw_in)
{
  if ( inp.device.bound_fd.has_error() )
    return -1;
  auto for_every = [&](const input_event &event, input_packet_t &__raw_in) {
    if ( __raw_in.mask_type == event.type ) {
      for ( const auto &mask : __raw_in.button_mask ) {
        if ( mask == event.code ) {
          inp.history.push(event);
          if ( __raw_in.callback )
            __raw_in.callback(event.time, event.code, event.value);
          if ( event.value ) {
            if ( __raw_in.actuate_callback )
              __raw_in.actuate_callback(event.time, event.code, event.value);
          } else {
            if ( __raw_in.release_callback )
              __raw_in.release_callback(event.time, event.code, event.value);
            // unpressed
          }
        }
      }
    }
  };
  pollfd pfd = make_poll(inp.device.bound_fd);
  for ( ;; ) {
    std::cout << inp.device.i_path.c_str() << std::endl;
    int res = micron::poll_for(pfd, S);
    if ( res < 0 ) {
      return -1;
    } else if ( !res )
      continue;
    if ( pfd.revents & poll_in ) {
      input_event ev = Fn(inp.device);
      (for_every(ev, raw_in), ...);
    }
  }
  return 0;
}

pollfd
__make_poll(const input_t &arg)
{
  return make_poll(arg.device.bound_fd);
}

// when pack polling set a really low poll time, otherwise lag will occur
template <umax_t S = 1, auto Fn = get_event, typename... Args>
  requires((micron::same_as<input_packet_t, Args> && ...))
poll_flag
poll_pack(slice<input_t> &inp, Args &&...raw_in)
{
  for ( const auto &_inp : inp )
    if ( _inp.device.bound_fd.has_error() )
      return -1;
  auto for_every = [](input_t &_inp, const input_event &event, input_packet_t &__raw_in) {
    if ( __raw_in.mask_type == event.type ) {
      for ( const auto &mask : __raw_in.button_mask ) {
        if ( mask == event.code ) {
          _inp.history.push(event);
          if ( __raw_in.callback )
            __raw_in.callback(event.time, event.code, event.value);
          if ( event.value ) {
            if ( __raw_in.actuate_callback )
              __raw_in.actuate_callback(event.time, event.code, event.value);
          } else {
            if ( __raw_in.release_callback )
              __raw_in.release_callback(event.time, event.code, event.value);
            // unpressed
          }
        }
      }
    }
  };
  slice<pollfd> pfd_pack(__make_poll, inp);
  for ( ;; ) {
    for ( size_t j = 0; j < inp.size(); ++j ) {
      auto &pfd = pfd_pack[j];
      auto &inp_i = inp[j];
      int res = micron::poll_for(pfd, S);
      if ( res < 0 ) {
        return -1;
      } else if ( !res )
        continue;
      if ( pfd.revents & poll_in ) {
        input_event ev = Fn(inp_i.device);
        (for_every(inp_i, ev, raw_in), ...);
      }
    }
  }
  return 0;
}

template <auto Fn = get_event, typename... Args>
  requires((micron::same_as<input_packet_t, Args> && ...))
poll_flag
poll_pack_rt(slice<input_t> &inp, Args &&...raw_in)
{
  for ( const auto &_inp : inp )
    if ( _inp.device.bound_fd.has_error() )
      return -1;
  auto for_every = [](input_t &_inp, const input_event &event, input_packet_t &__raw_in) {
    if ( __raw_in.mask_type == event.type ) {
      for ( const auto &mask : __raw_in.button_mask ) {
        if ( mask == event.code ) {
          _inp.history.push(event);
          if ( __raw_in.callback )
            __raw_in.callback(event.time, event.code, event.value);
          if ( event.value ) {
            if ( __raw_in.actuate_callback )
              __raw_in.actuate_callback(event.time, event.code, event.value);
          } else {
            if ( __raw_in.release_callback )
              __raw_in.release_callback(event.time, event.code, event.value);
            // unpressed
          }
        }
      }
    }
  };
  for ( ;; ) {
    for ( size_t j = 0; j < inp.size(); ++j ) {
      auto &inp_i = inp[j];
      input_event ev = Fn(inp_i.device);
      (for_every(inp_i, ev, raw_in), ...);
    }
  }
  return 0;
}

};
};
