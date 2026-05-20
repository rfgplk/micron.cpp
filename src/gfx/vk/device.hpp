//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "errors.hpp"
#include "hints.hpp"
#include "loader.hpp"
#include "physical_device.hpp"
#include "queue.hpp"
#include "surface.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline constexpr u32 max_unique_queue_families = 4;
inline constexpr u32 max_device_extensions = 32;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// vk::device
class device
{
private:
  VkDevice __h = nullptr;
  physical_device __pd;
  queue __graphics;
  queue __compute;
  queue __transfer;
  queue __present;
  u32 __graphics_family = ~0u;
  u32 __compute_family = ~0u;
  u32 __transfer_family = ~0u;
  u32 __present_family = ~0u;

  static u32
  __add_unique(u32 *out, u32 n, u32 family) noexcept
  {
    if ( family == ~0u ) return n;
    for ( u32 i = 0; i < n; ++i ) {
      if ( out[i] == family ) return n;
    }
    out[n] = family;
    return n + 1;
  }

public:
  ~device() { reset(); }

  device() = default;

  device(const physical_device &pd, const device_hints &h, const surface *surf = nullptr) : __pd(pd)
  {
    if ( !pd.valid() ) throw except::logic_error("vk::device: invalid physical_device");
    if ( !vkCreateDevice ) throw except::library_error("vk::device: vkCreateDevice unresolved — was vk::instance constructed first?");

    VkQueueFlags gfx_req{}, cmp_req{}, xfer_req{};
    if ( h.graphics ) gfx_req |= VK_QUEUE_GRAPHICS_BIT;
    if ( h.compute ) cmp_req |= VK_QUEUE_COMPUTE_BIT;
    if ( h.transfer ) xfer_req |= VK_QUEUE_TRANSFER_BIT;

    if ( h.graphics ) __graphics_family = pd.select_queue_family(gfx_req);
    if ( h.compute ) __compute_family = pd.select_queue_family(cmp_req);
    if ( h.transfer ) __transfer_family = pd.select_queue_family(xfer_req);
    if ( h.present && surf && surf->valid() ) __present_family = pd.select_queue_family(VkQueueFlags{}, surf->handle());

    u32 unique_families[max_unique_queue_families];
    u32 unique_n = 0;
    unique_n = __add_unique(unique_families, unique_n, __graphics_family);
    unique_n = __add_unique(unique_families, unique_n, __compute_family);
    unique_n = __add_unique(unique_families, unique_n, __transfer_family);
    unique_n = __add_unique(unique_families, unique_n, __present_family);
    if ( unique_n == 0 ) throw except::logic_error("vk::device: no requested capability could be satisfied");

    VkDeviceQueueCreateInfo queue_cis[max_unique_queue_families]{};
    const float priority = 1.0f;
    for ( u32 i = 0; i < unique_n; ++i ) {
      queue_cis[i].sType = structure_type_of_v<VkDeviceQueueCreateInfo>;
      queue_cis[i].queueFamilyIndex = unique_families[i];
      queue_cis[i].queueCount = 1;
      queue_cis[i].pQueuePriorities = &priority;
    }

    const char *exts[max_device_extensions];
    u32 num_exts = 0;
    if ( h.present && surf && surf->valid() ) {
      exts[num_exts++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }
    for ( usize i = 0; i < h.extra_extensions_count && num_exts < max_device_extensions; ++i ) {
      exts[num_exts++] = h.extra_extensions[i];
    }

    VkPhysicalDeviceVulkan13Features vk13 = h.required_vk13_features;
    vk13.sType = structure_type_of_v<VkPhysicalDeviceVulkan13Features>;
    vk13.pNext = nullptr;

    VkDeviceCreateInfo ci{};
    ci.sType = structure_type_of_v<VkDeviceCreateInfo>;
    ci.queueCreateInfoCount = unique_n;
    ci.pQueueCreateInfos = queue_cis;
    ci.enabledExtensionCount = num_exts;
    ci.ppEnabledExtensionNames = num_exts ? exts : nullptr;
    ci.pEnabledFeatures = &h.required_features;
    ci.pNext = &vk13;

    check_vk(vkCreateDevice(pd.handle(), &ci, nullptr, &__h), "vkCreateDevice");

    __load_device_1_3(__h);

    auto pull = [&](u32 family, queue &out) {
      if ( family == ~0u ) return;
      VkQueue q = nullptr;
      if ( vkGetDeviceQueue ) vkGetDeviceQueue(__h, family, 0, &q);
      out = queue(q, family, 0);
    };
    pull(__graphics_family, __graphics);
    pull(__compute_family, __compute);
    pull(__transfer_family, __transfer);
    pull(__present_family, __present);
  }

  device(const device &) = delete;

  device(device &&o) noexcept
      : __h(o.__h), __pd(o.__pd), __graphics(o.__graphics), __compute(o.__compute), __transfer(o.__transfer), __present(o.__present),
        __graphics_family(o.__graphics_family), __compute_family(o.__compute_family), __transfer_family(o.__transfer_family),
        __present_family(o.__present_family)
  {
    o.__h = nullptr;
  }

  device &operator=(const device &) = delete;

  device &
  operator=(device &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __h = o.__h;
      __pd = o.__pd;
      __graphics = o.__graphics;
      __compute = o.__compute;
      __transfer = o.__transfer;
      __present = o.__present;
      __graphics_family = o.__graphics_family;
      __compute_family = o.__compute_family;
      __transfer_family = o.__transfer_family;
      __present_family = o.__present_family;
      o.__h = nullptr;
    }
    return *this;
  }

  VkDevice
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

  const physical_device &
  physical() const noexcept
  {
    return __pd;
  }

  const queue &
  graphics() const noexcept
  {
    return __graphics;
  }

  const queue &
  compute() const noexcept
  {
    return __compute;
  }

  const queue &
  transfer() const noexcept
  {
    return __transfer;
  }

  const queue &
  present() const noexcept
  {
    return __present;
  }

  u32
  graphics_family() const noexcept
  {
    return __graphics_family;
  }

  u32
  compute_family() const noexcept
  {
    return __compute_family;
  }

  u32
  transfer_family() const noexcept
  {
    return __transfer_family;
  }

  u32
  present_family() const noexcept
  {
    return __present_family;
  }

  void
  wait_idle()
  {
    if ( !__h || !vkDeviceWaitIdle ) return;
    check_vk(vkDeviceWaitIdle(__h), "vkDeviceWaitIdle");
  }

  void *
  proc_address(const char *name) const noexcept
  {
    if ( !__h || !name || !vkGetDeviceProcAddr ) return nullptr;
    return reinterpret_cast<void *>(vkGetDeviceProcAddr(__h, name));
  }

  void
  reset() noexcept
  {
    if ( __h ) {

      if ( vkDeviceWaitIdle ) (void)vkDeviceWaitIdle(__h);
      if ( vkDestroyDevice ) vkDestroyDevice(__h, host_allocation_callbacks());
    }
    __h = nullptr;
    __graphics = queue{};
    __compute = queue{};
    __transfer = queue{};
    __present = queue{};
    __graphics_family = ~0u;
    __compute_family = ~0u;
    __transfer_family = ~0u;
    __present_family = ~0u;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
