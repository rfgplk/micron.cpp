//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if defined(MICRON_CORO_URING)

#include "../../tasks/spawn.hpp"
#include "../../vector/vector.hpp"

#include "../fsys.hpp"
#include "file.hpp"

namespace micron
{
namespace io
{
namespace coro
{

[[nodiscard]] inline micron::task<file>
open_file(io::path_t p, modes m = modes::read, open_opts opts = {})
{
  i32 fd = co_await micron::coro::io::openat(posix::at_fdcwd, p.c_str(), compose_open_flags(m, opts), opts.perms);

  const bool ap = (m == modes::append || m == modes::appendread);
  const bool chk = (m != modes::append && m != modes::create && m != modes::readwritecreate);
  if ( fd >= 0 && chk ) {
    stat_t st{};
    if ( i32 e = static_cast<i32>(posix::fstat(fd_t{ fd }, st)); e != 0 ) [[unlikely]] {
      i32 c = co_await micron::coro::io::close(fd);
      (void)c;
      co_return file(fd_t{ e }, p.c_str());
    }
    if ( !(posix::__impl::stat_is_reg(st) || posix::__impl::stat_is_chr(st)) ) [[unlikely]] {
      i32 c = co_await micron::coro::io::close(fd);
      (void)c;
      co_return file(fd_t{ posix::__impl::stat_is_dir(st) ? -error::is_a_dir : -error::invalid_arg }, p.c_str());
    }
  }
  file out(fd_t{ fd }, p.c_str(), ap);
  co_return out;
}

[[nodiscard]] inline micron::task<file>
create_file(io::path_t p, u32 mode = 0644)
{
  const i32 flags = posix::o_rdwr | posix::o_create | posix::o_excl | posix::o_cloexec;
  i32 fd = co_await micron::coro::io::openat(posix::at_fdcwd, p.c_str(), static_cast<u32>(flags), mode);
  co_return file(fd_t{ fd }, p.c_str());
}

template<typename T = micron::string>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
[[nodiscard]] micron::task<micron::option<T, io::error_t>>
read_file(io::path_t p)
{
  using Ret = micron::option<T, io::error_t>;
  file f = co_await coro::open_file(micron::move(p));
  if ( !f.valid() ) [[unlikely]]
    co_return Ret{ io::error_t(f.raw_fd()) };
  auto r = co_await f.template read<T>();
  co_return r;
}

template<typename T>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
[[nodiscard]] micron::task<max_t>
read_file(io::path_t p, T &target)
{
  file f = co_await coro::open_file(micron::move(p));
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  max_t r = co_await f.read(target);
  co_return r;
}

template<typename C>
[[nodiscard]] micron::task<max_t>
write_file(io::path_t p, const C &c)
{
  file f = co_await coro::open_file(micron::move(p), modes::write, open_opts{});
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  max_t w = co_await f.write(c);
  co_return w;
}

template<typename C>
[[nodiscard]] micron::task<max_t>
write_file_sync(io::path_t p, const C &c)
{
  file f = co_await coro::open_file(micron::move(p), modes::write, open_opts{});
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  max_t w = co_await f.write(c);
  if ( w < 0 ) [[unlikely]]
    co_return w;
  i32 s = co_await f.sync();
  if ( s != 0 ) [[unlikely]]
    co_return s;
  co_return w;
}

template<typename C>
[[nodiscard]] micron::task<max_t>
append_file(io::path_t p, const C &c)
{
  file f = co_await coro::open_file(micron::move(p), modes::append, open_opts{});
  if ( !f.valid() ) [[unlikely]]
    co_return f.raw_fd();
  max_t w = co_await f.write(c);
  co_return w;
}

[[nodiscard]] inline micron::task<max_t>
remove(io::path_t p)
{
  i32 r = co_await micron::coro::io::unlinkat(posix::at_fdcwd, p.c_str(), 0);
  co_return static_cast<max_t>(r);
}

[[nodiscard]] inline micron::task<max_t>
rename(io::path_t from, io::path_t to)
{
  i32 r = co_await micron::coro::io::renameat(posix::at_fdcwd, from.c_str(), posix::at_fdcwd, to.c_str());
  co_return static_cast<max_t>(r);
}

[[nodiscard]] inline micron::task<max_t>
mkdir(io::path_t p, u32 mode = 0755)
{
  i32 r = co_await micron::coro::io::mkdirat(posix::at_fdcwd, p.c_str(), mode);
  co_return static_cast<max_t>(r);
}

[[nodiscard]] inline micron::task<micron::option<posix::statx_t, io::error_t>>
stat(io::path_t p)
{
  using Ret = micron::option<posix::statx_t, io::error_t>;
  posix::statx_t sx{};
  i32 r = co_await micron::coro::io::statx(posix::at_fdcwd, p.c_str(), 0, 0x7ff /*STATX_BASIC_STATS*/, &sx);
  if ( r != 0 ) [[unlikely]]
    co_return Ret{ io::error_t(r) };
  co_return Ret{ sx };
}

[[nodiscard]] inline micron::task<max_t>
file_size(io::path_t p)
{
  posix::statx_t sx{};
  i32 r = co_await micron::coro::io::statx(posix::at_fdcwd, p.c_str(), 0, 0x7ff, &sx);
  if ( r != 0 ) [[unlikely]]
    co_return static_cast<max_t>(r);
  co_return static_cast<max_t>(sx.stx_size);
}

[[nodiscard]] inline micron::task<bool>
exists(io::path_t p)
{
  posix::statx_t sx{};
  i32 r = co_await micron::coro::io::statx(posix::at_fdcwd, p.c_str(), 0, 0x7ff, &sx);
  co_return r == 0;
}

[[nodiscard]] inline max_t
__copy_probe(fd_t src, const char *to, u32 &perms) noexcept
{
  stat_t st{};
  if ( posix::fstat(src, st) < 0 ) [[unlikely]]
    return -error::io_error;
  stat_t dst_st{};
  if ( posix::exists(to, dst_st) && dst_st.st_dev == st.st_dev && dst_st.st_ino == st.st_ino ) [[unlikely]]
    return -error::invalid_arg;
  perms = st.st_mode & 0777u;
  return 0;
}

[[nodiscard]] inline micron::task<max_t>
copy(io::path_t from, io::path_t to)
{
  file src = co_await coro::open_file(micron::move(from));
  if ( !src.valid() ) [[unlikely]]
    co_return src.raw_fd();

  open_opts dopts{};
  if ( max_t g = coro::__copy_probe(src.fd(), to.c_str(), dopts.perms); g < 0 ) [[unlikely]]
    co_return g;
  file dst = co_await coro::open_file(micron::move(to), modes::write, dopts);
  if ( !dst.valid() ) [[unlikely]]
    co_return dst.raw_fd();
  const i32 slot = micron::coro::acquire_fixed();
  byte *buf = slot >= 0 ? micron::coro::fixed_ptr(slot) : nullptr;
  micron::buffer heap(slot >= 0 ? 0 : (256 * 1024));
  if ( buf == nullptr ) buf = reinterpret_cast<byte *>(heap.data());
  const usize cap = slot >= 0 ? micron::coro::fixed_size() : heap.size();
  max_t total = 0;
  for ( ;; ) {
    max_t r = co_await src.read_at(static_cast<u64>(total), buf, cap);
    if ( r < 0 ) [[unlikely]] {
      if ( slot >= 0 ) micron::coro::release_fixed(slot);
      co_return r;
    }
    if ( r == 0 ) break;
    max_t w = co_await dst.write_at(static_cast<u64>(total), buf, static_cast<usize>(r));
    if ( w != r ) [[unlikely]] {
      if ( slot >= 0 ) micron::coro::release_fixed(slot);
      co_return w < 0 ? w : -error::io_error;
    }
    total += r;
  }
  if ( slot >= 0 ) micron::coro::release_fixed(slot);

  i32 fe = co_await dst.sync();
  if ( fe != 0 ) [[unlikely]]
    co_return static_cast<max_t>(fe);
  co_return total;
}

[[nodiscard]] inline micron::task<max_t>
move(io::path_t from, io::path_t to)
{
  i32 r = co_await micron::coro::io::renameat(posix::at_fdcwd, from.c_str(), posix::at_fdcwd, to.c_str());
  if ( r == 0 ) co_return 0;
  if ( r != -18 /*EXDEV*/ ) [[unlikely]]
    co_return static_cast<max_t>(r);
  max_t c = co_await coro::copy(from, to);
  if ( c < 0 ) [[unlikely]]
    co_return c;

  max_t u = co_await coro::remove(micron::move(from));
  co_return u;
}

namespace __impl
{

inline constexpr usize __fan_window = 64;

template<typename T>
[[nodiscard]] micron::task<micron::option<T, io::error_t>>
__load_one(io::path_t p)
{
  auto r = co_await coro::read_file<T>(micron::move(p));
  co_return r;
}

};      // namespace __impl

template<typename T = micron::string>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
[[nodiscard]] micron::task<micron::vector<micron::option<T, io::error_t>>>
read_files(micron::vector<io::path_t> paths)
{
  micron::vector<micron::option<T, io::error_t>> out;
  out.reserve(paths.size());
  usize base = 0;
  while ( base < paths.size() ) {
    usize w = paths.size() - base;
    if ( w > __impl::__fan_window ) w = __impl::__fan_window;
    auto part = co_await micron::coro::spawn_many(w, [&paths, base](usize i) -> micron::task<micron::option<T, io::error_t>> {
      return coro::__impl::__load_one<T>(paths[base + i]);
    });
    for ( usize i = 0; i < part.size(); ++i ) out.push_back(micron::move(part[i]));
    base += w;
  }
  co_return out;
}

struct write_spec {
  io::path_t path;
  const byte *data = nullptr;
  usize len = 0;
};

[[nodiscard]] inline micron::task<micron::vector<max_t>>
write_files(micron::vector<write_spec> specs)
{
  micron::vector<max_t> out;
  out.reserve(specs.size());
  usize base = 0;
  while ( base < specs.size() ) {
    usize w = specs.size() - base;
    if ( w > __impl::__fan_window ) w = __impl::__fan_window;
    auto part = co_await micron::coro::spawn_many(w, [&specs, base](usize i) -> micron::task<max_t> {
      const write_spec &s = specs[base + i];
      return [](io::path_t p, const byte *d, usize n) -> micron::task<max_t> {
        file f = co_await coro::open_file(micron::move(p), modes::write, open_opts{});
        if ( !f.valid() ) [[unlikely]]
          co_return f.raw_fd();
        max_t r = co_await f.write(static_cast<const void *>(d), n);
        co_return r;
      }(s.path, s.data, s.len);
    });
    for ( usize i = 0; i < part.size(); ++i ) out.push_back(part[i]);
    base += w;
  }
  co_return out;
}

[[nodiscard]] inline micron::task<micron::vector<micron::option<posix::statx_t, io::error_t>>>
stat_many(micron::vector<io::path_t> paths)
{
  micron::vector<micron::option<posix::statx_t, io::error_t>> out;
  out.reserve(paths.size());
  usize base = 0;
  while ( base < paths.size() ) {
    usize w = paths.size() - base;
    if ( w > __impl::__fan_window ) w = __impl::__fan_window;
    auto part = co_await micron::coro::spawn_many(
        w, [&paths, base](usize i) -> micron::task<micron::option<posix::statx_t, io::error_t>> { return coro::stat(paths[base + i]); });
    for ( usize i = 0; i < part.size(); ++i ) out.push_back(micron::move(part[i]));
    base += w;
  }
  co_return out;
}

};      // namespace coro
};      // namespace io
};      // namespace micron

#endif
