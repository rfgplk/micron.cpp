//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../linux/dynamic.hpp"
#include "../../memory/cstring.hpp"
#include "../../types.hpp"

#include "vulkan.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// main vulkan fn loading
//
//   0 (global)   —> resolved via vkGetInstanceProcAddr(VK_NULL_HANDLE, name)
//
//   1 (instance) -> resolved via vkGetInstanceProcAddr(instance, name)
//
//   2 (device)   —> resolved via vkGetDeviceProcAddr(device, name)

namespace micron
{
namespace gfx
{
namespace vk
{

// own libvulkan.so.1; cached
// NOTE: libvulkan must be linked against this at comptime, otherwise errors out
struct vk_lib_t {
  host_dso host;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

  vk_lib_t() : host("libvulkan.so.1")
  {
    vkGetInstanceProcAddr = host.sym_as<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    if ( !vkGetInstanceProcAddr ) {
      throw except::library_error("vulkan: vkGetInstanceProcAddr missing in libvulkan.so.1");
    }
  }
};

inline vk_lib_t &
vk_lib() noexcept(false)
{
  static vk_lib_t lib;
  return lib;
}

inline VkInstance __current_instance_for_loader = nullptr;

// 0 global
inline void
__load_global() noexcept
{
  auto get = vk_lib().vkGetInstanceProcAddr;
  for ( usize i = 0; i < vk_table_size; ++i ) {
    if ( vk_table[i].dispatch_kind != 0 ) continue;
    *__vk_fnptr_slots[i] = reinterpret_cast<void *>(get(VK_NULL_HANDLE, vk_table[i].name));
  }
}

// 1 instance
inline void
__load_instance_1_3(VkInstance instance) noexcept
{
  __current_instance_for_loader = instance;
  auto get = vk_lib().vkGetInstanceProcAddr;
  for ( usize i = 0; i < vk_table_size; ++i ) {
    if ( vk_table[i].dispatch_kind != 1 ) continue;
    *__vk_fnptr_slots[i] = reinterpret_cast<void *>(get(instance, vk_table[i].name));
  }
  if ( !vkGetDeviceProcAddr ) {
    vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(get(instance, "vkGetDeviceProcAddr"));
  }
}

// 2 device
inline void
__load_device_1_3(VkDevice device) noexcept
{
  if ( !vkGetDeviceProcAddr ) return;      // should be populated by __load_instance_1_3
  for ( usize i = 0; i < vk_table_size; ++i ) {
    if ( vk_table[i].dispatch_kind != 2 ) continue;
    *__vk_fnptr_slots[i] = reinterpret_cast<void *>(vkGetDeviceProcAddr(device, vk_table[i].name));
  }
}

// detect at rt whether the loader supports a named instance extension
inline bool
has_instance_extension(const char *name) noexcept
{
  if ( !name || !vkEnumerateInstanceExtensionProperties ) return false;
  u32 n = 0;
  if ( vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr) != VK_SUCCESS ) return false;
  if ( n == 0 ) return false;
  constexpr u32 cap = 256;
  if ( n > cap ) n = cap;
  VkExtensionProperties props[cap]{};
  if ( vkEnumerateInstanceExtensionProperties(nullptr, &n, props) != VK_SUCCESS && /* tolerate */ true ) { /* fallthrough */
  }
  for ( u32 i = 0; i < n; ++i ) {
    if ( micron::strcmp(props[i].extensionName, name) == 0 ) return true;
  }
  return false;
}

inline bool
has_device_extension(VkPhysicalDevice pd, const char *name) noexcept
{
  if ( !pd || !name || !vkEnumerateDeviceExtensionProperties ) return false;
  u32 n = 0;
  if ( vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, nullptr) != VK_SUCCESS ) return false;
  if ( n == 0 ) return false;
  constexpr u32 cap = 512;
  if ( n > cap ) n = cap;
  VkExtensionProperties props[cap]{};
  if ( vkEnumerateDeviceExtensionProperties(pd, nullptr, &n, props) != VK_SUCCESS ) return false;
  for ( u32 i = 0; i < n; ++i ) {
    if ( micron::strcmp(props[i].extensionName, name) == 0 ) return true;
  }
  return false;
}

template<u16... Indices> class subset
{
  void *__fns[sizeof...(Indices)] = {};

public:
  static constexpr usize count = sizeof...(Indices);
  static constexpr u16 indices[count] = { Indices... };

  void
  load_global() noexcept
  {
    auto get = vk_lib().vkGetInstanceProcAddr;
    usize k = 0;
    ((__fns[k++] = reinterpret_cast<void *>(get(VK_NULL_HANDLE, vk_table[Indices].name))), ...);
  }

  void
  load_instance(VkInstance instance) noexcept
  {
    auto get = vk_lib().vkGetInstanceProcAddr;
    usize k = 0;
    ((__fns[k++] = reinterpret_cast<void *>(get(instance, vk_table[Indices].name))), ...);
  }

  void
  load_device(VkDevice device) noexcept
  {
    if ( !vkGetDeviceProcAddr ) return;
    usize k = 0;
    ((__fns[k++] = reinterpret_cast<void *>(vkGetDeviceProcAddr(device, vk_table[Indices].name))), ...);
  }

  template<u16 Idx>
  static consteval usize
  position_of() noexcept
  {
    static_assert(((Idx == Indices) || ...), "subset does not contain that index");
    usize pos = 0;
    bool found = false;
    auto check = [&](u16 idx) {
      if ( !found ) {
        if ( idx == Idx )
          found = true;
        else
          ++pos;
      }
    };
    ((check(Indices)), ...);
    return pos;
  }

  template<u16 Idx>
  void *
  raw() const noexcept
  {
    return __fns[position_of<Idx>()];
  }

  template<u16 Idx, class Fn>
  Fn
  as() const noexcept
  {
    return reinterpret_cast<Fn>(raw<Idx>());
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
