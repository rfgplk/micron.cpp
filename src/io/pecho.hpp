//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "echo.hpp"

#include "../mutex/mutex.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  thread safe printing

namespace micron
{
namespace io
{

// will be lazily loaded
inline micron::mutex __p_print_lock;
inline constinit micron::__global_pointer<micron::io::stream<__global_buffer_size, __global_buffer_chunk>> __p_buffer_stdout(nullptr);

namespace __pecho_impl
{

struct __plock {
  __plock() { __p_print_lock.lock(); }

  ~__plock() { __p_print_lock.unlock(); }

  __plock(const __plock &) = delete;
  __plock &operator=(const __plock &) = delete;
};

inline void
__ensure_stdout(void)
{
  if ( !__p_buffer_stdout ) __p_buffer_stdout = micron::make_global<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>();
}

};      // namespace __pecho_impl

struct locked_stdout_sink {
  static max_t
  put(const char *p, usize n)
  {
    if ( __p_buffer_stdout->full(n) ) {
      (*__p_buffer_stdout) >> io::stdout;
      if ( n >= static_cast<usize>(__global_buffer_size) )     
        return posix::write_all(io::stdout, reinterpret_cast<const byte *>(p), n);
    }
    __p_buffer_stdout->append(reinterpret_cast<const byte *>(p), n);
    return static_cast<max_t>(n);
  }

  static max_t
  put(char c)
  {
    if ( __p_buffer_stdout->full(1) ) (*__p_buffer_stdout) >> io::stdout;
    __p_buffer_stdout->append(reinterpret_cast<const byte *>(&c), 1);
    return 1;
  }

  static max_t
  flush(void)
  {
    (*__p_buffer_stdout) >> io::stdout;  
    return 0;
  }
};

template<typename First, typename... Rest>
  requires(!echo_target<First>)
inline max_t
pecho(const First &first, const Rest &...rest)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = __echo_impl::run(s, true, first, rest...);
  s.flush();
  return r;
}

inline max_t
pecho(void)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = s.put('\n');
  s.flush();
  return r;
}

template<typename First, typename... Rest>
  requires(!echo_target<First>)
inline max_t
pechon(const First &first, const Rest &...rest)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = __echo_impl::run(s, false, first, rest...);
  s.flush();
  return r;
}

template<typename... Args>
inline max_t
pechof(const char *fmt, const Args &...args)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = __echo_impl::format_to_sink(s, fmt, args...);
  r += s.put('\n');
  s.flush();
  return r;
}

template<typename... Args>
inline max_t
pechofn(const char *fmt, const Args &...args)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = __echo_impl::format_to_sink(s, fmt, args...);
  s.flush();
  return r;
}

template<typename... T>
inline max_t
pprint(const T &...str)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = 0;
  ((r += printk(s, str)), ...);
  s.flush();
  return r;
}

template<typename... T>
inline max_t
pprintln(const T &...str)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = 0;
  ((r += printk(s, str)), ...);
  r += s.put('\n');
  s.flush();
  return r;
}

template<typename... T>
inline max_t
pprintn(const T &...str)
{
  __pecho_impl::__plock g;
  __pecho_impl::__ensure_stdout();
  locked_stdout_sink s;
  max_t r = 0;
  ((r += printkn(s, str)), ...);
  s.flush();
  return r;
}

};      // namespace io
};      // namespace micron
