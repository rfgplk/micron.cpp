//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../memory/actions.hpp"
#include "devices.hpp"
#include "key_mapper.hpp"
#include "poll.hpp"
#include "reader.hpp"

#include "../../except.hpp"

namespace micron
{
namespace uxin
{

// high level "open a device and init" func.
vector<input_t>
open_nonblock(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    break;
  case type_t::mouse:
    break;
  default:
    exc<except::library_error>("uxin open(): invalid specified type of device");
  }
  auto dev = ::micron::uxin::get_devices();
  vector<input_t> res;
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      ::micron::uxin::nonblock_bind_device(n);
      res.push_back({ micron::move(n), {} });      // device_t is move-only (owns its fd)
    }
  }
  if ( res.empty() ) exc<except::library_error>("uxin open(): couldn't open device");
  return res;
}

input_t
open_first_nonblock(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    break;
  case type_t::mouse:
    break;
  default:
    exc<except::library_error>("uxin open(): invalid specified type of device");
  }
  auto dev = ::micron::uxin::get_devices();
  vector<input_t> res;
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      ::micron::uxin::nonblock_bind_device(n);
      return { micron::move(n), {} };
    }
  }
  exc<except::library_error>("uxin open(): couldn't open device");
  __builtin_unreachable();
}

vector<input_t>
open(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    break;
  case type_t::mouse:
    break;
  default:
    exc<except::library_error>("uxin open(): invalid specified type of device");
  }
  auto dev = ::micron::uxin::get_devices();
  vector<input_t> res;
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      ::micron::uxin::bind_device(n);
      res.push_back({ micron::move(n), {} });      // device_t is move-only (owns its fd)
    }
  }
  if ( res.empty() ) exc<except::library_error>("uxin open(): couldn't open device");
  return res;
}

input_t
open_first(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    break;
  case type_t::mouse:
    break;
  default:
    exc<except::library_error>("uxin open(): invalid specified type of device");
  }
  auto dev = ::micron::uxin::get_devices();
  vector<input_t> res;
  for ( auto &n : dev ) {
    if ( n.type == __type ) {
      ::micron::uxin::bind_device(n);
      return { micron::move(n), {} };
    }
  }
  exc<except::library_error>("uxin open(): couldn't open device");
  __builtin_unreachable();
}

template<auto Fn = nullptr, auto Fn_2 = nullptr, auto Fn_3 = nullptr>
input_packet_t
prepare_listener(type_t __type)
{
  switch ( __type ) {
  case type_t::keyboard:
    return ::micron::uxin::prepare_generic_keyboard_us(Fn, Fn_2, Fn_3);
    break;
  case type_t::mouse:
    return ::micron::uxin::prepare_generic_mouse_sensor(Fn, Fn_2, Fn_3);
    break;
  default:
    exc<except::library_error>("uxin prepare_listener(): invalid specified type of device");
  }
  exc<except::library_error>("uxin prepare_listener(): invalid specified type of device");
}

template<typename... Args>
  requires(micron::same_as<input_packet_t, micron::remove_cvref_t<Args>> && ...)
void
read(input_t &t, Args &&...__input_packet)
{
  if ( !is_loaded(t.device) ) exc<except::library_error>("uxin read(): device isn't loaded");
  if ( !is_bound(t.device) ) exc<except::library_error>("uxin read(): device isn't bound");
  ::micron::uxin::poll(t, micron::forward<Args>(__input_packet)...);
}

template<typename... Args>
  requires(micron::same_as<input_packet_t, micron::remove_cvref_t<Args>> && ...)
void
read(vector<input_t> &inputs, Args &&...__input_packet)
{
  for ( const auto &t : inputs ) {
    if ( !is_loaded(t.device) ) exc<except::library_error>("uxin read(): device isn't loaded");
    if ( !is_bound(t.device) ) exc<except::library_error>("uxin read(): device isn't bound");
  }
  ::micron::uxin::poll_pack(inputs, micron::forward<Args>(__input_packet)...);
}

// NOTE: if using a pack, be sure to open with a nonblocking method
template<typename... Args>
  requires(micron::same_as<input_packet_t, micron::remove_cvref_t<Args>> && ...)
void
read_rt(vector<input_t> &inputs, Args &&...__input_packet)
{
  for ( const auto &t : inputs ) {
    if ( !is_loaded(t.device) ) exc<except::library_error>("uxin read(): device isn't loaded");
    if ( !is_bound(t.device) ) exc<except::library_error>("uxin read(): device isn't bound");
  }
  ::micron::uxin::poll_pack_rt(inputs, micron::forward<Args>(__input_packet)...);
}

template<typename... Args>
  requires(micron::same_as<input_packet_t, micron::remove_cvref_t<Args>> && ...)
void
read_once(vector<input_t> &inputs, Args &&...__input_packet)
{
  for ( const auto &t : inputs ) {
    if ( !is_loaded(t.device) ) exc<except::library_error>("uxin read(): device isn't loaded");
    if ( !is_bound(t.device) ) exc<except::library_error>("uxin read(): device isn't bound");
  }
  ::micron::uxin::poll_pack_once(inputs, micron::forward<Args>(__input_packet)...);
}

void
write()
{
}

};      // namespace uxin
};      // namespace micron
