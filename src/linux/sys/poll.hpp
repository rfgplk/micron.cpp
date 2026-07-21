#pragma once

#include "../../errno.hpp"
#include "../../kernel.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "signal.hpp"

namespace micron
{

namespace posix
{

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

constexpr u32 epollin = 0x00000001u;
constexpr u32 epollpri = 0x00000002u;
constexpr u32 epollout = 0x00000004u;
constexpr u32 epollerr = 0x00000008u;
constexpr u32 epollhup = 0x00000010u;
constexpr u32 epollrdnorm = 0x00000040u;
constexpr u32 epollrdband = 0x00000080u;
constexpr u32 epollwrnorm = 0x00000100u;
constexpr u32 epollwrband = 0x00000200u;
constexpr u32 epollmsg = 0x00000400u;
constexpr u32 epollrdhup = 0x00002000u;
constexpr u32 epollexclusive = 0x10000000u;      // linux 4.5
constexpr u32 epollwakeup = 0x20000000u;
constexpr u32 epolloneshot = 0x40000000u;
constexpr u32 epollet = 0x80000000u;

constexpr i32 epoll_ctl_add = 1;
constexpr i32 epoll_ctl_del = 2;
constexpr i32 epoll_ctl_mod = 3;

constexpr i32 epoll_cloexec = 02000000;

struct pollfd {
  i32 fd;
  i16 events;
  i16 revents;
};

union epoll_data {
  void *ptr;
  i32 fd;
  u32 u32v;
  u64 u64v;
};

// WARNING: the kernel struct epoll_event is __attribute__((packed)) __ONLY__ on amd64
// on arm64/arm32 it is NOT packed
#if defined(__micron_arch_amd64)
struct __attribute__((packed)) epoll_event {
#else
struct epoll_event {
#endif
  u32 events;
  epoll_data data;
};

inline int
poll(pollfd &pfd, nfds_t nfds, int timeout)
{
#if defined(__micron_syscall_generic)
  // arm64 has no poll
  if ( timeout < 0 ) return static_cast<int>(micron::syscall(SYS_ppoll, &pfd, nfds, nullptr, nullptr, __sig_syscall_size));
  timespec_t __ts{};
  __ts.tv_sec = timeout / 1000;
  __ts.tv_nsec = static_cast<decltype(__ts.tv_nsec)>(timeout % 1000) * 1000000;
  return static_cast<int>(micron::syscall(SYS_ppoll, &pfd, nfds, &__ts, nullptr, __sig_syscall_size));
#else
  return static_cast<int>(micron::syscall(SYS_poll, &pfd, nfds, timeout));
#endif
}

inline int
ppoll(pollfd &pfd, nfds_t nfds, int timeout, sigset_t &ss)
{
#if defined(__micron_arch_width_32)
  constexpr auto __nr_ppoll = SYS_ppoll_time64;
#else
  constexpr auto __nr_ppoll = SYS_ppoll;
#endif
  if ( timeout < 0 ) return static_cast<int>(micron::syscall(__nr_ppoll, &pfd, nfds, nullptr, &ss, __sig_syscall_size));
  timespec_t __ts{};
  __ts.tv_sec = timeout / 1000;
  __ts.tv_nsec = static_cast<decltype(__ts.tv_nsec)>(timeout % 1000) * 1000000;
  return static_cast<int>(micron::syscall(__nr_ppoll, &pfd, nfds, &__ts, &ss, __sig_syscall_size));
}

int poll_chk(pollfd &pfd, nfds_t nfds, int timeout);

inline int
epoll_create(int size = 1)
{
#if defined(__micron_syscall_generic)
  // arm64 has no epoll_create
  (void)size;
  return static_cast<int>(micron::syscall(SYS_epoll_create1, 0));
#else
  return static_cast<int>(micron::syscall(SYS_epoll_create, size));
#endif
}

inline int
epoll_create1(int flags = 0)
{
  return static_cast<int>(micron::syscall(SYS_epoll_create1, flags));
}

inline int
epoll_ctl(int epfd, int op, int fd, epoll_event &event)
{
  return static_cast<int>(micron::syscall(SYS_epoll_ctl, epfd, op, fd, &event));
}

inline int
epoll_ctl_delete(int epfd, int fd)
{
  return static_cast<int>(micron::syscall(SYS_epoll_ctl, epfd, epoll_ctl_del, fd, nullptr));
}

inline int
epoll_wait(int epfd, epoll_event *events, int maxevents, int timeout)
{
#if defined(__micron_syscall_generic)
  // arm64 has no epoll_wait
  return static_cast<int>(micron::syscall(SYS_epoll_pwait, epfd, events, maxevents, timeout, nullptr, __sig_syscall_size));
#else
  return static_cast<int>(micron::syscall(SYS_epoll_wait, epfd, events, maxevents, timeout));
#endif
}

inline int
epoll_wait_nonblock(int epfd, epoll_event *events, int maxevents)
{
  return epoll_wait(epfd, events, maxevents, 0);
}

inline int
epoll_pwait(int epfd, epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask)
{
  return static_cast<int>(micron::syscall(SYS_epoll_pwait, epfd, events, maxevents, timeout, sigmask, __sig_syscall_size));
}

inline int
epoll_pwait(int epfd, epoll_event *events, int maxevents, int timeout, const sigset_t &sigmask)
{
  return epoll_pwait(epfd, events, maxevents, timeout, &sigmask);
}

// >=5.11
inline kernel::probe_gate __epoll_pwait2_gate{};

inline int
epoll_pwait2(int epfd, epoll_event *events, int maxevents, const timespec_t *timeout, const sigset_t *sigmask)
{
  if ( __epoll_pwait2_gate.open(kernel::feature::epoll_pwait2) ) [[likely]] {
    int r = static_cast<int>(micron::syscall(SYS_epoll_pwait2, epfd, events, maxevents, timeout, sigmask, __sig_syscall_size));
    if ( r != -ENOSYS ) [[likely]]
      return r;
    __epoll_pwait2_gate.demote();
  }
  int ms = -1;      // nullptr -> block indefinitely
  if ( timeout != nullptr ) {
    if ( timeout->tv_sec < 0 || static_cast<u64>(timeout->tv_nsec) >= 1000000000ull ) return -error::invalid_arg;
    constexpr i64 __ms_max = 0x7fffffff;
    const i64 __sec = static_cast<i64>(timeout->tv_sec);
    // nonzero wait must not spin as 0
    const i64 v = __sec > __ms_max / 1000 ? __ms_max : __sec * 1000 + (static_cast<i64>(timeout->tv_nsec) + 999999) / 1000000;
    ms = static_cast<int>(v > __ms_max ? __ms_max : v);
  }
  return epoll_pwait(epfd, events, maxevents, ms, sigmask);
}

inline int
epoll_pwait2(int epfd, epoll_event *events, int maxevents, const timespec_t &timeout, const sigset_t &sigmask)
{
  return epoll_pwait2(epfd, events, maxevents, &timeout, &sigmask);
}

inline int
epoll_pwait2_block(int epfd, epoll_event *events, int maxevents)
{
  return epoll_pwait2(epfd, events, maxevents, static_cast<const timespec_t *>(nullptr), static_cast<const sigset_t *>(nullptr));
}
};      // namespace posix
};      // namespace micron
