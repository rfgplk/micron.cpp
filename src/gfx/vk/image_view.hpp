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
#include "image.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class image_view
{
  VkDevice __dev = nullptr;
  VkImageView __h = nullptr;

public:
  ~image_view() { reset(); }

  image_view() = default;

  image_view(device &dev, const VkImageViewCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateImageView ) throw except::library_error("vk::image_view: vkCreateImageView unresolved");
    check_vk(vkCreateImageView(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateImageView");
  }

  image_view(device &dev, const image &img, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
             VkImageViewType view_type = static_cast<VkImageViewType>(VK_IMAGE_VIEW_TYPE_2D))
      : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateImageView ) throw except::library_error("vk::image_view: vkCreateImageView unresolved");
    VkImageViewCreateInfo ci{};
    ci.sType = structure_type_of_v<VkImageViewCreateInfo>;
    ci.image = img.handle();
    ci.viewType = view_type;
    ci.format = img.format();
    ci.components.r = static_cast<VkComponentSwizzle>(VK_COMPONENT_SWIZZLE_IDENTITY);
    ci.components.g = static_cast<VkComponentSwizzle>(VK_COMPONENT_SWIZZLE_IDENTITY);
    ci.components.b = static_cast<VkComponentSwizzle>(VK_COMPONENT_SWIZZLE_IDENTITY);
    ci.components.a = static_cast<VkComponentSwizzle>(VK_COMPONENT_SWIZZLE_IDENTITY);
    ci.subresourceRange.aspectMask = aspect_mask;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = ~0u;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = ~0u;
    check_vk(vkCreateImageView(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateImageView");
  }

  image_view(const image_view &) = delete;

  image_view(image_view &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  image_view &operator=(const image_view &) = delete;

  image_view &
  operator=(image_view &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      o.__dev = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkImageView
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

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyImageView ) vkDestroyImageView(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
