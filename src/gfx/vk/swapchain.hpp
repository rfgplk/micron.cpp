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
#include "image_view.hpp"
#include "surface.hpp"
#include "sync.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline constexpr u32 max_swapchain_images = 8;

struct swapchain_request {
  VkExtent2D extent{};
  VkSurfaceFormatKHR format{ static_cast<VkFormat>(VK_FORMAT_B8G8R8A8_UNORM),
                             static_cast<VkColorSpaceKHR>(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) };
  VkPresentModeKHR present_mode = static_cast<VkPresentModeKHR>(VK_PRESENT_MODE_FIFO_KHR);
  VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  u32 min_image_count = 0;
};

class swapchain
{
private:
  device *__dev = nullptr;
  surface *__surf = nullptr;
  VkSwapchainKHR __h = nullptr;
  VkFormat __format = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR __color_space = static_cast<VkColorSpaceKHR>(0);
  VkExtent2D __extent{};
  VkPresentModeKHR __present_mode = static_cast<VkPresentModeKHR>(VK_PRESENT_MODE_FIFO_KHR);
  VkImageUsageFlags __image_usage = {};
  u32 __min_image_count = 0;
  image __images[max_swapchain_images];
  image_view __views[max_swapchain_images];
  u32 __image_count = 0;

  VkExtent2D
  __compose_create_info(VkSwapchainCreateInfoKHR &ci, const swapchain_request &r, VkSwapchainKHR old) const
  {
    if ( !__dev || !__surf || !vkGetPhysicalDeviceSurfaceCapabilitiesKHR )
      throw except::library_error("vk::swapchain: surface caps probe missing");

    VkSurfaceCapabilitiesKHR caps{};
    check_vk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(__dev->physical().handle(), __surf->handle(), &caps),
             "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    VkExtent2D ext = r.extent;
    if ( ext.width == 0 || ext.height == 0 ) {

      if ( caps.currentExtent.width != ~0u )
        ext = caps.currentExtent;
      else
        ext = caps.minImageExtent;
    }

    if ( ext.width < caps.minImageExtent.width ) ext.width = caps.minImageExtent.width;
    if ( ext.height < caps.minImageExtent.height ) ext.height = caps.minImageExtent.height;
    if ( caps.maxImageExtent.width && ext.width > caps.maxImageExtent.width ) ext.width = caps.maxImageExtent.width;
    if ( caps.maxImageExtent.height && ext.height > caps.maxImageExtent.height ) ext.height = caps.maxImageExtent.height;

    u32 min_count = r.min_image_count ? r.min_image_count : (caps.minImageCount + 1);
    if ( caps.maxImageCount > 0 && min_count > caps.maxImageCount ) min_count = caps.maxImageCount;
    if ( min_count < caps.minImageCount ) min_count = caps.minImageCount;

    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ( !static_cast<bool>(caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ) {

      const u32 mask = static_cast<u32>(caps.supportedCompositeAlpha);
      for ( u32 b = 1; b < 16; b <<= 1 ) {
        if ( mask & b ) {
          alpha = static_cast<VkCompositeAlphaFlagBitsKHR>(b);
          break;
        }
      }
    }

    ci = {};
    ci.sType = structure_type_of_v<VkSwapchainCreateInfoKHR>;
    ci.surface = __surf->handle();
    ci.minImageCount = min_count;
    ci.imageFormat = r.format.format;
    ci.imageColorSpace = r.format.colorSpace;
    ci.imageExtent = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage = r.image_usage;
    ci.imageSharingMode = static_cast<VkSharingMode>(VK_SHARING_MODE_EXCLUSIVE);
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = alpha;
    ci.presentMode = r.present_mode;
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = old;
    return ext;
  }

  void
  __retrieve_images()
  {
    if ( !__h || !vkGetSwapchainImagesKHR )
      throw except::library_error("vk::swapchain::__retrieve_images: vkGetSwapchainImagesKHR unresolved");
    u32 n = 0;
    check_vk(vkGetSwapchainImagesKHR(__dev->handle(), __h, &n, nullptr), "vkGetSwapchainImagesKHR");
    if ( n > max_swapchain_images ) n = max_swapchain_images;
    VkImage raw[max_swapchain_images]{};
    check_vk(vkGetSwapchainImagesKHR(__dev->handle(), __h, &n, raw), "vkGetSwapchainImagesKHR");

    VkExtent3D ext3{ __extent.width, __extent.height, 1 };
    for ( u32 i = 0; i < n; ++i ) {
      __images[i] = image::wrap_external(raw[i], __format, ext3);
      __views[i] = image_view(*__dev, __images[i], VK_IMAGE_ASPECT_COLOR_BIT, static_cast<VkImageViewType>(VK_IMAGE_VIEW_TYPE_2D));
    }
    __image_count = n;
  }

  void
  __build(const swapchain_request &r, VkSwapchainKHR old)
  {
    if ( !__dev || !__dev->valid() ) throw except::logic_error("vk::swapchain: invalid device");
    if ( !__surf || !__surf->valid() ) throw except::logic_error("vk::swapchain: invalid surface");
    if ( !vkCreateSwapchainKHR )
      throw except::library_error("vk::swapchain: vkCreateSwapchainKHR unresolved — enable VK_KHR_swapchain on the device");

    VkSwapchainCreateInfoKHR ci{};
    VkExtent2D final_ext = __compose_create_info(ci, r, old);

    check_vk(vkCreateSwapchainKHR(__dev->handle(), &ci, host_allocation_callbacks(), &__h), "vkCreateSwapchainKHR");

    __format = r.format.format;
    __color_space = r.format.colorSpace;
    __extent = final_ext;
    __present_mode = r.present_mode;
    __image_usage = r.image_usage;
    __min_image_count = ci.minImageCount;
    __retrieve_images();
  }

  void
  __destroy_views() noexcept
  {
    for ( u32 i = 0; i < __image_count; ++i ) __views[i].reset();
    for ( u32 i = 0; i < __image_count; ++i ) __images[i].reset();
    __image_count = 0;
  }

public:
  ~swapchain() { reset(); }

  swapchain() = default;

  swapchain(device &dev, surface &surf, const swapchain_request &r = {}) : __dev(&dev), __surf(&surf) { __build(r, VK_NULL_HANDLE); }

  swapchain(const swapchain &) = delete;

  swapchain(swapchain &&o) noexcept
      : __dev(o.__dev), __surf(o.__surf), __h(o.__h), __format(o.__format), __color_space(o.__color_space), __extent(o.__extent),
        __present_mode(o.__present_mode), __image_usage(o.__image_usage), __min_image_count(o.__min_image_count),
        __image_count(o.__image_count)
  {
    for ( u32 i = 0; i < o.__image_count; ++i ) {
      __images[i] = static_cast<image &&>(o.__images[i]);
      __views[i] = static_cast<image_view &&>(o.__views[i]);
    }
    o.__h = nullptr;
    o.__image_count = 0;
  }

  swapchain &operator=(const swapchain &) = delete;

  swapchain &
  operator=(swapchain &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __surf = o.__surf;
      __h = o.__h;
      __format = o.__format;
      __color_space = o.__color_space;
      __extent = o.__extent;
      __present_mode = o.__present_mode;
      __image_usage = o.__image_usage;
      __min_image_count = o.__min_image_count;
      __image_count = o.__image_count;
      for ( u32 i = 0; i < o.__image_count; ++i ) {
        __images[i] = static_cast<image &&>(o.__images[i]);
        __views[i] = static_cast<image_view &&>(o.__views[i]);
      }
      o.__h = nullptr;
      o.__image_count = 0;
    }
    return *this;
  }

