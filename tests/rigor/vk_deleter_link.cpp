//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// the link cliff.
//
// this file includes vulkan.hpp and DELIBERATELY NOT allocator.hpp, because that is the only shape
// that catches the bug it guards. the *_deleter functors in __bits/__vk_deleters.hpp default their
// allocator to host_allocation_callbacks(); if that name is merely FORWARD-DECLARED there rather
// than defined, every TU that reaches vulkan.hpp without allocator.hpp -- loader.hpp, queue.hpp,
// errors.hpp, physical_device.hpp and five more do -- compiles clean and then fails to link with
// "undefined reference to micron::gfx::vk::host_allocation_callbacks()". an inline function must be
// defined in every TU that odr-uses it, and a default member initializer odr-uses this one.
//
// tests/rigor/vk_allocator.cpp cannot catch it: it includes allocator.hpp, so the definition is
// always there. this cell has to stay allocator-free to keep meaning anything.

#include "../../src/gfx/vk/vulkan.hpp"
#include "../../src/io/console.hpp"
#include "../snowball/snowball.hpp"

namespace v = micron::gfx::vk;

using sb::end_test_case;
using sb::require;
using sb::test_case;

int
main()
{
  sb::print("=== VK DELETER LINK CLIFF ===");

  test_case("deleters resolve host_allocation_callbacks() without allocator.hpp");
  {
    const v::VkAllocationCallbacks *ours = v::host_allocation_callbacks();
    require(ours != nullptr);

    // odr-use the default member initializer from a TU that never saw allocator.hpp
    const v::instance_deleter inst{};
    const v::device_deleter dev{};
    const v::buffer_deleter buf{};
    const v::image_deleter img{};
    const v::device_memory_deleter mem{};

    require(inst.alloc == ours);
    require(dev.alloc == ours);
    require(buf.alloc == ours);
    require(img.alloc == ours);
    require(mem.alloc == ours);
  }
  end_test_case();

  test_case("a null handle through a deleter is inert");
  {
    // every vk* entry point is still an inline nullptr global here, so this exercises the guards in
    // the generated operator() bodies and nothing else
    const v::buffer_deleter buf{};
    buf(nullptr);
    const v::instance_deleter inst{};
    inst(nullptr);
  }
  end_test_case();

  sb::print("=== VK DELETER LINK CLIFF OK ===");
  return 1;
}
