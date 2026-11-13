//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"
#if defined(__micron_arch_wasm32)
#error "micron: the native OpenGL backend is unavailable in WebAssembly"
#else

#include "gfx/gl/gl.hpp"

namespace micron
{
namespace gl = ::micron::gfx::gl;
};      // namespace micron

#endif
