//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "sys/poll.hpp"
#include "sys/signal.hpp"
#include "sys/time.hpp"

namespace micron
{
using pollfd = posix::pollfd;
using epoll_event = posix::epoll_event;
using epoll_data = posix::epoll_data;
using sigset_t = posix::sigset_t;
using nfds_t = posix::nfds_t;

template <i32 Events = posix::poll_in>
inline pollfd
make_poll(const io::fd_t &hnd)
{
  if ( hnd.has_error() )
    return {};
  pollfd pfd{};
  pfd.fd = hnd.fd;
  pfd.events = static_cast<i16>(Events);
  return pfd;
}

template <i32 Events = posix::poll_in>
inline pollfd
make_poll(i32 raw_fd)
{
  pollfd pfd{};
  pfd.fd = raw_fd;
  pfd.events = static_cast<i16>(Events);
  return pfd;
}

inline pollfd
make_poll(const io::fd_t &hnd, i32 events)
{
  if ( hnd.has_error() )
    return {};
  pollfd pfd{};
  pfd.fd = hnd.fd;
  pfd.events = static_cast<i16>(events);
  return pfd;
}

inline pollfd
make_poll(i32 raw_fd, i32 events)
{
  pollfd pfd{};
  pfd.fd = raw_fd;
  pfd.events = static_cast<i16>(events);
  return pfd;
}

inline pollfd
make_poll_rw(const io::fd_t &hnd)
{
  return make_poll(hnd, posix::poll_in | posix::poll_out);
}

inline pollfd
make_poll_rw(i32 raw_fd)
{
  return make_poll(raw_fd, posix::poll_in | posix::poll_out);
}

template <usize N>
inline void
make_poll(const io::fd_t (&handles)[N], pollfd (&out)[N], i32 events = posix::poll_in)
{
  for ( usize i = 0; i < N; ++i )
    out[i] = make_poll(handles[i], events);
}

template <usize N>
inline void
make_poll(const i32 (&fds)[N], pollfd (&out)[N], i32 events = posix::poll_in)
{
  for ( usize i = 0; i < N; ++i )
    out[i] = make_poll(fds[i], events);
}

inline i32
poll_for(pollfd &pfd, i32 timeout_ms)
{
  return posix::poll(pfd, 1, timeout_ms);
}

inline i32
poll_for(pollfd &pfd)
{
  return posix::poll(pfd, 1, -1);
}

inline i32
poll_nowait(pollfd &pfd)
{
  return posix::poll(pfd, 1, 0);
}

template <usize N>
inline i32
poll_for(pollfd (&pfds)[N], i32 timeout_ms)
{
  return posix::poll(pfds[0], static_cast<nfds_t>(N), timeout_ms);
}

template <usize N>
inline i32
poll_for(pollfd (&pfds)[N])
{
  return posix::poll(pfds[0], static_cast<nfds_t>(N), -1);
}

template <usize N>
inline i32
poll_nowait(pollfd (&pfds)[N])
{
  return posix::poll(pfds[0], static_cast<nfds_t>(N), 0);
}

inline i32
poll_for(pollfd *pfds, nfds_t count, i32 timeout_ms)
{
  return posix::poll(*pfds, count, timeout_ms);
}

inline i32
poll_for(pollfd *pfds, nfds_t count)
{
  return posix::poll(*pfds, count, -1);
}

inline i32
poll_for(pollfd &pfd, i32 timeout_ms, sigset_t &mask)
{
  return posix::ppoll(pfd, 1, timeout_ms, mask);
}

template <usize N>
inline i32
poll_for(pollfd (&pfds)[N], i32 timeout_ms, sigset_t &mask)
{
  return posix::ppoll(pfds[0], static_cast<nfds_t>(N), timeout_ms, mask);
}

inline bool
poll_readable(const pollfd &pfd)
{
  return pfd.revents & posix::poll_in;
}

inline bool
poll_writable(const pollfd &pfd)
{
  return pfd.revents & posix::poll_out;
}

inline bool
poll_errored(const pollfd &pfd)
{
  return pfd.revents & posix::poll_error;
}

inline bool
poll_hungup(const pollfd &pfd)
{
  return pfd.revents & posix::poll_hangup;
}

inline bool
poll_invalid(const pollfd &pfd)
{
  return pfd.revents & posix::poll_invalid;
}

inline bool
poll_priority(const pollfd &pfd)
{
  return pfd.revents & posix::poll_pri;
}

inline bool
poll_ok(const pollfd &pfd)
{
  return (pfd.revents & (posix::poll_error | posix::poll_hangup | posix::poll_invalid)) == 0 && pfd.revents != 0;
}

template <u32 Events = posix::epollin>
inline epoll_event
make_epoll_event(const io::fd_t &hnd)
{
  epoll_event ev{};
  ev.events = Events;
  ev.data.fd = hnd.fd;
  return ev;
}

template <u32 Events = posix::epollin>
inline epoll_event
make_epoll_event(i32 raw_fd)
{
  epoll_event ev{};
  ev.events = Events;
  ev.data.fd = raw_fd;
  return ev;
}

template <u32 Events = posix::epollin>
inline epoll_event
make_epoll_event(void *ptr)
{
  epoll_event ev{};
  ev.events = Events;
  ev.data.ptr = ptr;
  return ev;
}

template <u32 Events = posix::epollin>
inline epoll_event
make_epoll_event(u64 cookie)
{
  epoll_event ev{};
  ev.events = Events;
  ev.data.u64v = cookie;
  return ev;
}

inline epoll_event
make_epoll_event(i32 raw_fd, u32 events)
{
  epoll_event ev{};
  ev.events = events;
  ev.data.fd = raw_fd;
  return ev;
}

inline epoll_event
make_epoll_event(const io::fd_t &hnd, u32 events)
{
  epoll_event ev{};
  ev.events = events;
  ev.data.fd = hnd.fd;
  return ev;
}

inline epoll_event
make_epoll_event(void *ptr, u32 events)
{
  epoll_event ev{};
  ev.events = events;
  ev.data.ptr = ptr;
  return ev;
}

template <u32 BaseEvents = posix::epollin>
inline epoll_event
make_epoll_et(i32 raw_fd)
{
  return make_epoll_event<BaseEvents | posix::epollet>(raw_fd);
}

template <u32 BaseEvents = posix::epollin>
inline epoll_event
make_epoll_et(const io::fd_t &hnd)
{
  return make_epoll_event<BaseEvents | posix::epollet>(hnd);
}

template <u32 BaseEvents = posix::epollin>
inline epoll_event
make_epoll_oneshot(i32 raw_fd)
{
  return make_epoll_event<BaseEvents | posix::epolloneshot>(raw_fd);
}

template <u32 BaseEvents = posix::epollin>
inline epoll_event
make_epoll_oneshot(const io::fd_t &hnd)
{
  return make_epoll_event<BaseEvents | posix::epolloneshot>(hnd);
}

class epoll_handle
{
public:
  static constexpr i32 invalid_fd = -1;

