//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/cstring.hpp"
#include "../../types.hpp"

#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline constexpr u32 max_queue_families = 16;

class physical_device
{
  VkPhysicalDevice __h = nullptr;
  VkPhysicalDeviceProperties __props{};
  VkPhysicalDeviceFeatures __features{};
  VkPhysicalDeviceMemoryProperties __mem_props{};
  VkQueueFamilyProperties __qf_props[max_queue_families]{};
  u32 __qf_count = 0;

  void
  __refresh() noexcept
  {
    if ( !__h ) return;
    if ( vkGetPhysicalDeviceProperties ) vkGetPhysicalDeviceProperties(__h, &__props);
    if ( vkGetPhysicalDeviceFeatures ) vkGetPhysicalDeviceFeatures(__h, &__features);
    if ( vkGetPhysicalDeviceMemoryProperties ) vkGetPhysicalDeviceMemoryProperties(__h, &__mem_props);
    if ( vkGetPhysicalDeviceQueueFamilyProperties ) {
      u32 n = max_queue_families;
      vkGetPhysicalDeviceQueueFamilyProperties(__h, &n, __qf_props);
      __qf_count = n > max_queue_families ? max_queue_families : n;
    }
  }

public:
  physical_device() = default;

  explicit physical_device(VkPhysicalDevice h) noexcept : __h(h) { __refresh(); }

  VkPhysicalDevice
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

  const VkPhysicalDeviceProperties &
  properties() const noexcept
  {
    return __props;
  }

  const VkPhysicalDeviceFeatures &
  features() const noexcept
  {
    return __features;
  }

  const VkPhysicalDeviceMemoryProperties &
  memory_properties() const noexcept
  {
    return __mem_props;
  }

  const VkQueueFamilyProperties *
  queue_family_properties() const noexcept
  {
    return __qf_props;
  }

  u32
  queue_family_count() const noexcept
  {
    return __qf_count;
  }

  u32
  select_queue_family(VkQueueFlags required, VkSurfaceKHR surf = VK_NULL_HANDLE) const noexcept
  {
    for ( u32 i = 0; i < __qf_count; ++i ) {
      if ( (__qf_props[i].queueFlags & required) != required ) continue;
      if ( surf == VK_NULL_HANDLE ) return i;
      if ( !vkGetPhysicalDeviceSurfaceSupportKHR ) continue;
      VkBool32 ok = 0;
      if ( vkGetPhysicalDeviceSurfaceSupportKHR(__h, i, surf, &ok) != VK_SUCCESS ) continue;
      if ( ok ) return i;
    }
    return ~0u;
  }

  bool
  supports_extension(const char *name) const noexcept
  {
    if ( !__h || !name || !vkEnumerateDeviceExtensionProperties ) return false;
    u32 n = 0;
    if ( vkEnumerateDeviceExtensionProperties(__h, nullptr, &n, nullptr) != VK_SUCCESS ) return false;
    if ( n == 0 ) return false;
    constexpr u32 cap = 512;
    if ( n > cap ) n = cap;
    VkExtensionProperties props[cap]{};
    if ( vkEnumerateDeviceExtensionProperties(__h, nullptr, &n, props) != VK_SUCCESS ) return false;
    for ( u32 i = 0; i < n; ++i ) {
      if ( micron::strcmp(props[i].extensionName, name) == 0 ) return true;
    }
    return false;
  }

  bool
  is_discrete() const noexcept
  {
    return __props.deviceType == static_cast<VkPhysicalDeviceType>(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
