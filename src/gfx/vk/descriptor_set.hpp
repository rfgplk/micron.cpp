//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "descriptor_pool.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class descriptor_set
{
  VkDevice __dev = nullptr;
  VkDescriptorSet __h = nullptr;

  friend class descriptor_pool;

public:
  descriptor_set() = default;

  descriptor_set(VkDescriptorSet h, VkDevice dev) noexcept : __dev(dev), __h(h) { }

  VkDescriptorSet
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
  write_buffer(u32 binding, VkDescriptorType type, VkBuffer buf, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE,
               u32 array_element = 0) const
  {
    if ( !__h || !__dev || !vkUpdateDescriptorSets )
      throw except::library_error("vk::descriptor_set::write_buffer: vkUpdateDescriptorSets unresolved");
    VkDescriptorBufferInfo info{};
    info.buffer = buf;
    info.offset = offset;
    info.range = range;
    VkWriteDescriptorSet w{};
    w.sType = structure_type_of_v<VkWriteDescriptorSet>;
    w.dstSet = __h;
    w.dstBinding = binding;
    w.dstArrayElement = array_element;
    w.descriptorCount = 1;
    w.descriptorType = type;
    w.pBufferInfo = &info;
    vkUpdateDescriptorSets(__dev, 1, &w, 0, nullptr);
  }

  void
  write_image(u32 binding, VkDescriptorType type, VkImageView view, VkSampler sampler = VK_NULL_HANDLE,
              VkImageLayout layout = static_cast<VkImageLayout>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), u32 array_element = 0) const
  {
    if ( !__h || !__dev || !vkUpdateDescriptorSets )
      throw except::library_error("vk::descriptor_set::write_image: vkUpdateDescriptorSets unresolved");
    VkDescriptorImageInfo info{};
    info.sampler = sampler;
    info.imageView = view;
    info.imageLayout = layout;
    VkWriteDescriptorSet w{};
    w.sType = structure_type_of_v<VkWriteDescriptorSet>;
    w.dstSet = __h;
    w.dstBinding = binding;
    w.dstArrayElement = array_element;
    w.descriptorCount = 1;
    w.descriptorType = type;
    w.pImageInfo = &info;
    vkUpdateDescriptorSets(__dev, 1, &w, 0, nullptr);
  }

  static void
  update_many(VkDevice dev, const VkWriteDescriptorSet *writes, u32 write_count, const VkCopyDescriptorSet *copies = nullptr,
              u32 copy_count = 0) noexcept
  {
    if ( !dev || !vkUpdateDescriptorSets ) return;
    vkUpdateDescriptorSets(dev, write_count, writes, copy_count, copies);
  }
};

inline void
descriptor_pool::allocate(const VkDescriptorSetLayout *layouts, u32 count, descriptor_set *out)
{
  if ( !__h || !__dev || !vkAllocateDescriptorSets )
    throw except::library_error("vk::descriptor_pool::allocate: vkAllocateDescriptorSets unresolved");
  constexpr u32 cap = 32;
  if ( count > cap ) throw except::logic_error("vk::descriptor_pool::allocate: count exceeds inline cap (32)");
  VkDescriptorSet raw[cap]{};
  VkDescriptorSetAllocateInfo ai{};
  ai.sType = structure_type_of_v<VkDescriptorSetAllocateInfo>;
  ai.descriptorPool = __h;
  ai.descriptorSetCount = count;
  ai.pSetLayouts = layouts;
  check_vk(vkAllocateDescriptorSets(__dev, &ai, raw), "vkAllocateDescriptorSets");
  for ( u32 i = 0; i < count; ++i ) out[i] = descriptor_set(raw[i], __dev);
}

inline void
descriptor_pool::free(const descriptor_set *sets, u32 count) noexcept
{
  if ( !__h || !__dev || !__free_allowed || !vkFreeDescriptorSets || count == 0 ) return;
  constexpr u32 cap = 32;
  if ( count > cap ) return;
  VkDescriptorSet raw[cap]{};
  for ( u32 i = 0; i < count; ++i ) raw[i] = sets[i].handle();
  (void)vkFreeDescriptorSets(__dev, __h, count, raw);
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
