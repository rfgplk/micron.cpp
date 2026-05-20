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

class sampler
{
  GLuint __h = 0;

public:
  ~sampler() { reset(); }

  sampler()
  {
    if ( !glGenSamplers ) throw except::library_error("sampler: gl not loaded");
    glGenSamplers(1, &__h);
    if ( !__h ) throw except::library_error("sampler: glGenSamplers returned 0");
  }

  sampler(const sampler &) = delete;

  sampler(sampler &&o) noexcept : __h(o.__h) { o.__h = 0; }

  sampler &operator=(const sampler &) = delete;

  sampler &
  operator=(sampler &&o) noexcept
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
    if ( __h && glDeleteSamplers ) glDeleteSamplers(1, &__h);
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
  bind(GLuint unit) const noexcept
  {
    if ( __h && glBindSampler ) glBindSampler(unit, __h);
  }

  void
  parameter(GLenum pname, GLint param) noexcept
  {
    if ( __h && glSamplerParameteri ) glSamplerParameteri(__h, pname, param);
  }

  void
  debug_label(const char *name) noexcept
  {
    if ( __h && glObjectLabel && name ) glObjectLabel(0x82E6 /* GL_SAMPLER */, __h, -1, name);
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
