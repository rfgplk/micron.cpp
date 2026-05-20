//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "../../../linux/dynamic.hpp"
#include "../../../memory/cstring.hpp"

#include "../../platform/__bits/__x11_types.hpp"
#include "../../platform/x11.hpp"
#include "../__bits/__glx_types.hpp"

namespace micron
{
namespace gfx
{
namespace gl
{
using namespace ::micron::gfx::platform;

struct glx_lib_t {
  host_dso host;
  PFN_glXGetProcAddress glXGetProcAddress = nullptr;
  PFN_glXChooseFBConfig glXChooseFBConfig = nullptr;
  PFN_glXGetVisualFromFBConfig glXGetVisualFromFBConfig = nullptr;
  PFN_glXGetFBConfigAttrib glXGetFBConfigAttrib = nullptr;
  PFN_glXCreateNewContext glXCreateNewContext = nullptr;
  PFN_glXCreateContextAttribsARB glXCreateContextAttribsARB = nullptr;
  PFN_glXDestroyContext glXDestroyContext = nullptr;
  PFN_glXMakeCurrent glXMakeCurrent = nullptr;
  PFN_glXMakeContextCurrent glXMakeContextCurrent = nullptr;
  PFN_glXSwapBuffers glXSwapBuffers = nullptr;
  PFN_glXSwapIntervalEXT glXSwapIntervalEXT = nullptr;
  PFN_glXQueryExtension glXQueryExtension = nullptr;
  PFN_glXQueryVersion glXQueryVersion = nullptr;
  PFN_glXQueryExtensionsString glXQueryExtensionsString = nullptr;
  PFN_glXGetCurrentContext glXGetCurrentContext = nullptr;
  PFN_glXCreatePbuffer glXCreatePbuffer = nullptr;
  PFN_glXDestroyPbuffer glXDestroyPbuffer = nullptr;

  glx_lib_t() : host(__pick_lib())
  {
    glXGetProcAddress = host.sym_as<PFN_glXGetProcAddress>("glXGetProcAddress");
    if ( !glXGetProcAddress ) {
      glXGetProcAddress = host.sym_as<PFN_glXGetProcAddress>("glXGetProcAddressARB");
    }
    glXChooseFBConfig = host.sym_as<PFN_glXChooseFBConfig>("glXChooseFBConfig");
    glXGetVisualFromFBConfig = host.sym_as<PFN_glXGetVisualFromFBConfig>("glXGetVisualFromFBConfig");
    glXGetFBConfigAttrib = host.sym_as<PFN_glXGetFBConfigAttrib>("glXGetFBConfigAttrib");
    glXCreateNewContext = host.sym_as<PFN_glXCreateNewContext>("glXCreateNewContext");
    glXDestroyContext = host.sym_as<PFN_glXDestroyContext>("glXDestroyContext");
    glXMakeCurrent = host.sym_as<PFN_glXMakeCurrent>("glXMakeCurrent");
    glXMakeContextCurrent = host.sym_as<PFN_glXMakeContextCurrent>("glXMakeContextCurrent");
    glXSwapBuffers = host.sym_as<PFN_glXSwapBuffers>("glXSwapBuffers");
    glXQueryExtension = host.sym_as<PFN_glXQueryExtension>("glXQueryExtension");
    glXQueryVersion = host.sym_as<PFN_glXQueryVersion>("glXQueryVersion");
    glXQueryExtensionsString = host.sym_as<PFN_glXQueryExtensionsString>("glXQueryExtensionsString");
    glXGetCurrentContext = host.sym_as<PFN_glXGetCurrentContext>("glXGetCurrentContext");
    glXCreatePbuffer = host.sym_as<PFN_glXCreatePbuffer>("glXCreatePbuffer");
    glXDestroyPbuffer = host.sym_as<PFN_glXDestroyPbuffer>("glXDestroyPbuffer");

    if ( !glXGetProcAddress || !glXChooseFBConfig || !glXGetVisualFromFBConfig || !glXMakeCurrent || !glXSwapBuffers ) {
      throw except::library_error("glx: required entry points missing in libGL.so.1 / libGLX.so.0");
    }

    glXCreateContextAttribsARB = reinterpret_cast<PFN_glXCreateContextAttribsARB>(
        glXGetProcAddress(reinterpret_cast<const GLubyte *>("glXCreateContextAttribsARB")));
    glXSwapIntervalEXT
        = reinterpret_cast<PFN_glXSwapIntervalEXT>(glXGetProcAddress(reinterpret_cast<const GLubyte *>("glXSwapIntervalEXT")));
  }

