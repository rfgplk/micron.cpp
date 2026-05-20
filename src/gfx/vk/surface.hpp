//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "../platform/window.hpp"

#include "allocator.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class surface
{
private:
  VkInstance __inst = nullptr;
  VkSurfaceKHR __h = nullptr;

  void
  __build_x11(platform::window &w)
  {
    auto *xd = w.dpy().as_x11();
    auto *xw = w.as_x11();
    if ( !xd || !xw ) throw except::logic_error("vk::surface: x11 display/window missing");
    if ( !vkCreateXlibSurfaceKHR ) throw except::library_error("vk::surface: VK_KHR_xlib_surface not loaded — enable it on the instance");
    VkXlibSurfaceCreateInfoKHR ci{};
    ci.sType = structure_type_of_v<VkXlibSurfaceCreateInfoKHR>;
    ci.dpy = xd->display();
    ci.window = xw->window();
    check_vk(vkCreateXlibSurfaceKHR(__inst, &ci, host_allocation_callbacks(), &__h), "vkCreateXlibSurfaceKHR");
  }

  void
  __build_wayland(platform::window &w)
  {
    auto *wd = w.dpy().as_wayland();
    auto *ww = w.as_wayland();
    if ( !wd || !ww ) throw except::logic_error("vk::surface: wayland display/window missing");
    if ( !vkCreateWaylandSurfaceKHR )
      throw except::library_error("vk::surface: VK_KHR_wayland_surface not loaded — enable it on the instance");
    VkWaylandSurfaceCreateInfoKHR ci{};
    ci.sType = structure_type_of_v<VkWaylandSurfaceCreateInfoKHR>;
    ci.display = wd->display();
    ci.surface = ww->surface();
    check_vk(vkCreateWaylandSurfaceKHR(__inst, &ci, host_allocation_callbacks(), &__h), "vkCreateWaylandSurfaceKHR");
  }

public:
  ~surface() { reset(); }

  surface() = default;

  surface(VkInstance inst, platform::window &w) : __inst(inst)
  {
    if ( !inst ) throw except::logic_error("vk::surface: null instance");
    switch ( w.dpy().backend() ) {
    case platform::backend_tag_t::x11:
      __build_x11(w);
      break;
    case platform::backend_tag_t::wayland:
      __build_wayland(w);
      break;
    case platform::backend_tag_t::none:
    default:
      throw except::logic_error("vk::surface: window's display has no backend");
    }
  }

  surface(const surface &) = delete;

  surface(surface &&o) noexcept : __inst(o.__inst), __h(o.__h)
  {
    o.__inst = nullptr;
    o.__h = nullptr;
  }

  surface &operator=(const surface &) = delete;

  surface &
  operator=(surface &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __inst = o.__inst;
      __h = o.__h;
      o.__inst = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkSurfaceKHR
  handle() const noexcept
  {
    return __h;
  }

  VkInstance
  instance() const noexcept
  {
    return __inst;
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

  void
  reset() noexcept
  {
    if ( __h && __inst && vkDestroySurfaceKHR ) vkDestroySurfaceKHR(__inst, __h, host_allocation_callbacks());
    __h = nullptr;
    __inst = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
