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
#include "../filesystem.hpp"
#include "../fsys.hpp"

namespace micron
{
namespace uxin
{

auto
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

bool
__has_alnum_keys(const micron::ustr8 &buf)
{
  constexpr usize __max_words = 16;
  i64 words[__max_words] = {};
  usize n_words = 0;

  auto cur = buf.cbegin();
  const auto end = buf.cend();
  while ( n_words < __max_words ) {
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
  if ( n_words == 0 ) return false;

  constexpr byte alnum[] = {
    key_1, key_2, key_3, key_4, key_5, key_6, key_7, key_8, key_9, key_0, key_q, key_w, key_e, key_r, key_T, key_y, key_u, key_i,
    key_o, key_p, key_a, key_s, key_d, key_f, key_g, key_h, key_j, key_k, key_l, key_z, key_x, key_c, key_v, key_b, key_n, key_m,
  };
  constexpr usize n_alnum = sizeof(alnum) / sizeof(alnum[0]);

  usize count = 0;
  for ( usize i = 0; i < n_alnum; ++i ) {
    const usize k = static_cast<usize>(alnum[i]);
    const usize word_from_low = k / 64u;
    const usize bit_in_word = k % 64u;
    if ( word_from_low >= n_words ) continue;
    const usize idx = n_words - 1 - word_from_low;
    if ( (words[idx] >> bit_in_word) & i64(1) ) ++count;
  }
  return count >= 30;
}

enum class type_t : char { keyboard, mouse };

struct device_t {
  ustr8 i_path;
  ustr8 path;
  ustr8 name;
  ustr8 phys;
  type_t type;
  io::fd_t bound_fd;
};

bool
is_loaded(const device_t &dev)
{
  return !(dev.i_path.empty() or dev.path.empty());
}

bool
is_bound(const device_t &dev)
{
  return !(dev.bound_fd.closed());
}

fvector<device_t>
get_devices()
{
  micron::fvector<device_t> vc;
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

    // identify by EV bits only
    // old substring sniffing on the uevent text misclassified
    const bool has_key = ev & (i64(1) << ev_key);
    const bool has_rel = ev & (i64(1) << ev_rel);
    const bool has_rep = ev & (i64(1) << ev_rep);

    if ( has_key && has_rep ) {
      if ( !format::contains(buf, "KEY") || !__has_alnum_keys(__parse_device("KEY", buf)) ) {
      } else {
        auto name = __parse_device("NAME", buf);
        auto phys = __parse_device("PHYS", buf);
        vc.emplace_back("/dev/input/" + n, "/sys/class/input/" + n, name, phys, type_t::keyboard, -1);
        continue;
      }
    }
    if ( has_rel && !has_rep ) {
      auto name = __parse_device("NAME", buf);
      auto phys = __parse_device("PHYS", buf);
      vc.emplace_back("/dev/input/" + n, "/sys/class/input/" + n, name, phys, type_t::mouse, -1);
      continue;
    }
  }
  return vc;
}

// start reading from device
// WARNING: need either CAP_SYS_ADMIN or CAP_SYS_RAWIO to be able to read raw character devices.
// WILL FAIL OTHERWISE
int
nonblock_bind_device(device_t &dev)
{
  if ( dev.i_path.empty() ) return -1;
  int fd = static_cast<int>(posix::open(dev.i_path.c_str(), posix::o_nonblock | posix::o_rdonly));
  if ( fd < 0 ) return fd;
  dev.bound_fd = fd;
  return fd;
}

int
bind_device(device_t &dev)
{
  if ( dev.i_path.empty() ) return -1;
  int fd = static_cast<int>(posix::open(dev.i_path.c_str(), posix::o_rdonly));
  if ( fd < 0 ) return fd;
  dev.bound_fd = fd;

  return fd;
}

void
unbind_device(device_t &dev)
{
  // iff the fd is actually open
  if ( dev.bound_fd.open() ) posix::close(dev.bound_fd.fd);
  dev.bound_fd = -1;
}
};      // namespace uxin
};      // namespace micron
