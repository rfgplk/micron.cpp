//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../vector/vector.hpp"

#include "fn.hpp"
#include "fsys.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// functional io free fn combinators

namespace micron
{
namespace io
{

template<typename Fn>
  requires(micron::invocable<Fn, io::file &> && micron::distinct<__unit_if_void_t<micron::invoke_result_t<Fn, io::file &>>, io::error_t>)
auto
with_file(const io::path_t &p, const io::modes m, Fn &&fn)
    -> micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, io::file &>>, io::error_t>
{
  using Ret = micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, io::file &>>, io::error_t>;
  io::file f = open_file(p, m);
  if ( !f.valid() ) [[unlikely]]
    return Ret{ io::error_t(f.raw_fd()) };
  if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, io::file &>> ) {
    micron::forward<Fn>(fn)(f);
    return Ret{ unit_t{} };
  } else {
    return Ret{ micron::forward<Fn>(fn)(f) };
  }
}

template<typename Fn>
  requires micron::invocable<Fn, io::file &>
auto
with_file(const io::path_t &p, Fn &&fn)
{
  return with_file(p, io::modes::read, micron::forward<Fn>(fn));
}

template<typename Fn>
  requires requires(io::file &f, Fn &&fn) { f.modify(micron::forward<Fn>(fn)); }
max_t
modify(const io::path_t &p, Fn &&fn)
{
  io::file f = open_file(p, io::modes::readwrite);
  if ( !f.valid() ) [[unlikely]]
    return f.raw_fd();
  max_t n = f.modify(micron::forward<Fn>(fn));
  if ( n >= 0 ) f.flush();
  return n;
}

template<typename C = micron::vector<micron::string>>
  requires requires(C c, micron::string s) { c.push_back(micron::move(s)); }
micron::option<C, io::error_t>
read_lines(const io::path_t &p)
{
  using Ret = micron::option<C, io::error_t>;
  io::file f = open_file(p, io::modes::read);
  if ( !f.valid() ) [[unlikely]]
    return Ret{ io::error_t(f.raw_fd()) };
  C out{};
  max_t n = f.each_line([&out](const char *s, usize len) { out.push_back(micron::string(s, s + len)); });
  if ( n < 0 ) [[unlikely]]
    return Ret{ io::error_t(static_cast<i32>(n)) };
  return Ret{ micron::move(out) };
}

template<typename C>
  requires requires(C c, micron::string s) { c.push_back(micron::move(s)); }
max_t
read_lines(const io::path_t &p, C &target)
{
  io::file f = open_file(p, io::modes::read);
  if ( !f.valid() ) [[unlikely]]
    return f.raw_fd();
  if constexpr ( requires(C c) { c.fast_clear(); } )
    target.fast_clear();
  else if constexpr ( requires(C c) { c.clear(); } )
    target.clear();
  return f.each_line([&target](const char *s, usize len) { target.push_back(micron::string(s, s + len)); });
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// curried forms

// content | io::write_file_c("/tmp/report.txt")
inline auto
write_file_c(io::path_t p)
{
  return [p = micron::move(p)](const auto &c) { return io::write_file(p, c); };
}

inline auto
append_file_c(io::path_t p)
{
  return [p = micron::move(p)](const auto &c) { return io::append_file(p, c); };
}

// path | io::modify_c(to_upper)  (the path is the datum being piped)
template<typename Fn>
  requires fn_deducible<Fn>
auto
modify_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](const io::path_t &p) mutable { return io::modify(p, fn); };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Haskell style interact :: (String -> String) -> IO ()
// reads all of stdin, applies fn, writes the result to stdout; returns bytes written or -errno
template<typename Fn>
  requires(micron::is_invocable_v<Fn, micron::string &&> && is_string<micron::invoke_result_t<Fn, micron::string &&>>)
max_t
interact(Fn &&fn)
{
  micron::string in;
  byte chunk[4096];
  for ( ;; ) {
    max_t r = posix::read(0, chunk, sizeof(chunk));
    if ( r < 0 ) [[unlikely]] {
      if ( -r == error::interrupted ) continue;
      return r;
    }
    if ( r == 0 ) break;
    in.append(reinterpret_cast<const char *>(chunk), static_cast<usize>(r));
  }
  auto out = micron::forward<Fn>(fn)(micron::move(in));
  return posix::write_all(fd_t{ 1 }, reinterpret_cast<const byte *>(out.c_str()), out.size() * sizeof(typename decltype(out)::value_type));
}

};      // namespace io
};      // namespace micron
