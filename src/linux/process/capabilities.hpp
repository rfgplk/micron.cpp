//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../sys/capabilities.hpp"

namespace micron
{

enum class cap : u32 {
  chown = posix::cap_chown,
  dac_override = posix::cap_dac_override,
  dac_read_search = posix::cap_dac_read_search,
  fowner = posix::cap_fowner,
  fsetid = posix::cap_fsetid,
  kill = posix::cap_kill,
  setgid = posix::cap_setgid,
  setuid = posix::cap_setuid,
  setpcap = posix::cap_setpcap,
  linux_immutable = posix::cap_linux_immutable,
  net_bind_service = posix::cap_net_bind_service,
  net_broadcast = posix::cap_net_broadcast,
  net_admin = posix::cap_net_admin,
  net_raw = posix::cap_net_raw,
  ipc_lock = posix::cap_ipc_lock,
  ipc_owner = posix::cap_ipc_owner,
  sys_module = posix::cap_sys_module,
  sys_rawio = posix::cap_sys_rawio,
  sys_chroot = posix::cap_sys_chroot,
  sys_ptrace = posix::cap_sys_ptrace,
  sys_pacct = posix::cap_sys_pacct,
  sys_admin = posix::cap_sys_admin,
  sys_boot = posix::cap_sys_boot,
  sys_nice = posix::cap_sys_nice,
  sys_resource = posix::cap_sys_resource,
  sys_time = posix::cap_sys_time,
  sys_tty_config = posix::cap_sys_tty_config,
  mknod = posix::cap_mknod,
  lease = posix::cap_lease,
  audit_write = posix::cap_audit_write,
  audit_control = posix::cap_audit_control,
  setfcap = posix::cap_setfcap,
  mac_override = posix::cap_mac_override,
  mac_admin = posix::cap_mac_admin,
  syslog = posix::cap_syslog,
  wake_alarm = posix::cap_wake_alarm,
  block_suspend = posix::cap_block_suspend,
  audit_read = posix::cap_audit_read,
  perfmon = posix::cap_perfmon,
  bpf = posix::cap_bpf,
  checkpoint_restore = posix::cap_checkpoint_restore,
  __count = posix::cap_last_cap + 1u
};

constexpr u64 cap_all_mask = (u64(1) << static_cast<u32>(cap::__count)) - 1u;

constexpr u64
cap_bit(cap c) noexcept
{
  return u64(1) << static_cast<u32>(c);
}

template <typename T>
concept is_cap_set = requires(const T &t) {
  { t.effective } -> micron::convertible_to<u64>;
  { t.permitted } -> micron::convertible_to<u64>;
  { t.inheritable } -> micron::convertible_to<u64>;
};

struct ucap_set_t {
  u64 effective = 0;
  u64 permitted = 0;
  u64 inheritable = 0;
  u64 ambient = 0;
  u64 bounding = cap_all_mask;

  constexpr ~ucap_set_t(void) = default;
  constexpr ucap_set_t(void) = default;

  constexpr ucap_set_t(u64 eff, u64 perm, u64 inh, u64 amb, u64 boun)
      : effective(eff), permitted(perm), inheritable(inh), ambient(amb), bounding(boun)
  {
  }

  constexpr bool
  has_effective(cap c) const noexcept
  {
    return (effective & cap_bit(c)) != 0;
  }

  constexpr bool
  has_permitted(cap c) const noexcept
  {
    return (permitted & cap_bit(c)) != 0;
  }

  constexpr bool
  has_inheritable(cap c) const noexcept
  {
    return (inheritable & cap_bit(c)) != 0;
  }

  constexpr bool
  has_ambient(cap c) const noexcept
  {
    return (ambient & cap_bit(c)) != 0;
  }

  constexpr bool
  has_bounding(cap c) const noexcept
  {
    return (bounding & cap_bit(c)) != 0;
  }

  constexpr bool
  empty() const noexcept
  {
    return (effective | permitted) == 0;
  }

