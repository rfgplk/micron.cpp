//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "opengl.hpp"
#include "renderbuffer.hpp"
#include "texture.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{

enum class fb_target : GLenum {
  both = GL_FRAMEBUFFER,
  draw = GL_DRAW_FRAMEBUFFER,
  read = GL_READ_FRAMEBUFFER,
};

class framebuffer
{
private:
  GLuint __h = 0;
  fb_target __target = fb_target::both;

public:
  ~framebuffer() { reset(); }

  explicit framebuffer(fb_target t = fb_target::both) : __target(t)
  {
    if ( !glGenFramebuffers ) throw except::library_error("framebuffer: gl not loaded");
    glGenFramebuffers(1, &__h);
    if ( !__h ) throw except::library_error("framebuffer: glGenFramebuffers returned 0");
  }

  framebuffer(const framebuffer &) = delete;

  framebuffer(framebuffer &&o) noexcept : __h(o.__h), __target(o.__target) { o.__h = 0; }

  framebuffer &operator=(const framebuffer &) = delete;

  framebuffer &
  operator=(framebuffer &&o) noexcept
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
    if ( __h && glDeleteFramebuffers ) glDeleteFramebuffers(1, &__h);
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
    if ( __h && glBindFramebuffer ) glBindFramebuffer(static_cast<GLenum>(__target), __h);
  }

  static void
  bind_default(fb_target t = fb_target::both) noexcept
  {
    if ( glBindFramebuffer ) glBindFramebuffer(static_cast<GLenum>(t), 0);
  }

  void
  attach_color(GLuint slot, const texture &tex, GLint level = 0) noexcept
  {
    if ( !__h || !glFramebufferTexture2D ) return;
    bind();
    glFramebufferTexture2D(static_cast<GLenum>(__target), GL_COLOR_ATTACHMENT0 + slot, static_cast<GLenum>(tex.target()), tex.handle(),
                           level);
  }

  void
  attach_depth(const texture &tex, GLint level = 0) noexcept
  {
    if ( !__h || !glFramebufferTexture2D ) return;
    bind();
    glFramebufferTexture2D(static_cast<GLenum>(__target), GL_DEPTH_ATTACHMENT, static_cast<GLenum>(tex.target()), tex.handle(), level);
  }

  void
  attach_depth_rb(const renderbuffer &rb) noexcept
  {
    if ( !__h || !glFramebufferRenderbuffer ) return;
    bind();
    glFramebufferRenderbuffer(static_cast<GLenum>(__target), GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb.handle());
  }

  void
  attach_depth_stencil_rb(const renderbuffer &rb) noexcept
  {
    if ( !__h || !glFramebufferRenderbuffer ) return;
    bind();
    glFramebufferRenderbuffer(static_cast<GLenum>(__target), GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb.handle());
  }

  GLenum
  check() const
  {
    if ( !__h || !glCheckFramebufferStatus ) throw except::logic_error("framebuffer: empty handle");
    bind();
    return glCheckFramebufferStatus(static_cast<GLenum>(__target));
  }

  void
  require_complete() const
  {
    GLenum s = check();
    if ( s != GL_FRAMEBUFFER_COMPLETE ) throw except::library_error("framebuffer: not complete");
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x8D40 /* GL_FRAMEBUFFER */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
