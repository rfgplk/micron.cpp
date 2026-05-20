//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "debug.hpp"
#include "errors.hpp"
#include "hints.hpp"
#include "loader.hpp"
#include "physical_device.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline constexpr u32 max_physical_devices = 32;
inline constexpr u32 max_enabled_layers = 32;
inline constexpr u32 max_enabled_extensions = 32;

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// vk::instance/VkInstance
class instance
{
private:
  VkInstance __h = nullptr;
  messenger __dbg;
  bool __debug_enabled = false;

  static u32
  __append(const char **buf, u32 i, u32 cap, const char *name) noexcept
  {
    if ( !name || i >= cap ) return i;

    for ( u32 k = 0; k < i; ++k ) {
      if ( buf[k] == name ) return i;
    }
    buf[i] = name;
    return i + 1;
  }

  static u32
  __build_layers(const instance_hints &h, const char **buf, u32 cap) noexcept
  {
    u32 n = 0;
    if ( h.debug ) n = __append(buf, n, cap, VK_LAYER_KHRONOS_VALIDATION_NAME);
    for ( usize i = 0; i < h.extra_layers_count && n < cap; ++i ) n = __append(buf, n, cap, h.extra_layers[i]);
    return n;
  }

  static u32
  __build_extensions(const instance_hints &h, const char **buf, u32 cap) noexcept
  {
    u32 n = 0;
    if ( h.surface ) {
      n = __append(buf, n, cap, VK_KHR_SURFACE_EXTENSION_NAME);

      if ( has_instance_extension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME) ) n = __append(buf, n, cap, VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
      if ( has_instance_extension(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) ) n = __append(buf, n, cap, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }
    if ( h.debug ) n = __append(buf, n, cap, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    for ( usize i = 0; i < h.extra_extensions_count && n < cap; ++i ) n = __append(buf, n, cap, h.extra_extensions[i]);
    return n;
  }

public:
  ~instance() { reset(); }

  instance() = default;

  explicit instance(const instance_hints &h)
  {

    (void)vk_lib();
    __load_global();
    if ( !vkCreateInstance ) throw except::library_error("vk::instance: vkCreateInstance unresolved — wrong libvulkan?");

    const char *layers[max_enabled_layers];
    const char *exts[max_enabled_extensions];
    const u32 num_layers = __build_layers(h, layers, max_enabled_layers);
    const u32 num_exts = __build_extensions(h, exts, max_enabled_extensions);

    VkApplicationInfo app{};
    app.sType = structure_type_of_v<VkApplicationInfo>;
    app.pApplicationName = h.app_name;
    app.applicationVersion = h.app_version;
    app.pEngineName = h.engine_name;
    app.engineVersion = h.engine_version;
    app.apiVersion = h.api_version;

    VkDebugUtilsMessengerCreateInfoEXT debug_ci = default_messenger_info();

    VkInstanceCreateInfo ci{};
    ci.sType = structure_type_of_v<VkInstanceCreateInfo>;
    ci.pApplicationInfo = &app;
    ci.enabledLayerCount = num_layers;
    ci.ppEnabledLayerNames = num_layers ? layers : nullptr;
    ci.enabledExtensionCount = num_exts;
    ci.ppEnabledExtensionNames = num_exts ? exts : nullptr;
    if ( h.debug ) ci.pNext = &debug_ci;

    check_vk(vkCreateInstance(&ci, host_allocation_callbacks(), &__h), "vkCreateInstance");

    __load_instance_1_3(__h);

    __debug_enabled = h.debug;
    if ( h.debug && vkCreateDebugUtilsMessengerEXT ) {

      __dbg = messenger(__h);
    }
  }

  instance(const instance &) = delete;

  instance(instance &&o) noexcept : __h(o.__h), __dbg(static_cast<messenger &&>(o.__dbg)), __debug_enabled(o.__debug_enabled)
  {
    o.__h = nullptr;
    o.__debug_enabled = false;
  }

  instance &operator=(const instance &) = delete;

  instance &
  operator=(instance &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      __dbg = static_cast<messenger &&>(o.__dbg);
      __debug_enabled = o.__debug_enabled;
      o.__h = nullptr;
      o.__debug_enabled = false;
    }
    return *this;
  }

  VkInstance
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
  debug_enabled() const noexcept
  {
    return __debug_enabled;
  }

  const messenger &
  default_messenger() const noexcept
  {
    return __dbg;
  }

  void
  reset() noexcept
  {

    __dbg.reset();
    if ( __h && vkDestroyInstance ) vkDestroyInstance(__h, host_allocation_callbacks());
    __h = nullptr;
    __debug_enabled = false;
  }

  struct physical_device_list {
    physical_device devices[max_physical_devices]{};
    u32 count = 0;

    physical_device *
    begin() noexcept
    {
      return devices;
    }

    physical_device *
    end() noexcept
    {
      return devices + count;
    }

    const physical_device *
    begin() const noexcept
    {
      return devices;
    }

    const physical_device *
    end() const noexcept
    {
      return devices + count;
    }

    physical_device &
    operator[](u32 i) noexcept
    {
      return devices[i];
    }

    const physical_device &
    operator[](u32 i) const noexcept
    {
      return devices[i];
    }
  };

  physical_device_list
  enumerate_physical_devices() const
  {
    if ( !__h || !vkEnumeratePhysicalDevices )
      throw except::library_error("vk::instance::enumerate_physical_devices: vkEnumeratePhysicalDevices not loaded");
    u32 n = 0;
    check_vk(vkEnumeratePhysicalDevices(__h, &n, nullptr), "vkEnumeratePhysicalDevices");
    if ( n > max_physical_devices ) n = max_physical_devices;
    VkPhysicalDevice raw[max_physical_devices]{};
    check_vk(vkEnumeratePhysicalDevices(__h, &n, raw), "vkEnumeratePhysicalDevices");
    physical_device_list out;
    out.count = n;
    for ( u32 i = 0; i < n; ++i ) out.devices[i] = physical_device(raw[i]);
    return out;
  }

  physical_device
  pick_physical_device(bool prefer_discrete = true) const
  {
    auto list = enumerate_physical_devices();
    if ( list.count == 0 ) return {};
    if ( prefer_discrete ) {
      for ( u32 i = 0; i < list.count; ++i ) {
        if ( list.devices[i].is_discrete() ) return list.devices[i];
      }
    }
    return list.devices[0];
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
