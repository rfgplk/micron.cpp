//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "command_pool.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class command_buffer
{
  VkDevice __dev = nullptr;
  VkCommandBuffer __h = nullptr;

  friend class command_pool;

public:
  command_buffer() = default;

  command_buffer(VkCommandBuffer h, VkDevice dev) noexcept : __dev(dev), __h(h) { }

  VkCommandBuffer
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
  begin(VkCommandBufferUsageFlags usage_flags = {})
  {
    if ( !__h || !vkBeginCommandBuffer ) throw except::library_error("vk::command_buffer::begin: vkBeginCommandBuffer unresolved");
    VkCommandBufferBeginInfo bi{};
    bi.sType = structure_type_of_v<VkCommandBufferBeginInfo>;
    bi.flags = usage_flags;
    check_vk(vkBeginCommandBuffer(__h, &bi), "vkBeginCommandBuffer");
  }

  void
  end()
  {
    if ( !__h || !vkEndCommandBuffer ) return;
    check_vk(vkEndCommandBuffer(__h), "vkEndCommandBuffer");
  }

  void
  reset_buffer(VkCommandBufferResetFlags flags = {})
  {
    if ( !__h || !vkResetCommandBuffer ) return;
    check_vk(vkResetCommandBuffer(__h, flags), "vkResetCommandBuffer");
  }

  void
  begin_render_pass(const VkRenderPassBeginInfo &info,
                    VkSubpassContents contents = static_cast<VkSubpassContents>(VK_SUBPASS_CONTENTS_INLINE)) noexcept
  {
    if ( __h && vkCmdBeginRenderPass ) vkCmdBeginRenderPass(__h, &info, contents);
  }

  void
  end_render_pass() noexcept
  {
    if ( __h && vkCmdEndRenderPass ) vkCmdEndRenderPass(__h);
  }

  void
  begin_rendering(const VkRenderingInfo &info) noexcept
  {
    if ( __h && vkCmdBeginRendering ) vkCmdBeginRendering(__h, &info);
  }

  void
  end_rendering() noexcept
  {
    if ( __h && vkCmdEndRendering ) vkCmdEndRendering(__h);
  }

  void
  bind_pipeline(VkPipelineBindPoint point, VkPipeline pipe) noexcept
  {
    if ( __h && vkCmdBindPipeline ) vkCmdBindPipeline(__h, point, pipe);
  }

  void
  set_viewport(const VkViewport &v) noexcept
  {
    if ( __h && vkCmdSetViewport ) vkCmdSetViewport(__h, 0, 1, &v);
  }

  void
  set_viewports(const VkViewport *v, u32 count, u32 first = 0) noexcept
  {
    if ( __h && vkCmdSetViewport ) vkCmdSetViewport(__h, first, count, v);
  }

  void
  set_scissor(const VkRect2D &r) noexcept
  {
    if ( __h && vkCmdSetScissor ) vkCmdSetScissor(__h, 0, 1, &r);
  }

  void
  set_scissors(const VkRect2D *r, u32 count, u32 first = 0) noexcept
  {
    if ( __h && vkCmdSetScissor ) vkCmdSetScissor(__h, first, count, r);
  }

  void
  bind_vertex_buffers(u32 first, u32 count, const VkBuffer *buffers, const VkDeviceSize *offsets) noexcept
  {
    if ( __h && vkCmdBindVertexBuffers ) vkCmdBindVertexBuffers(__h, first, count, buffers, offsets);
  }

  void
  bind_index_buffer(VkBuffer buf, VkDeviceSize offset, VkIndexType type) noexcept
  {
    if ( __h && vkCmdBindIndexBuffer ) vkCmdBindIndexBuffer(__h, buf, offset, type);
  }

  void
  draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) noexcept
  {
    if ( __h && vkCmdDraw ) vkCmdDraw(__h, vertex_count, instance_count, first_vertex, first_instance);
  }

  void
  draw_indexed(u32 index_count, u32 instance_count = 1, u32 first_index = 0, i32 vertex_offset = 0, u32 first_instance = 0) noexcept
  {
    if ( __h && vkCmdDrawIndexed ) vkCmdDrawIndexed(__h, index_count, instance_count, first_index, vertex_offset, first_instance);
  }

  void
  dispatch(u32 x, u32 y, u32 z) noexcept
  {
    if ( __h && vkCmdDispatch ) vkCmdDispatch(__h, x, y, z);
  }

  void
  copy_buffer(VkBuffer src, VkBuffer dst, const VkBufferCopy *regions, u32 region_count) noexcept
  {
    if ( __h && vkCmdCopyBuffer ) vkCmdCopyBuffer(__h, src, dst, region_count, regions);
  }

  void
  copy_image(VkImage src, VkImageLayout src_layout, VkImage dst, VkImageLayout dst_layout, const VkImageCopy *regions,
             u32 region_count) noexcept
  {
    if ( __h && vkCmdCopyImage ) vkCmdCopyImage(__h, src, src_layout, dst, dst_layout, region_count, regions);
  }

  void
  copy_buffer_to_image(VkBuffer src, VkImage dst, VkImageLayout dst_layout, const VkBufferImageCopy *regions, u32 region_count) noexcept
  {
    if ( __h && vkCmdCopyBufferToImage ) vkCmdCopyBufferToImage(__h, src, dst, dst_layout, region_count, regions);
  }

  void
  copy_image_to_buffer(VkImage src, VkImageLayout src_layout, VkBuffer dst, const VkBufferImageCopy *regions, u32 region_count) noexcept
  {
    if ( __h && vkCmdCopyImageToBuffer ) vkCmdCopyImageToBuffer(__h, src, src_layout, dst, region_count, regions);
  }

  void
  pipeline_barrier(VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkDependencyFlags dep_flags, const VkMemoryBarrier *mem,
                   u32 mem_count, const VkBufferMemoryBarrier *buf, u32 buf_count, const VkImageMemoryBarrier *img, u32 img_count) noexcept
  {
    if ( __h && vkCmdPipelineBarrier )
      vkCmdPipelineBarrier(__h, src_stage, dst_stage, dep_flags, mem_count, mem, buf_count, buf, img_count, img);
  }

  void
  pipeline_barrier2(const VkDependencyInfo &info) noexcept
  {
    if ( __h && vkCmdPipelineBarrier2 ) vkCmdPipelineBarrier2(__h, &info);
  }

  void
  reset_query_pool(VkQueryPool pool, u32 first, u32 count) noexcept
  {
    if ( __h && vkCmdResetQueryPool ) vkCmdResetQueryPool(__h, pool, first, count);
  }

  void
  begin_query(VkQueryPool pool, u32 index, VkQueryControlFlags flags = {}) noexcept
  {
    if ( __h && vkCmdBeginQuery ) vkCmdBeginQuery(__h, pool, index, flags);
  }

  void
  end_query(VkQueryPool pool, u32 index) noexcept
  {
    if ( __h && vkCmdEndQuery ) vkCmdEndQuery(__h, pool, index);
  }

  void
  write_timestamp(VkPipelineStageFlagBits stage, VkQueryPool pool, u32 index) noexcept
  {
    if ( __h && vkCmdWriteTimestamp ) vkCmdWriteTimestamp(__h, stage, pool, index);
  }

  void
  write_timestamp2(VkPipelineStageFlags2 stage, VkQueryPool pool, u32 index) noexcept
  {
    if ( __h && vkCmdWriteTimestamp2 ) vkCmdWriteTimestamp2(__h, stage, pool, index);
  }

  void
  bind_descriptor_sets(VkPipelineBindPoint bind_point, VkPipelineLayout layout, u32 first_set, const VkDescriptorSet *sets, u32 count,
                       const u32 *dynamic_offsets = nullptr, u32 dynamic_offset_count = 0) noexcept
  {
    if ( __h && vkCmdBindDescriptorSets )
      vkCmdBindDescriptorSets(__h, bind_point, layout, first_set, count, sets, dynamic_offset_count, dynamic_offsets);
  }

  void
  set_event(VkEvent ev, VkPipelineStageFlags stage) noexcept
  {
    if ( __h && vkCmdSetEvent ) vkCmdSetEvent(__h, ev, stage);
  }

  void
  reset_event(VkEvent ev, VkPipelineStageFlags stage) noexcept
  {
    if ( __h && vkCmdResetEvent ) vkCmdResetEvent(__h, ev, stage);
  }
};

