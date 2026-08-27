//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fixed_map_allocator
//
// use when a mapping must begin at a caller selected virtual address, such as loader or ABI layout work

namespace micron
{

class fixed_map_allocator
{
public:
  [[nodiscard]] static chunk<byte>
  create_at(void *address, usize bytes)
  {
    const uintptr_t requested = reinterpret_cast<uintptr_t>(address);
    if ( address == nullptr || (requested & (page_size - 1)) != 0 )
      exc<except::invalid_argument>("fixed_map_allocator: address must be page aligned and non-null");
    if ( bytes == 0 ) return { nullptr, 0 };
    const usize mapping_len = allocation_round_up_or_throw(bytes, page_size);
    addr_t *memory = micron::mmap(reinterpret_cast<addr_t *>(address), mapping_len, prot_read | prot_write,
                                  map_private | map_anonymous | map_fixed_noreplace, -1, 0);
    if ( micron::mmap_failed(memory) ) exc<except::memory_error>("fixed_map_allocator: requested range is unavailable");
    if ( memory != address ) {
      micron::munmap(memory, mapping_len);
      exc<except::memory_error>("fixed_map_allocator: kernel ignored MAP_FIXED_NOREPLACE");
    }
    if ( !abc::register_external(reinterpret_cast<byte *>(memory), mapping_len, abc::external_provenance::fixed) ) {
      micron::munmap(memory, mapping_len);
      exc<except::memory_error>("fixed_map_allocator: provenance registration failed");
    }
    return { reinterpret_cast<byte *>(memory), bytes };
  }

  static void
  destroy(chunk<byte> memory) noexcept
  {
    if ( memory.ptr == nullptr || memory.len == 0 ) return;
    usize mapping_len;
    if ( !allocation_checked_round_up(memory.len, page_size, mapping_len) ) return;
    (void)abc::unregister_external(memory.ptr, mapping_len);
    micron::munmap(reinterpret_cast<addr_t *>(memory.ptr), mapping_len);
  }
};

};      // namespace micron
