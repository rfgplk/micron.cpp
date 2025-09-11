#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "signal.hpp"

namespace micron
{
struct pollfd {
  i32 fd;
  i16 events;
  i16 revents;
};

using nfds_t = unsigned long int;

constexpr i32 poll_in = 0x001;
constexpr i32 poll_pri = 0x002;
constexpr i32 poll_out = 0x004;
constexpr i32 poll_error = 0x008;
constexpr i32 poll_hangup = 0x010;
constexpr i32 poll_invalid = 0x020;
constexpr i32 poll_read_normal = 0x040;
constexpr i32 poll_read_prio = 0x080;
constexpr i32 poll_write_normal = 0x100;
constexpr i32 poll_write_prio = 0x200;
constexpr i32 poll_msg = 0x400;
constexpr i32 poll_remove = 0x1000;
constexpr i32 poll_read_hangup = 0x2000;

// start funcs

int
poll(pollfd &pfd, nfds_t nfds, int timeout)
{
  return static_cast<int>(micron::syscall(SYS_poll, &pfd, nfds, timeout));
}
int
ppoll(pollfd &pfd, nfds_t nfds, int timeout, sigset_t &ss)
{
  return static_cast<int>(micron::syscall(SYS_ppoll, &pfd, nfds, timeout, &ss));
}

int poll_chk(pollfd &pfd, nfds_t nfds, int timeout);

};