inline void
command_pool::allocate(VkCommandBufferLevel level, u32 count, command_buffer *out)
{
  if ( !__h || !__dev || !vkAllocateCommandBuffers )
    throw except::library_error("vk::command_pool::allocate: vkAllocateCommandBuffers unresolved");

  constexpr u32 cap = 16;
  if ( count > cap ) throw except::logic_error("vk::command_pool::allocate: count exceeds inline cap (16)");
  VkCommandBuffer raw[cap]{};
  VkCommandBufferAllocateInfo ai{};
  ai.sType = structure_type_of_v<VkCommandBufferAllocateInfo>;
  ai.commandPool = __h;
  ai.level = level;
  ai.commandBufferCount = count;
  check_vk(vkAllocateCommandBuffers(__dev, &ai, raw), "vkAllocateCommandBuffers");
  for ( u32 i = 0; i < count; ++i ) out[i] = command_buffer(raw[i], __dev);
}

inline void
command_pool::free(const command_buffer *bufs, u32 count) noexcept
{
  if ( !__h || !__dev || !vkFreeCommandBuffers || count == 0 ) return;
  constexpr u32 cap = 16;
  if ( count > cap ) return;
  VkCommandBuffer raw[cap]{};
  for ( u32 i = 0; i < count; ++i ) raw[i] = bufs[i].handle();
  vkFreeCommandBuffers(__dev, __h, count, raw);
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
