//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

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

inline constexpr usize max_pool_pages = 64;

struct sub_allocation {
  VkDeviceMemory memory = nullptr;
  VkDeviceSize offset = 0;
  VkDeviceSize size = 0;
  void *mapped = nullptr;

  bool
  valid() const noexcept
  {
    return memory != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }
};

class memory_pool
{
private:
  device *__dev = nullptr;
  VkDeviceSize __page_size = 0;
  u32 __type_index = ~0u;
  VkMemoryPropertyFlags __props = {};
  bool __persistent_map = false;

  memory_allocation __pages[max_pool_pages]{};
  VkDeviceSize __cursors[max_pool_pages]{};
  u32 __page_count = 0;

  static u32
  __pick_type(const VkPhysicalDeviceMemoryProperties &mp, u32 type_mask, VkMemoryPropertyFlags required) noexcept
  {
    for ( u32 i = 0; i < mp.memoryTypeCount; ++i ) {
      if ( (type_mask & (1u << i)) == 0 ) continue;
      if ( (mp.memoryTypes[i].propertyFlags & required) != required ) continue;
      return i;
    }
    return ~0u;
  }

  static VkDeviceSize
  __align_up(VkDeviceSize v, VkDeviceSize a) noexcept
  {
    if ( a == 0 ) return v;
    return (v + a - 1) & ~(a - 1);
  }

  void
  __append_page(VkDeviceSize min_bytes)
  {
    if ( __page_count >= max_pool_pages ) throw except::library_error("vk::memory_pool: page cap (max_pool_pages) reached");
    VkMemoryRequirements req{};
    req.size = min_bytes > __page_size ? min_bytes : __page_size;
    req.alignment = 1;
    req.memoryTypeBits = (1u << __type_index);
    memory_allocation page(*__dev, req, __props);
    if ( __persistent_map ) {
      (void)page.map();
    }
    __pages[__page_count] = static_cast<memory_allocation &&>(page);
    __cursors[__page_count] = 0;
    ++__page_count;
  }

public:
  ~memory_pool() = default;

  memory_pool() = default;

  memory_pool(device &dev, u32 type_mask, VkMemoryPropertyFlags required_props, VkDeviceSize page_size = 4 * 1024 * 1024,
              bool persistent_map = false)
      : __dev(&dev), __page_size(page_size), __props(required_props), __persistent_map(persistent_map)
  {
    if ( __page_size == 0 ) throw except::library_error("vk::memory_pool: page_size must be > 0");
    const auto &mp = dev.physical().memory_properties();
    __type_index = __pick_type(mp, type_mask, required_props);
    if ( __type_index == ~0u ) throw except::library_error("vk::memory_pool: no memory type satisfies the requested property mask");
    if ( persistent_map ) {
    }
  }

  memory_pool(const memory_pool &) = delete;
  memory_pool &operator=(const memory_pool &) = delete;

  memory_pool(memory_pool &&o) noexcept
      : __dev(o.__dev), __page_size(o.__page_size), __type_index(o.__type_index), __props(o.__props), __persistent_map(o.__persistent_map),
        __page_count(o.__page_count)
  {
    for ( u32 i = 0; i < o.__page_count; ++i ) {
      __pages[i] = static_cast<memory_allocation &&>(o.__pages[i]);
      __cursors[i] = o.__cursors[i];
    }
    o.__page_count = 0;
    o.__dev = nullptr;
  }

  memory_pool &
  operator=(memory_pool &&o) noexcept
  {
    if ( this != &o ) {

      for ( u32 i = 0; i < __page_count; ++i ) {
        __pages[i] = memory_allocation{};
        __cursors[i] = 0;
      }
      __page_count = 0;
      __dev = o.__dev;
      __page_size = o.__page_size;
      __type_index = o.__type_index;
      __props = o.__props;
      __persistent_map = o.__persistent_map;
      __page_count = o.__page_count;
      for ( u32 i = 0; i < o.__page_count; ++i ) {
        __pages[i] = static_cast<memory_allocation &&>(o.__pages[i]);
        __cursors[i] = o.__cursors[i];
      }
      o.__page_count = 0;
      o.__dev = nullptr;
    }
    return *this;
  }

  u32
  page_count() const noexcept
  {
    return __page_count;
  }

  VkDeviceSize
  page_size() const noexcept
  {
    return __page_size;
  }

  u32
  type_index() const noexcept
  {
    return __type_index;
  }

  bool
  persistent_map() const noexcept
  {
    return __persistent_map;
  }

  VkDeviceSize
  committed() const noexcept
  {
    VkDeviceSize t = 0;
    for ( u32 i = 0; i < __page_count; ++i ) t += __pages[i].size();
    return t;
  }

  VkDeviceSize
  used() const noexcept
  {
    VkDeviceSize t = 0;
    for ( u32 i = 0; i < __page_count; ++i ) t += __cursors[i];
    return t;
  }

  sub_allocation
  allocate(VkDeviceSize size, VkDeviceSize align = 1)
  {
    if ( !__dev ) throw except::library_error("vk::memory_pool::allocate: pool is uninitialized");
    if ( size == 0 ) return {};
    if ( align == 0 ) align = 1;

    for ( u32 i = 0; i < __page_count; ++i ) {
      const VkDeviceSize start = __align_up(__cursors[i], align);
      const VkDeviceSize end = start + size;
      if ( end <= __pages[i].size() ) {
        __cursors[i] = end;
        sub_allocation sa;
        sa.memory = __pages[i].handle();
        sa.offset = start;
        sa.size = size;
        if ( __persistent_map && __pages[i].mapped() ) {
          sa.mapped = static_cast<byte *>(__pages[i].mapped_ptr()) + start;
        }
        return sa;
      }
    }

    __append_page(size + align);
    const u32 i = __page_count - 1;
    const VkDeviceSize start = __align_up(__cursors[i], align);
    const VkDeviceSize end = start + size;
    __cursors[i] = end;
    sub_allocation sa;
    sa.memory = __pages[i].handle();
    sa.offset = start;
    sa.size = size;
    if ( __persistent_map && __pages[i].mapped() ) {
      sa.mapped = static_cast<byte *>(__pages[i].mapped_ptr()) + start;
    }
    return sa;
  }

  void
  reset() noexcept
  {
    for ( u32 i = 0; i < __page_count; ++i ) __cursors[i] = 0;
  }

  void
  release() noexcept
  {
    for ( u32 i = 0; i < __page_count; ++i ) {
      __pages[i] = memory_allocation{};
      __cursors[i] = 0;
    }
    __page_count = 0;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
