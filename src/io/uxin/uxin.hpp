//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../memory/actions.hpp"
#include "devices.hpp"
#include "poll.hpp"
#include "reader.hpp"

#include "../../except.hpp"

namespace micron
{
namespace uxin
{

// high level "open a device and init" func.
input_t
open(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    break;
  case type_t::mouse:
    break;
  default:
    throw micron::except::library_error("uxin open(): invalid specified type of device");
  }
  auto dev = mc::uxin::get_devices();
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      mc::uxin::bind_device(n);
      return { n, {} };
    }
  }
  throw micron::except::library_error("uxin open(): couldn't open device");
}

template <auto Fn = nullptr, auto Fn_2 = nullptr, auto Fn_3 = nullptr>
input_packet_t
prepare_listener(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    return mc::uxin::prepare_generic_keyboard_us(Fn, Fn_2, Fn_3);
    break;
  case type_t::mouse:
    return mc::uxin::prepare_generic_mouse_sensor(Fn, Fn_2, Fn_3);
    break;
  default:
    throw micron::except::library_error("uxin prepare_listener(): invalid specified type of device");
  }
  throw micron::except::library_error("uxin prepare_listener(): invalid specified type of device");
}

template <typename... Args>
  requires(micron::same_as<input_packet_t, Args> && ...)
void
read(input_t &t, Args &&...__input_packet)
{
  if ( !is_loaded(t.device) )
    throw micron::except::library_error("uxin read(): device isn't loaded");
  if ( !is_bound(t.device) )
    throw micron::except::library_error("uxin read(): device isn't bound");
  mc::uxin::poll(t, micron::forward<Args>(__input_packet)...);
}
void
write()
{
}

};
};
