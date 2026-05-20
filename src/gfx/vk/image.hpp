//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "device.hpp"
#include "errors.hpp"
#include "memory.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

struct image_2d_request {
  u32 width = 1;
  u32 height = 1;
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  u32 mip_levels = 1;
  u32 array_layers = 1;
  VkSampleCountFlagBits samples = static_cast<VkSampleCountFlagBits>(VK_SAMPLE_COUNT_1_BIT);
  VkImageTiling tiling = static_cast<VkImageTiling>(VK_IMAGE_TILING_OPTIMAL);
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// image/vkImage

class image
{
  VkDevice __dev = nullptr;
  VkImage __h = nullptr;
  VkFormat __format = VK_FORMAT_UNDEFINED;
  VkExtent3D __extent{};
  bool __owned = true;

public:
  ~image() { reset(); }

  image() = default;

  image(device &dev, const VkImageCreateInfo &ci) : __dev(dev.handle()), __format(ci.format), __extent(ci.extent)
  {
    if ( !__dev || !vkCreateImage ) throw except::library_error("vk::image: vkCreateImage unresolved");
    check_vk(vkCreateImage(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateImage");
  }

  image(device &dev, const image_2d_request &r) : __dev(dev.handle()), __format(r.format)
  {
    if ( !__dev || !vkCreateImage ) throw except::library_error("vk::image: vkCreateImage unresolved");
    __extent.width = r.width;
    __extent.height = r.height;
    __extent.depth = 1;
    VkImageCreateInfo ci{};
    ci.sType = structure_type_of_v<VkImageCreateInfo>;
    ci.imageType = static_cast<VkImageType>(VK_IMAGE_TYPE_2D);
    ci.format = r.format;
    ci.extent = __extent;
    ci.mipLevels = r.mip_levels;
    ci.arrayLayers = r.array_layers;
    ci.samples = r.samples;
    ci.tiling = r.tiling;
    ci.usage = r.usage;
    ci.sharingMode = static_cast<VkSharingMode>(VK_SHARING_MODE_EXCLUSIVE);
    ci.initialLayout = static_cast<VkImageLayout>(VK_IMAGE_LAYOUT_UNDEFINED);
    check_vk(vkCreateImage(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateImage");
  }

  static image
  wrap_external(VkImage h, VkFormat fmt, VkExtent3D extent) noexcept
  {
    image i;
    i.__h = h;
    i.__format = fmt;
    i.__extent = extent;
    i.__owned = false;
    return i;
  }

  image(const image &) = delete;

  image(image &&o) noexcept : __dev(o.__dev), __h(o.__h), __format(o.__format), __extent(o.__extent), __owned(o.__owned)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__owned = true;
  }

  image &operator=(const image &) = delete;

  image &
  operator=(image &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __format = o.__format;
      __extent = o.__extent;
      __owned = o.__owned;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__owned = true;
    }
    return *this;
  }

  VkImage
  handle() const noexcept
  {
    return __h;
  }

  VkFormat
  format() const noexcept
  {
    return __format;
  }

  VkExtent3D
  extent() const noexcept
  {
    return __extent;
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

  VkMemoryRequirements
  memory_requirements() const noexcept
  {
    VkMemoryRequirements r{};
    if ( __h && __dev && vkGetImageMemoryRequirements ) vkGetImageMemoryRequirements(__dev, __h, &r);
    return r;
  }

  void
  bind(memory_allocation &mem, VkDeviceSize offset = 0)
  {
    if ( !__h || !__dev || !vkBindImageMemory ) throw except::library_error("vk::image::bind: vkBindImageMemory unresolved");
    check_vk(vkBindImageMemory(__dev, __h, mem.handle(), offset), "vkBindImageMemory");
  }

  void
  reset() noexcept
  {
    if ( __owned && __h && __dev && vkDestroyImage ) vkDestroyImage(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __format = VK_FORMAT_UNDEFINED;
    __extent = {};
    __owned = true;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