  ~epoll_handle() { close(); }

  explicit epoll_handle(bool cloexec = true) : epfd_(posix::epoll_create1(cloexec ? posix::epoll_cloexec : 0)) {}

  static epoll_handle
  legacy(i32 size = 1)
  {
    epoll_handle h;
    h.epfd_ = posix::epoll_create(size);
    return h;
  }

  epoll_handle(const epoll_handle &) = delete;
  epoll_handle &operator=(const epoll_handle &) = delete;

  epoll_handle(epoll_handle &&o) noexcept : epfd_(o.epfd_) { o.epfd_ = invalid_fd; }

  epoll_handle &
  operator=(epoll_handle &&o) noexcept
  {
    if ( this != &o ) {
      close();
      epfd_ = o.epfd_;
      o.epfd_ = invalid_fd;
    }
    return *this;
  }

  bool
  valid() const
  {
    return epfd_ >= 0;
  }

  i32
  fd() const
  {
    return epfd_;
  }

  explicit
  operator bool() const
  {
    return valid();
  }

  void
  close()
  {
    if ( valid() ) {
      posix::syscall(SYS_close, epfd_);
      epfd_ = invalid_fd;
    }
  }

  i32
  add(i32 fd, epoll_event &ev) const
  {
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_add, fd, ev);
  }

