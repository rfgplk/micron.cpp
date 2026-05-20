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

enum class buffer_target : GLenum {
  array = GL_ARRAY_BUFFER,
  element_array = GL_ELEMENT_ARRAY_BUFFER,
  uniform = GL_UNIFORM_BUFFER,
  shader_storage = GL_SHADER_STORAGE_BUFFER,
  copy_read = GL_COPY_READ_BUFFER,
  copy_write = GL_COPY_WRITE_BUFFER,
  pixel_pack = GL_PIXEL_PACK_BUFFER,
  pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
};

enum class buffer_usage : GLenum {
  static_draw = GL_STATIC_DRAW,
  static_read = GL_STATIC_READ,
  static_copy = GL_STATIC_COPY,
  stream_draw = GL_STREAM_DRAW,
  stream_read = GL_STREAM_READ,
  stream_copy = GL_STREAM_COPY,
  dynamic_draw = GL_DYNAMIC_DRAW,
  dynamic_read = GL_DYNAMIC_READ,
  dynamic_copy = GL_DYNAMIC_COPY,
};

class buffer
{
private:
  GLuint __h = 0;
  buffer_target __target = buffer_target::array;

public:
  ~buffer() { reset(); }

  buffer() = default;

  explicit buffer(buffer_target t) : __target(t)
  {
    if ( !glGenBuffers ) throw except::library_error("buffer: gl not loaded");
    glGenBuffers(1, &__h);
    if ( !__h ) throw except::library_error("buffer: glGenBuffers returned 0");
  }

  buffer(const buffer &) = delete;

  buffer(buffer &&o) noexcept : __h(o.__h), __target(o.__target) { o.__h = 0; }

  buffer &operator=(const buffer &) = delete;

  buffer &
  operator=(buffer &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      __target = o.__target;
      o.__h = 0;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( __h && glDeleteBuffers ) glDeleteBuffers(1, &__h);
    __h = 0;
  }

  GLuint
  handle() const noexcept
  {
    return __h;
  }

  buffer_target
  target() const noexcept
  {
    return __target;
  }

  bool
  valid() const noexcept
  {
    return __h != 0;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  void
  bind() const noexcept
  {
    if ( __h && glBindBuffer ) glBindBuffer(static_cast<GLenum>(__target), __h);
  }

  void
  unbind() const noexcept
  {
    if ( glBindBuffer ) glBindBuffer(static_cast<GLenum>(__target), 0);
  }

  void
  data(const void *src, GLsizeiptr size, buffer_usage usage) noexcept
  {
    if ( !__h || !glBufferData ) return;
    bind();
    glBufferData(static_cast<GLenum>(__target), size, src, static_cast<GLenum>(usage));
  }

  void
  sub_data(GLintptr offset, GLsizeiptr size, const void *src) noexcept
  {
    if ( !__h || !glBufferSubData ) return;
    bind();
    glBufferSubData(static_cast<GLenum>(__target), offset, size, src);
  }

  void *
  map(GLintptr offset, GLsizeiptr length, GLbitfield access) noexcept
  {
    if ( !__h || !glMapBufferRange ) return nullptr;
    bind();
    return glMapBufferRange(static_cast<GLenum>(__target), offset, length, access);
  }

  bool
  unmap() noexcept
  {
    if ( !__h || !glUnmapBuffer ) return false;
    bind();
    return glUnmapBuffer(static_cast<GLenum>(__target)) != GL_FALSE;
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x82E0 /* GL_BUFFER */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