  bool
  has_extension(x11_display_t &dpy, const char *name) noexcept
  {
    if ( !glXQueryExtensionsString || !name ) return false;
    const char *exts = glXQueryExtensionsString(dpy.display(), dpy.screen());
    if ( !exts ) return false;
    const usize nlen = micron::strlen(name);
    if ( nlen == 0 ) return false;
    const char *p = exts;
    while ( (p = micron::strstr(p, name)) != nullptr ) {
      const bool left_ok = (p == exts) || (p[-1] == ' ');
      const bool right_ok = (p[nlen] == ' ' || p[nlen] == '\0');
      if ( left_ok && right_ok ) return true;
      p += nlen;
    }
    return false;
  }

private:
  static const char *
  __pick_lib() noexcept
  {
    return "libGL.so.1";
  }
};

struct glx_fbconfig_request_t {
  int red_size = 8;
  int green_size = 8;
  int blue_size = 8;
  int alpha_size = 8;
  int depth_size = 24;
  int stencil_size = 8;
  int samples = 0;      // 0 = no MSAA
  bool double_buffer = true;
};

struct glx_context_request_t {
  int major = 4;
  int minor = 6;
  bool core_profile = true;
  bool forward_compat = true;
  bool debug = false;
  GLXContext share = nullptr;      // non-null = share resources with this context
};

class glx_context_t
{
private:
  GLXFBConfig __fbconfig = nullptr;
  XVisualInfo *__visual = nullptr;
  GLXContext __ctx = nullptr;
  glx_lib_t *__lib = nullptr;
  x11_display_t *__disp = nullptr;

public:
  static constexpr backend_tag_t tag = backend_tag_t::x11;

  ~glx_context_t() { destroy(); }

  glx_context_t() = default;

  glx_context_t(const glx_context_t &) = delete;
  glx_context_t(glx_context_t &&) = delete;

  glx_context_t &operator=(const glx_context_t &) = delete;
  glx_context_t &operator=(glx_context_t &&) = delete;

  GLXFBConfig
  fbconfig() const noexcept
  {
    return __fbconfig;
  }

  XVisualInfo *
  visual() const noexcept
  {
    return __visual;
  }

  GLXContext
  ctx() const noexcept
  {
    return __ctx;
  }

  glx_lib_t *
  lib() const noexcept
  {
    return __lib;
  }

  x11_display_t *
  disp() const noexcept
  {
    return __disp;
  }

  void
  bind(glx_lib_t &g, x11_display_t &dpy, GLXFBConfig cfg) noexcept
  {
    __lib = &g;
    __disp = &dpy;
    __fbconfig = cfg;
  }

  void
  set_visual(XVisualInfo *v) noexcept
  {
    __visual = v;
  }

  XVisualInfo *
  take_visual() noexcept
  {
    XVisualInfo *v = __visual;
    __visual = nullptr;
    return v;
  }

  void
  pick_visual(glx_lib_t &g, x11_display_t &dpy, const glx_fbconfig_request_t &r)
  {
    __lib = &g;
    __disp = &dpy;
    const int attribs[] = {
      GLX_X_RENDERABLE,
      1,
      GLX_DRAWABLE_TYPE,
      GLX_WINDOW_BIT,
      GLX_RENDER_TYPE,
      GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE,
      GLX_TRUE_COLOR,
      GLX_RED_SIZE,
      r.red_size,
      GLX_GREEN_SIZE,
      r.green_size,
      GLX_BLUE_SIZE,
      r.blue_size,
      GLX_ALPHA_SIZE,
      r.alpha_size,
      GLX_DEPTH_SIZE,
      r.depth_size,
      GLX_STENCIL_SIZE,
      r.stencil_size,
      GLX_DOUBLEBUFFER,
      r.double_buffer ? 1 : 0,
      GLX_SAMPLE_BUFFERS,
      r.samples > 0 ? 1 : 0,
      GLX_SAMPLES,
      r.samples,
      0,
    };
    int n = 0;
    GLXFBConfig *cfgs = g.glXChooseFBConfig(dpy.display(), dpy.screen(), attribs, &n);
    if ( !cfgs || n == 0 ) {
      if ( cfgs ) dpy.lib()->XFree(cfgs);
      throw except::library_error("glx: no matching FBConfig");
    }
    __fbconfig = cfgs[0];
    dpy.lib()->XFree(cfgs);

    __visual = g.glXGetVisualFromFBConfig(dpy.display(), __fbconfig);
    if ( !__visual ) throw except::library_error("glx: glXGetVisualFromFBConfig returned null");
  }

