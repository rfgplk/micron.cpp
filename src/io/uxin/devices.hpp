//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../io/posix/iosys.hpp"
#include "../../linux/sys/event_codes.hpp"

#include "../bits.hpp"

#include "../../string/format.hpp"
#include "../../string/unistring.hpp"
#include "../../vector/fvector.hpp"
#include "../../vector/vector.hpp"
#include "../filesystem.hpp"
#include "../fsys.hpp"

namespace micron
{
namespace uxin
{

inline auto
__parse_device(const char *s, const micron::ustr8 &buf)
{
  auto tk = micron::format::find(buf, s);
  if ( !tk ) return micron::ustr8{};
  auto del = micron::format::find(buf, tk, '=');
  if ( !del ) return micron::ustr8{};
  auto del_eol = micron::format::find(buf, del, '\n');
  if ( !del_eol ) del_eol = buf.end();
  return buf.substr(del + 1, del_eol);
}

inline usize
__parse_hex_words(const micron::ustr8 &buf, i64 *words, usize max_words)
{
  usize n_words = 0;
  auto cur = buf.cbegin();
  const auto end = buf.cend();
  while ( n_words < max_words ) {
    while ( cur != end && *cur == ' ' ) ++cur;
    if ( cur == end ) break;
    auto next = micron::format::find(buf, cur, ' ');
    if ( next == nullptr ) {
      words[n_words++] = hex_string_to_int64(buf.substr(cur, end));
      break;
    }
    words[n_words++] = hex_string_to_int64(buf.substr(cur, next));
    cur = next + 1;
  }
  return n_words;
}

inline bool
__key_bit_set(const i64 *words, usize n_words, i32 code)
{
  const usize k = static_cast<usize>(code);
  const usize word_from_low = k / 64u;
  const usize bit_in_word = k % 64u;
  if ( word_from_low >= n_words ) return false;
  const usize idx = n_words - 1 - word_from_low;
  return (words[idx] >> bit_in_word) & i64(1);
}

inline bool
__has_key_code(const micron::ustr8 &keybuf, i32 code)
{
  constexpr usize __max_words = 16;
  i64 words[__max_words] = {};
  const usize n_words = __parse_hex_words(keybuf, words, __max_words);
  if ( n_words == 0 ) return false;
  return __key_bit_set(words, n_words, code);
}

inline bool
__has_alnum_keys(const micron::ustr8 &buf)
{
  constexpr usize __max_words = 16;
  i64 words[__max_words] = {};
  const usize n_words = __parse_hex_words(buf, words, __max_words);
  if ( n_words == 0 ) return false;

  constexpr byte alnum[] = {
    key_1, key_2, key_3, key_4, key_5, key_6, key_7, key_8, key_9, key_0, key_q, key_w, key_e, key_r, key_T, key_y, key_u, key_i,
    key_o, key_p, key_a, key_s, key_d, key_f, key_g, key_h, key_j, key_k, key_l, key_z, key_x, key_c, key_v, key_b, key_n, key_m,
  };
  constexpr usize n_alnum = sizeof(alnum) / sizeof(alnum[0]);

  usize count = 0;
  for ( usize i = 0; i < n_alnum; ++i )
    if ( __key_bit_set(words, n_words, static_cast<i32>(alnum[i])) ) ++count;
  return count >= 30;
}

enum class type_t : char { unknown, keyboard, mouse, touchpad, touchscreen, joystick, tablet, pointing_stick };

struct device_t {
  ustr8 i_path;
  ustr8 path;
  ustr8 name;
  ustr8 phys;
  type_t type = type_t::unknown;
  io::fd_t bound_fd{ -1 };

  device_t() = default;

  device_t(ustr8 ip, ustr8 p, ustr8 n, ustr8 ph, type_t t, io::fd_t fd)
      : i_path(micron::move(ip)), path(micron::move(p)), name(micron::move(n)), phys(micron::move(ph)), type(t), bound_fd(fd)
  {
  }

  ~device_t()
  {
    if ( bound_fd.open() ) posix::close(bound_fd.fd);
  }

  device_t(const device_t &) = delete;
  device_t &operator=(const device_t &) = delete;

  device_t(device_t &&o) noexcept
      : i_path(micron::move(o.i_path)), path(micron::move(o.path)), name(micron::move(o.name)), phys(micron::move(o.phys)), type(o.type),
        bound_fd(o.bound_fd)
  {
    o.bound_fd = io::fd_t{ -1 };
  }