  i32
  add(const io::fd_t &hnd, epoll_event &ev) const
  {
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_add, hnd.fd, ev);
  }

  template <u32 Events = posix::epollin>
  i32
  add(i32 fd) const
  {
    epoll_event ev = make_epoll_event<Events>(fd);
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_add, fd, ev);
  }

  template <u32 Events = posix::epollin>
  i32
  add(const io::fd_t &hnd) const
  {
    epoll_event ev = make_epoll_event<Events>(hnd);
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_add, hnd.fd, ev);
  }

  template <u32 Events = posix::epollin>
  i32
  add(i32 fd, void *userptr) const
  {
    epoll_event ev = make_epoll_event<Events>(userptr);
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_add, fd, ev);
  }

  i32
  modify(i32 fd, epoll_event &ev) const
  {
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_mod, fd, ev);
  }

  i32
  modify(const io::fd_t &hnd, epoll_event &ev) const
  {
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_mod, hnd.fd, ev);
  }

  template <u32 Events>
  i32
  modify(i32 fd) const
  {
    epoll_event ev = make_epoll_event<Events>(fd);
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_mod, fd, ev);
  }

  i32
  remove(i32 fd) const
  {
    return posix::epoll_ctl_delete(epfd_, fd);
  }

  i32
  remove(const io::fd_t &hnd) const
  {
    return posix::epoll_ctl_delete(epfd_, hnd.fd);
  }

  template <u32 Events = posix::epollin | posix::epolloneshot>
  i32
  rearm(i32 fd) const
  {
    epoll_event ev = make_epoll_event<Events>(fd);
    return posix::epoll_ctl(epfd_, posix::epoll_ctl_mod, fd, ev);
  }

  i32
  wait(epoll_event *events, i32 maxevents, i32 timeout_ms = -1) const
  {
    return posix::epoll_wait(epfd_, events, maxevents, timeout_ms);
  }

  i32
  wait_nowait(epoll_event *events, i32 maxevents) const
  {
    return posix::epoll_wait(epfd_, events, maxevents, 0);
  }

  template <usize N>
  i32
  wait(epoll_event (&events)[N], i32 timeout_ms = -1) const
  {
    return posix::epoll_wait(epfd_, events, static_cast<i32>(N), timeout_ms);
  }

  template <usize N>
  i32
  wait_nowait(epoll_event (&events)[N]) const
  {
    return posix::epoll_wait(epfd_, events, static_cast<i32>(N), 0);
  }

  i32
  wait(epoll_event *events, i32 maxevents, i32 timeout_ms, const sigset_t &mask) const
  {
    return posix::epoll_pwait(epfd_, events, maxevents, timeout_ms, mask);
  }

  i32
  wait(epoll_event *events, i32 maxevents, i32 timeout_ms, const sigset_t *mask) const
  {
    return posix::epoll_pwait(epfd_, events, maxevents, timeout_ms, mask);
  }

  template <usize N>
  i32
  wait(epoll_event (&events)[N], i32 timeout_ms, const sigset_t &mask) const
  {
    return posix::epoll_pwait(epfd_, events, static_cast<i32>(N), timeout_ms, mask);
  }

  i32
  wait_ns(epoll_event *events, i32 maxevents, const timespec &timeout, const sigset_t &mask) const
  {
    return posix::epoll_pwait2(epfd_, events, maxevents, timeout, mask);
  }

  i32
  wait_ns(epoll_event *events, i32 maxevents, const timespec &timeout) const
  {
    return posix::epoll_pwait2(epfd_, events, maxevents, &timeout, nullptr);
  }

  i32
  wait_ns_block(epoll_event *events, i32 maxevents) const
  {
    return posix::epoll_pwait2_block(epfd_, events, maxevents);
  }

  template <usize N>
  i32
  wait_ns(epoll_event (&events)[N], const timespec &timeout, const sigset_t &mask) const
  {
    return posix::epoll_pwait2(epfd_, events, static_cast<i32>(N), timeout, mask);
  }

private:
  // private default ctor used by legacy()
  epoll_handle() : epfd_(invalid_fd) {}

  i32 epfd_;
};

inline i32
epoll_add(i32 epfd, i32 fd, u32 events = posix::epollin)
{
  epoll_event ev = make_epoll_event(fd, events);
  return posix::epoll_ctl(epfd, posix::epoll_ctl_add, fd, ev);
}

inline i32
epoll_add(i32 epfd, const io::fd_t &hnd, u32 events = posix::epollin)
{
  return epoll_add(epfd, hnd.fd, events);
}

inline i32
epoll_add_ptr(i32 epfd, i32 fd, void *ptr, u32 events = posix::epollin)
{
  epoll_event ev = make_epoll_event(ptr, events);
  return posix::epoll_ctl(epfd, posix::epoll_ctl_add, fd, ev);
}

inline i32
epoll_mod(i32 epfd, i32 fd, u32 events)
{
  epoll_event ev = make_epoll_event(fd, events);
  return posix::epoll_ctl(epfd, posix::epoll_ctl_mod, fd, ev);
}

inline i32
epoll_del(i32 epfd, i32 fd)
{
  return posix::epoll_ctl_delete(epfd, fd);
}

inline i32
epoll_wait_for(i32 epfd, epoll_event &event, i32 timeout_ms = -1)
{
  return posix::epoll_wait(epfd, &event, 1, timeout_ms);
}

inline i32
epoll_wait_for(i32 epfd, epoll_event *events, i32 maxevents, i32 timeout_ms = -1)
{
  return posix::epoll_wait(epfd, events, maxevents, timeout_ms);
}

template <usize N>
inline i32
epoll_wait_for(i32 epfd, epoll_event (&events)[N], i32 timeout_ms = -1)
{
  return posix::epoll_wait(epfd, events, static_cast<i32>(N), timeout_ms);
}

inline i32
epoll_wait_for(i32 epfd, epoll_event *events, i32 maxevents, i32 timeout_ms, const sigset_t &mask)
{
  return posix::epoll_pwait(epfd, events, maxevents, timeout_ms, mask);
}

inline bool
epoll_readable(const epoll_event &ev)
{
  return ev.events & posix::epollin;
}

inline bool
epoll_writable(const epoll_event &ev)
{
  return ev.events & posix::epollout;
}

inline bool
epoll_errored(const epoll_event &ev)
{
  return ev.events & posix::epollerr;
}

inline bool
epoll_hungup(const epoll_event &ev)
{
  return ev.events & posix::epollhup;
}

inline bool
epoll_peer_hup(const epoll_event &ev)
{
  return ev.events & posix::epollrdhup;
}

inline bool
epoll_priority(const epoll_event &ev)
{
  return ev.events & posix::epollpri;
}

inline bool
epoll_ok(const epoll_event &ev)
{
  return (ev.events & (posix::epollerr | posix::epollhup)) == 0 && ev.events != 0;
}
};     // namespace micron