  void
  pick_pbuffer_fbconfig(glx_lib_t &g, x11_display_t &dpy, const glx_fbconfig_request_t &r)
  {
    __lib = &g;
    __disp = &dpy;
    const int attribs[] = {
      GLX_X_RENDERABLE,
      1,
      GLX_DRAWABLE_TYPE,
      GLX_PBUFFER_BIT,
      GLX_RENDER_TYPE,
      GLX_RGBA_BIT,
      GLX_RED_SIZE,
      r.red_size,
      GLX_GREEN_SIZE,
      r.green_size,
      GLX_BLUE_SIZE,
      r.blue_size,
      GLX_ALPHA_SIZE,
      r.alpha_size,
      GLX_DEPTH_SIZE,
      r.depth_size,
      GLX_STENCIL_SIZE,
      r.stencil_size,
      GLX_DOUBLEBUFFER,
      0,
      GLX_SAMPLE_BUFFERS,
      r.samples > 0 ? 1 : 0,
      GLX_SAMPLES,
      r.samples,
      0,
    };
    int n = 0;
    GLXFBConfig *cfgs = g.glXChooseFBConfig(dpy.display(), dpy.screen(), attribs, &n);
    if ( !cfgs || n == 0 ) {
      if ( cfgs ) dpy.lib()->XFree(cfgs);
      throw except::library_error("glx: no pbuffer-capable FBConfig");
    }
    __fbconfig = cfgs[0];
    dpy.lib()->XFree(cfgs);
  }

  void
  create(const glx_context_request_t &r)
  {
    if ( !__lib || !__disp || !__fbconfig ) throw except::logic_error("glx_context: pick_visual not called");

    if ( __lib->glXCreateContextAttribsARB ) {
      int flags = 0;
      if ( r.debug ) flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
      if ( r.forward_compat ) flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
      const int profile = r.core_profile ? GLX_CONTEXT_CORE_PROFILE_BIT_ARB : GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
      const int attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB,
        r.major,
        GLX_CONTEXT_MINOR_VERSION_ARB,
        r.minor,
        GLX_CONTEXT_PROFILE_MASK_ARB,
        profile,
        GLX_CONTEXT_FLAGS_ARB,
        flags,
        0,
      };
      __ctx = __lib->glXCreateContextAttribsARB(__disp->display(), __fbconfig, r.share, 1, attribs);
    }
    if ( !__ctx && __lib->glXCreateNewContext ) {
      __ctx = __lib->glXCreateNewContext(__disp->display(), __fbconfig, /*GLX_RGBA_TYPE*/ 0x8014, r.share, 1);
    }
    if ( !__ctx ) throw except::library_error("glx: context creation failed");
  }

  bool
  make_current(GLXDrawable drawable) noexcept
  {
    return __lib && __lib->glXMakeCurrent(__disp->display(), drawable, __ctx) != 0;
  }

  bool
  make_context_current(GLXDrawable draw, GLXDrawable read) noexcept
  {
    if ( !__lib || !__lib->glXMakeContextCurrent ) return false;
    return __lib->glXMakeContextCurrent(__disp->display(), draw, read, __ctx) != 0;
  }

  void
  release() noexcept
  {
    if ( __lib && __disp ) __lib->glXMakeCurrent(__disp->display(), 0, nullptr);
  }

  void
  swap(GLXDrawable drawable) noexcept
  {
    if ( __lib && __disp ) __lib->glXSwapBuffers(__disp->display(), drawable);
  }

  void
  set_swap_interval(GLXDrawable drawable, int interval) noexcept
  {
    if ( __lib && __lib->glXSwapIntervalEXT ) __lib->glXSwapIntervalEXT(__disp->display(), drawable, interval);
  }

  void *
  proc_address(const char *name) noexcept
  {
    if ( !__lib || !__lib->glXGetProcAddress ) return nullptr;
    return reinterpret_cast<void *>(__lib->glXGetProcAddress(reinterpret_cast<const GLubyte *>(name)));
  }

  GLXPbuffer
  create_pbuffer(i32 width, i32 height) noexcept
  {
    if ( !__lib || !__lib->glXCreatePbuffer || !__disp || !__fbconfig ) return 0;
    const int attribs[] = {
      GLX_PBUFFER_WIDTH, static_cast<int>(width), GLX_PBUFFER_HEIGHT, static_cast<int>(height), GLX_LARGEST_PBUFFER, 0, 0,
    };
    return __lib->glXCreatePbuffer(__disp->display(), __fbconfig, attribs);
  }

  void
  destroy_pbuffer(GLXPbuffer pb) noexcept
  {
    if ( __lib && __lib->glXDestroyPbuffer && __disp && pb ) __lib->glXDestroyPbuffer(__disp->display(), pb);
  }

  void
  destroy() noexcept
  {
    if ( !__lib || !__disp ) return;
    if ( __ctx ) __lib->glXDestroyContext(__disp->display(), __ctx);
    if ( __visual ) __disp->lib()->XFree(__visual);
    __ctx = nullptr;
    __visual = nullptr;
  }
};

};      // namespace gl
};      // namespace gfx
};      // namespace micron