  constexpr ucap_set_t &
  set_effective(cap c) noexcept
  {
    effective |= cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  set_permitted(cap c) noexcept
  {
    permitted |= cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  set_inheritable(cap c) noexcept
  {
    inheritable |= cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  set_ambient(cap c) noexcept
  {
    ambient |= cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  set_bounding(cap c) noexcept
  {
    bounding |= cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  clr_effective(cap c) noexcept
  {
    effective &= ~cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  clr_permitted(cap c) noexcept
  {
    permitted &= ~cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  clr_inheritable(cap c) noexcept
  {
    inheritable &= ~cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  clr_ambient(cap c) noexcept
  {
    ambient &= ~cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  drop_bounding(cap c) noexcept
  {
    bounding &= ~cap_bit(c);
    return *this;
  }

  constexpr ucap_set_t &
  grant(cap c) noexcept
  {
    return set_effective(c).set_permitted(c).set_inheritable(c);
  }

  constexpr ucap_set_t &
  revoke(cap c) noexcept
  {
    return clr_effective(c).clr_permitted(c).clr_inheritable(c).clr_ambient(c).drop_bounding(c);
  }

  constexpr ucap_set_t &
  inherit_ambient(cap c) noexcept
  {
    return grant(c).set_ambient(c);
  }

  static constexpr ucap_set_t
  none() noexcept
  {
    ucap_set_t s;
    s.effective = s.permitted = s.inheritable = s.ambient = s.bounding = 0;
    return s;
  }

  static constexpr ucap_set_t
  all() noexcept
  {
    ucap_set_t s;
    s.effective = s.permitted = s.inheritable = s.ambient = s.bounding = cap_all_mask;
    return s;
  }

  template <typename... Cs>
    requires(is_same_v<Cs, cap> && ...)
  static constexpr ucap_set_t
  from(Cs... cs) noexcept
  {
    ucap_set_t s{};
    (s.grant(cs), ...);
    return s;
  }

  static constexpr ucap_set_t
  from_masks(u64 eff, u64 prm, u64 inh, u64 bnd = cap_all_mask, u64 amb = 0) noexcept
  {
    return ucap_set_t{ eff, prm, inh, amb, bnd };
  }
};

// NOTE: bounding and ambient sets are only accessible via prctl for the calling thread
// for foreign pids use ucap_set_t::from_masks()
inline ucap_set_t
get_caps(pid_t pid = 0)
{
  posix::cap_data_t data[2]{};
  if ( posix::capget_pid(pid, data) < 0 ) return ucap_set_t{};

  ucap_set_t cs{};
  posix::data_to_caps(data, cs.effective, cs.permitted, cs.inheritable);

  if ( pid == 0 || pid == posix::getpid() ) {
    cs.bounding = posix::cap_bset_read_all();
    cs.ambient = posix::cap_ambient_read_all();
  } else {
    cs.bounding = cap_all_mask;
    cs.ambient = 0;
  }
  return cs;
}

inline ucap_set_t
this_caps()
{
  return get_caps(0);
}

inline bool
has_cap(cap c)
{
  posix::cap_data_t data[2]{};
  posix::capget_pid(0, data);
  u64 eff = u64(data[0].effective) | (u64(data[1].effective) << 32);
  return (eff & cap_bit(c)) != 0;
}

template <bool Effective = true, bool Permitted = false, bool Inheritable = false>
inline bool
has_cap(cap c)
{
  posix::cap_data_t data[2]{};
  posix::capget_pid(0, data);
  u64 eff, prm, inh;
  posix::data_to_caps(data, eff, prm, inh);
  if constexpr ( Effective ) return (eff & cap_bit(c)) != 0;
  if constexpr ( Permitted ) return (prm & cap_bit(c)) != 0;
  if constexpr ( Inheritable ) return (inh & cap_bit(c)) != 0;
  return false;
}

inline int
set_caps(const ucap_set_t &cs)
{
  posix::cap_data_t data[2]{};
  posix::caps_to_data(cs.effective, cs.permitted, cs.inheritable, data);
  return posix::capset_self(data);
}

inline int
drop_cap(cap c)
{
  auto cs = get_caps(0);
  cs.revoke(c);
  return set_caps(cs);
}

inline int
drop_all_caps()
{
  return set_caps(ucap_set_t::none());
}

inline int
drop_bounding(cap c)
{
  return posix::cap_bset_drop(static_cast<u32>(c));
}

inline int
raise_ambient(cap c)
{
  return posix::cap_ambient_raise(static_cast<u32>(c));
}

inline int
lower_ambient(cap c)
{
  return posix::cap_ambient_lower(static_cast<u32>(c));
}

inline int
clear_ambient()
{
  return posix::cap_ambient_clear_all();
}

inline int
no_new_privs()
{
  return posix::cap_no_new_privs();
}

template <is_cap_set Caps>
inline int
apply_caps_child(const Caps &cs) noexcept
{
  posix::cap_set_keepcaps(1);

  u64 want_bnd;
  if constexpr ( requires { cs.bounding; } )
    want_bnd = static_cast<u64>(cs.bounding);
  else
    want_bnd = cap_all_mask;

  u64 drop_bnd = ~want_bnd & cap_all_mask;
  for ( u32 i = 0; i <= posix::cap_last_cap; ++i ) {
    if ( drop_bnd & (u64(1) << i) ) posix::cap_bset_drop(i);
  }

  posix::cap_data_t data[2]{};
  posix::caps_to_data(cs.effective, cs.permitted, cs.inheritable, data);
  int r = posix::capset_self(data);
  if ( r < 0 ) return r;

  if constexpr ( requires { cs.ambient; } ) {
    u64 amb = static_cast<u64>(cs.ambient) & static_cast<u64>(cs.inheritable);
    for ( u32 i = 0; i <= posix::cap_last_cap; ++i ) {
      if ( amb & (u64(1) << i) ) posix::cap_ambient_raise(i);
    }
  }

  return 0;
}

};     // namespace micron
