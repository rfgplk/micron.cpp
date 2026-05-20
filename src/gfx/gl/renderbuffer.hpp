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

class renderbuffer
{
  GLuint __h = 0;

public:
  ~renderbuffer() { reset(); }

  renderbuffer()
  {
    if ( !glGenRenderbuffers ) throw except::library_error("renderbuffer: gl not loaded");
    glGenRenderbuffers(1, &__h);
    if ( !__h ) throw except::library_error("renderbuffer: glGenRenderbuffers returned 0");
  }

  renderbuffer(const renderbuffer &) = delete;

  renderbuffer(renderbuffer &&o) noexcept : __h(o.__h) { o.__h = 0; }

  renderbuffer &operator=(const renderbuffer &) = delete;

  renderbuffer &
  operator=(renderbuffer &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      o.__h = 0;
    }
    return *this;
  }

  void
  reset() noexcept
  {
    if ( __h && glDeleteRenderbuffers ) glDeleteRenderbuffers(1, &__h);
    __h = 0;
  }

  GLuint
  handle() const noexcept
  {
    return __h;
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
    if ( __h && glBindRenderbuffer ) glBindRenderbuffer(GL_RENDERBUFFER, __h);
  }

  void
  storage(GLenum internalformat, GLsizei width, GLsizei height) noexcept
  {
    if ( !__h || !glRenderbufferStorage ) return;
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8D41 /* GL_RENDERBUFFER */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
