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

enum class tex_target : GLenum {
  d1 = GL_TEXTURE_1D,
  d2 = GL_TEXTURE_2D,
  d3 = GL_TEXTURE_3D,
  cube = GL_TEXTURE_CUBE_MAP,
  d1_array = GL_TEXTURE_1D_ARRAY,
  d2_array = GL_TEXTURE_2D_ARRAY,
  rect = GL_TEXTURE_RECTANGLE,
  buffer = GL_TEXTURE_BUFFER,
};

class texture
{
  GLuint __h = 0;
  tex_target __target = tex_target::d2;

public:
  ~texture() { reset(); }

  texture() = default;

  explicit texture(tex_target t) : __target(t)
  {
    if ( !glGenTextures ) throw except::library_error("texture: gl not loaded");
    glGenTextures(1, &__h);
    if ( !__h ) throw except::library_error("texture: glGenTextures returned 0");
  }

  texture(const texture &) = delete;

  texture(texture &&o) noexcept : __h(o.__h), __target(o.__target) { o.__h = 0; }

  texture &operator=(const texture &) = delete;

  texture &
  operator=(texture &&o) noexcept
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
    if ( __h && glDeleteTextures ) glDeleteTextures(1, &__h);
    __h = 0;
  }

  GLuint
  handle() const noexcept
  {
    return __h;
  }

  tex_target
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
    if ( __h && glBindTexture ) glBindTexture(static_cast<GLenum>(__target), __h);
  }

  void
  bind_unit(GLuint unit) const noexcept
  {
    if ( !__h || !glActiveTexture || !glBindTexture ) return;
    glActiveTexture(0x84C0 /* GL_TEXTURE0 */ + unit);
    glBindTexture(static_cast<GLenum>(__target), __h);
  }

  void
  storage_2d(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) noexcept
  {
    if ( !__h || !glTexStorage2D ) return;
    bind();
    glTexStorage2D(static_cast<GLenum>(__target), levels, internalformat, width, height);
  }

  void
  image_2d(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
  {
    if ( !__h || !glTexImage2D ) return;
    bind();
    glTexImage2D(static_cast<GLenum>(__target), level, static_cast<GLint>(internalformat), width, height, 0, format, type, pixels);
  }

  void
  sub_image_2d(GLint level, GLint xofs, GLint yofs, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) noexcept
  {
    if ( !__h || !glTexSubImage2D ) return;
    bind();
    glTexSubImage2D(static_cast<GLenum>(__target), level, xofs, yofs, width, height, format, type, pixels);
  }

  void
  parameter(GLenum pname, GLint param) noexcept
  {
    if ( !__h || !glTexParameteri ) return;
    bind();
    glTexParameteri(static_cast<GLenum>(__target), pname, param);
  }

  void
  generate_mipmap() noexcept
  {
    if ( !__h || !glGenerateMipmap ) return;
    bind();
    glGenerateMipmap(static_cast<GLenum>(__target));
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x1702 /* GL_TEXTURE */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
