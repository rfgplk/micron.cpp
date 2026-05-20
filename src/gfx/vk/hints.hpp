//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline constexpr u32
make_api_version(u32 variant, u32 major, u32 minor, u32 patch) noexcept
{
  return (variant << 29) | (major << 22) | (minor << 12) | patch;
}

struct instance_hints {
  const char *app_name = "micron-app";
  u32 app_version = make_api_version(0, 0, 1, 0);
  const char *engine_name = "micron";
  u32 engine_version = make_api_version(0, 0, 1, 0);
  u32 api_version = VK_API_VERSION_1_3;
  bool debug = false;
  bool surface = true;

  const char *const *extra_extensions = nullptr;
  usize extra_extensions_count = 0;
  const char *const *extra_layers = nullptr;
  usize extra_layers_count = 0;
};

struct device_hints {
  bool prefer_discrete = true;
  bool graphics = true;
  bool compute = true;
  bool transfer = true;
  bool present = true;
  VkPhysicalDeviceFeatures required_features{};
  VkPhysicalDeviceVulkan13Features required_vk13_features{};
  const char *const *extra_extensions = nullptr;
  usize extra_extensions_count = 0;
  bool debug = false;
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
