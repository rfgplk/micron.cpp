//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

namespace abc::inspection
{
inline constexpr u32 detail_magic = 0x44434241u;
inline constexpr u16 detail_version = 1;
inline constexpr u64 free_lists = 1, redzones = 2, ledger = 4, history = 8, pending = 16, quiescence = 32;

enum field : u32 {
  buddy_heads,
  buddy_mask,
  buddy_next,
  buddy_prev,
  tlsf_heads,
  tlsf_fl,
  tlsf_sl,
  tlsf_prev_phys,
  tlsf_next,
  tlsf_prev,
  tlsf_fl_count,
  tlsf_sl_count,
  tlsf_fl_shift,
  tlsf_sl_log2,
  redzone_byte,
  doctor_slots,
  doctor_cap,
  doctor_mask,
  doctor_occupied,
  doctor_live,
  doctor_freed,
  doctor_events,
  doctor_event_total,
  doctor_corruption,
  doctor_record_losses,
  doctor_event_losses,
  record_size,
  record_key,
  record_requested,
  record_granted,
  record_owner,
  record_state,
  record_flags,
  record_alloc_op,
  record_free_op,
  record_alloc_tid,
  record_free_tid,
  record_alloc_bt,
  record_free_bt,
  event_size,
  event_op,
  event_key,
  event_length,
  event_tid,
  event_kind,
  event_bt,
  bt_depth,
  event_capacity,
  doctor_canary,
  doctor_canary_byte,
  remote_cells,
  remote_tail,
  remote_head,
  remote_cell_size,
  remote_seq,
  remote_payload,
  remote_payload_ptr,
  remote_payload_size,
  remote_overflow,
  remote_overflow_next,
  remote_overflow_size,
  arena_busy,
  doctor_busy,
  field_count
};

struct detail_descriptor {
  u32 magic = detail_magic;
  u16 version = detail_version;
  u16 size = sizeof(detail_descriptor);
  u64 features = 0;
  u32 values[field_count]{};
};
}      // namespace abc::inspection