  device_t &
  operator=(device_t &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( bound_fd.open() ) posix::close(bound_fd.fd);
    i_path = micron::move(o.i_path);
    path = micron::move(o.path);
    name = micron::move(o.name);
    phys = micron::move(o.phys);
    type = o.type;
    bound_fd = o.bound_fd;
    o.bound_fd = io::fd_t{ -1 };
    return *this;
  }
};

inline bool
is_loaded(const device_t &dev)
{
  return !(dev.i_path.empty() or dev.path.empty());
}

inline bool
is_bound(const device_t &dev)
{
  return !(dev.bound_fd.closed());
}

inline vector<device_t>
get_devices()
{
  micron::vector<device_t> vc;
  fsys::system<micron::io::rd> sys;
  auto ps = micron::io::path("/sys/class/input/").all();
  for ( auto &n : ps ) {
    auto loc = format::find(n, "event");
    if ( loc == nullptr ) continue;

    // /sys/class/input/[eventX]/device/uevent
    micron::ustr8 pt = "/sys/class/input/" + n + "/device/uevent";
    micron::hstring<char> raw;
    sys[pt] >> raw;
    micron::ustr8 buf(raw);

    if ( !format::contains(buf, "EV") ) continue;
    const i64 ev = hex_string_to_int64(__parse_device("EV", buf));
    const i64 prop = hex_string_to_int64(__parse_device("PROP", buf));
    const micron::ustr8 keybuf = __parse_device("KEY", buf);

    // capability bits (EV_*)
    const bool has_key = ev & (i64(1) << ev_key);
    const bool has_rel = ev & (i64(1) << ev_rel);
    const bool has_rep = ev & (i64(1) << ev_rep);
    const bool has_abs = ev & (i64(1) << ev_abs);
    // input properties (INPUT_PROP_*)
    const bool p_direct = prop & (i64(1) << input_prop_direct);
    const bool p_stick = prop & (i64(1) << input_prop_pointing_stick);
    // tool / button signatures (BTN_*)
    const bool k_pen = __has_key_code(keybuf, btn_tool_pen);
    const bool k_stylus = __has_key_code(keybuf, btn_stylus);
    const bool k_finger = __has_key_code(keybuf, btn_tool_finger);
    const bool k_joy = __has_key_code(keybuf, btn_joystick);
    const bool k_pad = __has_key_code(keybuf, btn_gamepad);

    // classify by capability
    // EV bits alone are ambiguous for absolute devices
    type_t type = type_t::unknown;
    if ( has_abs ) {
      if ( k_pen || k_stylus )
        type = type_t::tablet;            // pen digitizer (BTN_TOOL_PEN/STYLUS)
      else if ( p_direct )
        type = type_t::touchscreen;       // coords map onto a screen (INPUT_PROP_DIRECT)
      else if ( k_finger )
        type = type_t::touchpad;          // indirect finger pad (BTN_TOOL_FINGER)
      else if ( k_joy || k_pad )
        type = type_t::joystick;          // stick / gamepad (BTN_TRIGGER/BTN_GAMEPAD)
    } else if ( has_rel && has_key ) {
      type = p_stick ? type_t::pointing_stick : type_t::mouse;
    } else if ( has_key && has_rep && __has_alnum_keys(keybuf) ) {
      type = type_t::keyboard;
    }
    if ( type == type_t::unknown ) continue;      // power / lid / audio / WMI / etc: correctly ignored

    auto name = __parse_device("NAME", buf);
    auto phys = __parse_device("PHYS", buf);
    vc.emplace_back("/dev/input/" + n, "/sys/class/input/" + n, name, phys, type, -1);
  }
  return vc;
}

// start reading from device
// WARNING: need either CAP_SYS_ADMIN or CAP_SYS_RAWIO to be able to read raw character devices.
// WILL FAIL OTHERWISE
inline int
nonblock_bind_device(device_t &dev)
{
  if ( dev.i_path.empty() ) return -1;
  int fd = static_cast<int>(posix::open(dev.i_path.c_str(), posix::o_nonblock | posix::o_rdonly));
  if ( fd < 0 ) return fd;
  if ( dev.bound_fd.open() ) posix::close(dev.bound_fd.fd);      // don't leak a prior binding
  dev.bound_fd = fd;
  return fd;
}

inline int
bind_device(device_t &dev)
{
  if ( dev.i_path.empty() ) return -1;
  int fd = static_cast<int>(posix::open(dev.i_path.c_str(), posix::o_rdonly));
  if ( fd < 0 ) return fd;
  if ( dev.bound_fd.open() ) posix::close(dev.bound_fd.fd);      // don't leak a prior binding
  dev.bound_fd = fd;

  return fd;
}

inline void
unbind_device(device_t &dev)
{
  // iff the fd is actually open
  if ( dev.bound_fd.open() ) posix::close(dev.bound_fd.fd);
  dev.bound_fd = -1;
}
};      // namespace uxin
};      // namespace micron
