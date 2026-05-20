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
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

struct sampler_request {
  VkFilter mag_filter = static_cast<VkFilter>(VK_FILTER_LINEAR);
  VkFilter min_filter = static_cast<VkFilter>(VK_FILTER_LINEAR);
  VkSamplerMipmapMode mipmap_mode = static_cast<VkSamplerMipmapMode>(VK_SAMPLER_MIPMAP_MODE_LINEAR);
  VkSamplerAddressMode address_mode_u = static_cast<VkSamplerAddressMode>(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  VkSamplerAddressMode address_mode_v = static_cast<VkSamplerAddressMode>(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  VkSamplerAddressMode address_mode_w = static_cast<VkSamplerAddressMode>(VK_SAMPLER_ADDRESS_MODE_REPEAT);
  float mip_lod_bias = 0.0f;
  bool anisotropy_enable = false;
  float max_anisotropy = 1.0f;
  bool compare_enable = false;
  VkCompareOp compare_op = static_cast<VkCompareOp>(VK_COMPARE_OP_NEVER);
  float min_lod = 0.0f;
  float max_lod = VK_LOD_CLAMP_NONE;
  VkBorderColor border_color = static_cast<VkBorderColor>(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
  bool unnormalized_coordinates = false;
};

class sampler
{
private:
  VkDevice __dev = nullptr;
  VkSampler __h = nullptr;

public:
  ~sampler() { reset(); }

  sampler() = default;

  sampler(device &dev, const VkSamplerCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateSampler ) throw except::library_error("vk::sampler: vkCreateSampler unresolved");
    check_vk(vkCreateSampler(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateSampler");
  }

  sampler(device &dev, const sampler_request &r) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateSampler ) throw except::library_error("vk::sampler: vkCreateSampler unresolved");
    VkSamplerCreateInfo ci{};
    ci.sType = structure_type_of_v<VkSamplerCreateInfo>;
    ci.magFilter = r.mag_filter;
    ci.minFilter = r.min_filter;
    ci.mipmapMode = r.mipmap_mode;
    ci.addressModeU = r.address_mode_u;
    ci.addressModeV = r.address_mode_v;
    ci.addressModeW = r.address_mode_w;
    ci.mipLodBias = r.mip_lod_bias;
    ci.anisotropyEnable = r.anisotropy_enable ? VK_TRUE : VK_FALSE;
    ci.maxAnisotropy = r.max_anisotropy;
    ci.compareEnable = r.compare_enable ? VK_TRUE : VK_FALSE;
    ci.compareOp = r.compare_op;
    ci.minLod = r.min_lod;
    ci.maxLod = r.max_lod;
    ci.borderColor = r.border_color;
    ci.unnormalizedCoordinates = r.unnormalized_coordinates ? VK_TRUE : VK_FALSE;
    check_vk(vkCreateSampler(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateSampler");
  }

  sampler(const sampler &) = delete;

  sampler(sampler &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  sampler &operator=(const sampler &) = delete;

  sampler &
  operator=(sampler &&o) noexcept
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

  VkSampler
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
    if ( __h && __dev && vkDestroySampler ) vkDestroySampler(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
