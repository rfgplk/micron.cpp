//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "opengl.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

class fence
{
  GLsync __h = nullptr;

public:
  ~fence() { reset(); }

  fence() = default;

  fence(const fence &) = delete;

  fence(fence &&o) noexcept : __h(o.__h) { o.__h = nullptr; }

  fence &operator=(const fence &) = delete;

  fence &
  operator=(fence &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      o.__h = nullptr;
    }
    return *this;
  }

  static fence
  insert(GLenum condition = 0x9117 /* GL_SYNC_GPU_COMMANDS_COMPLETE */) noexcept
  {
    fence f;
    if ( glFenceSync ) f.__h = glFenceSync(condition, 0);
    return f;
  }

  void
  reset() noexcept
  {
    if ( __h && glDeleteSync ) glDeleteSync(__h);
    __h = nullptr;
  }

  GLsync
  handle() const noexcept
  {
    return __h;
  }

  bool
  valid() const noexcept
  {
    return __h != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  bool
  wait_for(GLuint64 timeout_ns) noexcept
  {
    if ( !__h || !glClientWaitSync ) return false;
    constexpr GLbitfield flush_bit = 0x00000001 /* GL_SYNC_FLUSH_COMMANDS_BIT */;
    GLenum r = glClientWaitSync(__h, flush_bit, timeout_ns);
    return r == 0x911C /* GL_ALREADY_SIGNALED */ || r == 0x911D /* GL_CONDITION_SATISFIED */;
  }

  bool
  is_signaled() noexcept
  {
    return wait_for(0);
  }

  void
  server_wait() noexcept
  {
    if ( __h && glWaitSync ) glWaitSync(__h, 0, 0xFFFFFFFFFFFFFFFFull /* GL_TIMEOUT_IGNORED */);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
