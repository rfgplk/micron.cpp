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
    throw micron::library_error("uxin open(): invalid specified type of device");
  }
  auto dev = mc::uxin::get_devices();
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      mc::uxin::bind_device(n);
      mc::uxin::poll(n, mc::uxin::prepare_generic_mouse(nullptr, key_press, nullptr));
      break;
    }
  }
}

void
read()
{
}
void
write()
{
}

};
};