  VkSwapchainKHR
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

  VkFormat
  format() const noexcept
  {
    return __format;
  }

  VkColorSpaceKHR
  color_space() const noexcept
  {
    return __color_space;
  }

  VkExtent2D
  extent() const noexcept
  {
    return __extent;
  }

  VkPresentModeKHR
  present_mode() const noexcept
  {
    return __present_mode;
  }

  u32
  image_count() const noexcept
  {
    return __image_count;
  }

  const image &
  image_at(u32 i) const noexcept
  {
    return __images[i];
  }

  const image_view &
  view_at(u32 i) const noexcept
  {
    return __views[i];
  }

  VkImage
  image_handle(u32 i) const noexcept
  {
    return __images[i].handle();
  }

  VkImageView
  view_handle(u32 i) const noexcept
  {
    return __views[i].handle();
  }

  VkResult
  acquire_next_image(semaphore *signal_sem, fence *signal_fence, u64 timeout_ns, u32 *out_index) noexcept
  {
    if ( !__h || !__dev || !vkAcquireNextImageKHR ) return VK_ERROR_INITIALIZATION_FAILED;
    VkSemaphore s = signal_sem ? signal_sem->handle() : VK_NULL_HANDLE;
    VkFence f = signal_fence ? signal_fence->handle() : VK_NULL_HANDLE;
    return vkAcquireNextImageKHR(__dev->handle(), __h, timeout_ns, s, f, out_index);
  }

  void
  recreate(VkExtent2D new_extent, const swapchain_request *overrides = nullptr)
  {
    swapchain_request r;
    if ( overrides ) r = *overrides;
    r.extent = new_extent;
    if ( !overrides ) {
      r.format.format = __format;
      r.format.colorSpace = __color_space;
      r.present_mode = __present_mode;
      r.image_usage = __image_usage;
      r.min_image_count = __min_image_count;
    }

    VkSwapchainKHR old = __h;
    __destroy_views();
    __h = nullptr;
    __build(r, old);
    if ( old && vkDestroySwapchainKHR ) vkDestroySwapchainKHR(__dev->handle(), old, host_allocation_callbacks());
  }

  void
  reset() noexcept
  {
    __destroy_views();
    if ( __h && __dev && vkDestroySwapchainKHR ) vkDestroySwapchainKHR(__dev->handle(), __h, host_allocation_callbacks());
    __h = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
