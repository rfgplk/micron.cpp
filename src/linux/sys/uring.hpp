//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "../../atomic/intrin.hpp"
#include "../../memory/mman.hpp"
#include "../../syscall.hpp"
#include "time.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io_uring
//
// NOTE: originally partially ported from libjkr; now the canonical micron implementation

namespace micron
{
namespace uring
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// opcodes (IORING_OP_*)

inline constexpr u8 op_nop = 0;
inline constexpr u8 op_readv = 1;
inline constexpr u8 op_writev = 2;
inline constexpr u8 op_fsync = 3;
inline constexpr u8 op_read_fixed = 4;
inline constexpr u8 op_write_fixed = 5;
inline constexpr u8 op_poll_add = 6;
inline constexpr u8 op_poll_remove = 7;
inline constexpr u8 op_sync_file_range = 8;
inline constexpr u8 op_sendmsg = 9;
inline constexpr u8 op_recvmsg = 10;
inline constexpr u8 op_timeout = 11;
inline constexpr u8 op_timeout_remove = 12;
inline constexpr u8 op_accept = 13;
inline constexpr u8 op_async_cancel = 14;
inline constexpr u8 op_link_timeout = 15;
inline constexpr u8 op_connect = 16;
inline constexpr u8 op_fallocate = 17;             // >=5.6
inline constexpr u8 op_openat = 18;                // >=5.6
inline constexpr u8 op_close = 19;                 // >=5.6
inline constexpr u8 op_files_update = 20;          // >=5.6
inline constexpr u8 op_statx = 21;                 // >=5.6
inline constexpr u8 op_read = 22;                  // >=5.6
inline constexpr u8 op_write = 23;                 // >=5.6
inline constexpr u8 op_fadvise = 24;               // >=5.6
inline constexpr u8 op_madvise = 25;               // >=5.6
inline constexpr u8 op_send = 26;                  // >=5.6
inline constexpr u8 op_recv = 27;                  // >=5.6
inline constexpr u8 op_openat2 = 28;               // >=5.6
inline constexpr u8 op_epoll_ctl = 29;             // >=5.6
inline constexpr u8 op_splice = 30;                // >=5.7
inline constexpr u8 op_provide_buffers = 31;       // >=5.7
inline constexpr u8 op_remove_buffers = 32;        // >=5.7
inline constexpr u8 op_tee = 33;                   // >=5.8
inline constexpr u8 op_shutdown = 34;              // >=5.11
inline constexpr u8 op_renameat = 35;              // >=5.11
inline constexpr u8 op_unlinkat = 36;              // >=5.11
inline constexpr u8 op_mkdirat = 37;               // >=5.15
inline constexpr u8 op_symlinkat = 38;             // >=5.15
inline constexpr u8 op_linkat = 39;                // >=5.15
inline constexpr u8 op_msg_ring = 40;              // >=5.18
inline constexpr u8 op_fsetxattr = 41;             // >=5.19
inline constexpr u8 op_setxattr = 42;              // >=5.19
inline constexpr u8 op_fgetxattr = 43;             // >=5.19
inline constexpr u8 op_getxattr = 44;              // >=5.19
inline constexpr u8 op_socket = 45;                // >=5.19
inline constexpr u8 op_uring_cmd = 46;             // >=5.19
inline constexpr u8 op_send_zc = 47;               // >=6.0
inline constexpr u8 op_sendmsg_zc = 48;            // >=6.1
inline constexpr u8 op_read_multishot = 49;        // >=6.7
inline constexpr u8 op_waitid = 50;                // >=6.7
inline constexpr u8 op_futex_wait = 51;            // >=6.7
inline constexpr u8 op_futex_wake = 52;            // >=6.7
inline constexpr u8 op_futex_waitv = 53;           // >=6.7
inline constexpr u8 op_fixed_fd_install = 54;      // >=6.8
inline constexpr u8 op_ftruncate = 55;             // >=6.9
inline constexpr u8 op_bind = 56;                  // >=6.11
inline constexpr u8 op_listen = 57;                // >=6.11
inline constexpr u8 op_recv_zc = 58;               // >=6.15
inline constexpr u8 op_epoll_wait = 59;            // >=6.15
inline constexpr u8 op_readv_fixed = 60;           // >=6.15
inline constexpr u8 op_writev_fixed = 61;          // >=6.15
inline constexpr u8 op_pipe = 62;                  // >=6.16
inline constexpr u8 op_nop128 = 63;                // >=7.1 (sqe_mixed rings)
inline constexpr u8 op_uring_cmd128 = 64;          // >=7.1 (sqe_mixed rings)
inline constexpr u8 op_last = 65;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// io_uring_setup flags (IORING_SETUP_*)

inline constexpr u32 setup_iopoll = 1u << 0;
inline constexpr u32 setup_sqpoll = 1u << 1;
inline constexpr u32 setup_sq_aff = 1u << 2;      // sq_thread_cpu is valid
inline constexpr u32 setup_cqsize = 1u << 3;
inline constexpr u32 setup_clamp = 1u << 4;
inline constexpr u32 setup_attach_wq = 1u << 5;
inline constexpr u32 setup_r_disabled = 1u << 6;               // >=5.10; start disabled, enable_rings() later
inline constexpr u32 setup_submit_all = 1u << 7;               // >=5.18
inline constexpr u32 setup_coop_taskrun = 1u << 8;             // >=5.19
inline constexpr u32 setup_taskrun_flag = 1u << 9;             // >=5.19; sets sq_taskrun when work is pending
inline constexpr u32 setup_sqe128 = 1u << 10;                  // >=5.19; 128-byte sqes
inline constexpr u32 setup_cqe32 = 1u << 11;                   // >=5.19; 32-byte cqes
inline constexpr u32 setup_single_issuer = 1u << 12;           // >=6.0
inline constexpr u32 setup_defer_taskrun = 1u << 13;           // >=6.1
inline constexpr u32 setup_no_mmap = 1u << 14;                 // >=6.5; app provides ring memory
inline constexpr u32 setup_registered_fd_only = 1u << 15;      // >=6.5; returns a registered index, no fd
inline constexpr u32 setup_no_sqarray = 1u << 16;              // >=6.6; drop the sq index array indirection
inline constexpr u32 setup_hybrid_iopoll = 1u << 17;           // >=6.13
inline constexpr u32 setup_cqe_mixed = 1u << 18;               // >=6.18; 16b and 32b cqes coexist
inline constexpr u32 setup_sqe_mixed = 1u << 19;               // >=7.1; 64b and 128b sqes coexist
inline constexpr u32 setup_sq_rewind = 1u << 20;               // >=7.1; sqes always start at index 0

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// io_uring_enter flags (IORING_ENTER_*)

inline constexpr u32 enter_getevents = 1u << 0;
inline constexpr u32 enter_sq_wakeup = 1u << 1;
inline constexpr u32 enter_sq_wait = 1u << 2;
inline constexpr u32 enter_ext_arg = 1u << 3;
inline constexpr u32 enter_registered_ring = 1u << 4;
inline constexpr u32 enter_abs_timer = 1u << 5;        // >=6.12 (with register_clock)
inline constexpr u32 enter_ext_arg_reg = 1u << 6;      // >=6.13 (registered wait regions)
inline constexpr u32 enter_no_iowait = 1u << 7;        // >=6.17

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// feature bits reported in params.features (IORING_FEAT_*)

inline constexpr u32 feat_single_mmap = 1u << 0;
inline constexpr u32 feat_nodrop = 1u << 1;
inline constexpr u32 feat_submit_stable = 1u << 2;
inline constexpr u32 feat_rw_cur_pos = 1u << 3;
inline constexpr u32 feat_cur_personality = 1u << 4;
inline constexpr u32 feat_fast_poll = 1u << 5;
inline constexpr u32 feat_poll_32bits = 1u << 6;
inline constexpr u32 feat_sqpoll_nonfixed = 1u << 7;
inline constexpr u32 feat_ext_arg = 1u << 8;
inline constexpr u32 feat_native_workers = 1u << 9;
inline constexpr u32 feat_rsrc_tags = 1u << 10;
inline constexpr u32 feat_cqe_skip = 1u << 11;
inline constexpr u32 feat_linked_file = 1u << 12;
inline constexpr u32 feat_reg_reg_ring = 1u << 13;         // >=6.3; registered ring fd works for register(2)
inline constexpr u32 feat_recvsend_bundle = 1u << 14;      // >=6.10
inline constexpr u32 feat_min_timeout = 1u << 15;          // >=6.12
inline constexpr u32 feat_rw_attr = 1u << 16;              // >=6.13
inline constexpr u32 feat_no_iowait = 1u << 17;            // >=6.17

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-sqe flags (IOSQE_*)

inline constexpr u8 sqe_fixed_file = 1u << 0;
inline constexpr u8 sqe_io_drain = 1u << 1;
inline constexpr u8 sqe_io_link = 1u << 2;
inline constexpr u8 sqe_io_hardlink = 1u << 3;
inline constexpr u8 sqe_async = 1u << 4;
inline constexpr u8 sqe_buffer_select = 1u << 5;
inline constexpr u8 sqe_cqe_skip_success = 1u << 6;      // needs feat_cqe_skip (>=5.17)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cqe flags (IORING_CQE_F_*), sq/cq ring flags, mmap offsets

inline constexpr u32 cqe_f_buffer = 1u << 0;             // upper 16 bits of cqe.flags = buffer id
inline constexpr u32 cqe_f_more = 1u << 1;               // multishot parent will post more cqes
inline constexpr u32 cqe_f_sock_nonempty = 1u << 2;      // >=5.19
inline constexpr u32 cqe_f_notif = 1u << 3;              // >=6.0; zerocopy notification cqe
inline constexpr u32 cqe_f_buf_more = 1u << 4;           // >=6.12; incremental pbuf consumption
inline constexpr u32 cqe_f_skip = 1u << 5;               // >=6.18; ring-gap filler, ignore entry
inline constexpr u32 cqe_f_32 = 1u << 15;                // >=6.18; 32b cqe on a cqe_mixed ring
inline constexpr u32 cqe_f_tstamp_hw = 1u << 16;         // >=6.17; SOCKET_URING_OP_TX_TIMESTAMP
inline constexpr u32 cqe_buffer_shift = 16;
inline constexpr u32 timestamp_hw_shift = 16;
inline constexpr u32 timestamp_type_shift = 17;

inline constexpr u32 sq_need_wakeup = 1u << 0;      // sqpoll thread parked; enter with enter_sq_wakeup
inline constexpr u32 sq_cq_overflow = 1u << 1;      // cq ring overflowed
inline constexpr u32 sq_taskrun = 1u << 2;          // >=5.19; pending task work (setup_taskrun_flag)

inline constexpr u32 cq_eventfd_disabled = 1u << 0;

// mmap offsets (IORING_OFF_*)
inline constexpr u64 off_sq_ring = 0ull;
inline constexpr u64 off_cq_ring = 0x8000000ull;
inline constexpr u64 off_sqes = 0x10000000ull;
inline constexpr u64 off_pbuf_ring = 0x80000000ull;      // >=5.19; | (bgid << off_pbuf_shift)
inline constexpr u64 off_pbuf_shift = 16;
inline constexpr u64 off_mmap_mask = 0xf8000000ull;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// io_uring_register opcodes (IORING_REGISTER_* / IORING_UNREGISTER_*)

inline constexpr u32 reg_register_buffers = 0;
inline constexpr u32 reg_unregister_buffers = 1;
inline constexpr u32 reg_register_files = 2;
inline constexpr u32 reg_unregister_files = 3;
inline constexpr u32 reg_register_eventfd = 4;
inline constexpr u32 reg_unregister_eventfd = 5;
inline constexpr u32 reg_register_files_update = 6;
inline constexpr u32 reg_register_eventfd_async = 7;
inline constexpr u32 reg_register_probe = 8;                  // >=5.6
inline constexpr u32 reg_register_personality = 9;            // >=5.6
inline constexpr u32 reg_unregister_personality = 10;         // >=5.6
inline constexpr u32 reg_register_restrictions = 11;          // >=5.10
inline constexpr u32 reg_register_enable_rings = 12;          // >=5.10
inline constexpr u32 reg_register_files2 = 13;                // >=5.13; tagged
inline constexpr u32 reg_register_files_update2 = 14;         // >=5.13
inline constexpr u32 reg_register_buffers2 = 15;              // >=5.13
inline constexpr u32 reg_register_buffers_update = 16;        // >=5.13
inline constexpr u32 reg_register_iowq_aff = 17;              // >=5.14
inline constexpr u32 reg_unregister_iowq_aff = 18;            // >=5.14
inline constexpr u32 reg_register_iowq_max_workers = 19;      // >=5.15
inline constexpr u32 reg_register_ring_fds = 20;              // >=5.18
inline constexpr u32 reg_unregister_ring_fds = 21;            // >=5.18
inline constexpr u32 reg_register_pbuf_ring = 22;             // >=5.19
inline constexpr u32 reg_unregister_pbuf_ring = 23;           // >=5.19
inline constexpr u32 reg_register_sync_cancel = 24;           // >=6.0
inline constexpr u32 reg_register_file_alloc_range = 25;      // >=6.0
inline constexpr u32 reg_register_pbuf_status = 26;           // >=6.8
inline constexpr u32 reg_register_napi = 27;                  // >=6.9
inline constexpr u32 reg_unregister_napi = 28;                // >=6.9
inline constexpr u32 reg_register_clock = 29;                 // >=6.12
inline constexpr u32 reg_register_clone_buffers = 30;         // >=6.12
inline constexpr u32 reg_register_send_msg_ring = 31;         // >=6.13; msg_ring without a local ring
inline constexpr u32 reg_register_zcrx_ifq = 32;              // >=6.15
inline constexpr u32 reg_register_resize_rings = 33;          // >=6.13
inline constexpr u32 reg_register_mem_region = 34;            // >=6.13
inline constexpr u32 reg_register_query = 35;                 // >=6.17
inline constexpr u32 reg_register_zcrx_ctrl = 36;             // >=6.18
inline constexpr u32 reg_register_bpf_filter = 37;            // >=7.1
inline constexpr u32 reg_last = 38;
inline constexpr u32 reg_use_registered_ring = 1u << 31;      // OR into the opcode; fd is a registered index

// io-wq worker categories (IO_WQ_*; index into register_iowq_max_workers array)
inline constexpr u32 wq_bound = 0;
inline constexpr u32 wq_unbound = 1;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-op flag families

// op_fsync: sqe.fsync_flags
inline constexpr u32 fsync_datasync = 1u << 0;

// op_timeout / op_link_timeout: sqe.timeout_flags
inline constexpr u32 timeout_abs = 1u << 0;                // deadline is absolute (CLOCK_MONOTONIC unless clocked)
inline constexpr u32 timeout_update = 1u << 1;             // >=5.11
inline constexpr u32 timeout_boottime = 1u << 2;           // >=5.15
inline constexpr u32 timeout_realtime = 1u << 3;           // >=5.15
inline constexpr u32 link_timeout_update = 1u << 4;        // >=5.15
inline constexpr u32 timeout_etime_success = 1u << 5;      // >=5.16
inline constexpr u32 timeout_multishot = 1u << 6;          // >=6.4
inline constexpr u32 timeout_immediate_arg = 1u << 7;      // >=6.17; sqe.addr is ns, not a timespec*
inline constexpr u32 timeout_clock_mask = timeout_boottime | timeout_realtime;
inline constexpr u32 timeout_update_mask = timeout_update | link_timeout_update;

// op_splice / op_tee: sqe.splice_flags (extends splice(2) flags)
inline constexpr u32 splice_f_fd_in_fixed = 1u << 31;

// op_poll_add: command flags live in sqe.len (events are in poll32_events)
inline constexpr u32 poll_add_multi = 1u << 0;             // >=5.13; multishot
inline constexpr u32 poll_update_events = 1u << 1;         // >=5.13
inline constexpr u32 poll_update_user_data = 1u << 2;      // >=5.13
inline constexpr u32 poll_add_level = 1u << 3;             // >=6.0; level-triggered

// op_async_cancel: sqe.cancel_flags
inline constexpr u32 async_cancel_all = 1u << 0;           // >=5.19
inline constexpr u32 async_cancel_fd = 1u << 1;            // >=5.19; key off fd instead of user_data
inline constexpr u32 async_cancel_any = 1u << 2;           // >=5.19
inline constexpr u32 async_cancel_fd_fixed = 1u << 3;      // >=6.0
inline constexpr u32 async_cancel_userdata = 1u << 4;      // >=6.8
inline constexpr u32 async_cancel_op = 1u << 5;            // >=6.8; match on opcode

// op_send/op_recv/op_sendmsg/op_recvmsg families: flags ride sqe.ioprio (u16)
inline constexpr u16 recvsend_poll_first = 1u << 0;         // >=5.19
inline constexpr u16 recv_multishot = 1u << 1;              // >=6.0
inline constexpr u16 recvsend_fixed_buf = 1u << 2;          // >=6.0
inline constexpr u16 send_zc_report_usage = 1u << 3;        // >=6.2
inline constexpr u16 recvsend_bundle = 1u << 4;             // >=6.10
inline constexpr u16 send_vectorized = 1u << 5;             // >=6.17
inline constexpr u32 notif_usage_zc_copied = 1u << 31;      // in cqe.res of the notif cqe

// op_accept: flags ride sqe.ioprio (u16)
inline constexpr u16 accept_multishot = 1u << 0;       // >=5.19
inline constexpr u16 accept_dontwait = 1u << 1;        // >=6.10
inline constexpr u16 accept_poll_first = 1u << 2;      // >=6.10

// op_msg_ring: command in sqe.addr, flags in sqe.msg_ring_flags
inline constexpr u64 msg_data = 0ull;                    // pass user_data/res to the target CQ
inline constexpr u64 msg_send_fd = 1ull;                 // >=6.0; send a registered fd to the target ring
inline constexpr u32 msg_ring_cqe_skip = 1u << 0;        // >=6.0; no cqe on the target
inline constexpr u32 msg_ring_flags_pass = 1u << 1;      // >=6.3; sqe.file_index -> target cqe.flags

// op_uring_cmd: sqe.uring_cmd_flags (top 8 bits reserved for the kernel)
inline constexpr u32 uring_cmd_fixed = 1u << 0;          // >=6.1; use registered buffer via buf_index
inline constexpr u32 uring_cmd_multishot = 1u << 1;      // >=6.17
inline constexpr u32 uring_cmd_mask = uring_cmd_fixed | uring_cmd_multishot;

// op_fixed_fd_install: sqe.install_fd_flags
inline constexpr u32 fixed_fd_no_cloexec = 1u << 0;      // >=6.8

// op_nop: sqe.nop_flags
inline constexpr u32 nop_inject_result = 1u << 0;      // >=6.8; cqe.res = sqe.len
inline constexpr u32 nop_file = 1u << 1;               // >=6.13
inline constexpr u32 nop_fixed_file = 1u << 2;         // >=6.13
inline constexpr u32 nop_fixed_buffer = 1u << 3;       // >=6.13
inline constexpr u32 nop_tw = 1u << 4;                 // >=6.16
inline constexpr u32 nop_cqe32 = 1u << 5;              // >=6.18

// rw attributes (sqe.attr_type_mask / attr_ptr; needs feat_rw_attr)
inline constexpr u64 rw_attr_flag_pi = 1u << 0;      // >=6.13; attr_ptr -> attr_pi

// direct-descriptor encoding: sqe.file_index = slot + 1; this sentinel asks the kernel to allocate
inline constexpr u32 file_index_alloc = ~0u;      // >=5.19 (alloc); slot+1 form >=5.15
// skip this index in register_files_update arrays
inline constexpr i32 register_files_skip = -2;      // >=5.12

// probe: probe_op.flags bit
inline constexpr u16 probe_op_supported = 1u << 0;

// rsrc_register.flags
inline constexpr u32 rsrc_register_sparse = 1u << 0;      // >=5.19

// mem region (register_mem_region)
inline constexpr u32 mem_region_type_user = 1;         // region_desc.flags
inline constexpr u64 mem_region_reg_wait_arg = 1;      // mem_region_reg.flags
inline constexpr u32 reg_wait_ts = 1u << 0;            // reg_wait.flags

// pbuf ring registration flags (IOU_PBUF_RING_*)
inline constexpr u16 pbuf_ring_mmap = 1;      // >=6.4; kernel allocates the ring, mmap it back
inline constexpr u16 pbuf_ring_inc = 2;       // >=6.12; incremental consumption

// clone_buffers.flags (IORING_REGISTER_SRC_REGISTERED / DST_REPLACE)
inline constexpr u32 clone_src_registered = 1u << 0;      // >=6.12
inline constexpr u32 clone_dst_replace = 1u << 1;         // >=6.13

// restriction.opcode values (IORING_RESTRICTION_*)
inline constexpr u16 restriction_register_op = 0;
inline constexpr u16 restriction_sqe_op = 1;
inline constexpr u16 restriction_sqe_flags_allowed = 2;
inline constexpr u16 restriction_sqe_flags_required = 3;
inline constexpr u16 restriction_last = 4;

// napi (io_uring_napi_op / tracking strategy)
inline constexpr u8 napi_register_op = 0;
inline constexpr u8 napi_static_add_id = 1;
inline constexpr u8 napi_static_del_id = 2;
inline constexpr u32 napi_tracking_dynamic = 0;
inline constexpr u32 napi_tracking_static = 1;
inline constexpr u32 napi_tracking_inactive = 255;

// socket uring_cmd ops (SOCKET_URING_OP_*)
inline constexpr u32 socket_op_siocinq = 0;
inline constexpr u32 socket_op_siocoutq = 1;
inline constexpr u32 socket_op_getsockopt = 2;        // >=6.7
inline constexpr u32 socket_op_setsockopt = 3;        // >=6.7
inline constexpr u32 socket_op_tx_timestamp = 4;      // >=6.17
inline constexpr u32 socket_op_getsockname = 5;       // >=6.18

// zcrx (zerocopy receive, >=6.15; <linux/io_uring/zcrx.h>)
inline constexpr u32 zcrx_area_shift = 48;
inline constexpr u64 zcrx_area_mask = ~((1ull << zcrx_area_shift) - 1);
inline constexpr u32 zcrx_area_dmabuf = 1;
inline constexpr u32 zcrx_reg_import = 1;      // >=6.18
inline constexpr u32 zcrx_reg_nodev = 2;       // >=6.18
inline constexpr u32 zcrx_feature_rx_page_size = 1u << 0;
inline constexpr u32 zcrx_ctrl_op_flush_rq = 0;      // >=6.18
inline constexpr u32 zcrx_ctrl_op_export = 1;        // >=6.18

// query ops (>=6.17; <linux/io_uring/query.h>)
inline constexpr u32 query_opcodes = 0;
inline constexpr u32 query_zcrx = 1;
inline constexpr u32 query_scq = 2;

// futex2 flags for the futex ops (sqe.fd carries them; FUTEX2_*)
inline constexpr u32 futex2_size_u32 = 0x02;
inline constexpr u32 futex2_private = 128;
inline constexpr u64 futex_match_any = 0xffffffffull;      // FUTEX_BITSET_MATCH_ANY

// %%%%%%%%%%%%%%%%%
// ABI structs

struct ktimespec {
  i64 tv_sec;
  i64 tv_nsec;
};

static_assert(sizeof(ktimespec) == 16, "__kernel_timespec ABI");

// local 16-byte iovec mirror
struct iovec {
  void *iov_base;
  usize iov_len;
};

static_assert(sizeof(iovec) == 2 * sizeof(void *), "iovec ABI");

struct sq_offsets {
  u32 head;
  u32 tail;
  u32 ring_mask;
  u32 ring_entries;
  u32 flags;
  u32 dropped;
  u32 array;
  u32 resv1;
  u64 user_addr;
};

struct cq_offsets {
  u32 head;
  u32 tail;
  u32 ring_mask;
  u32 ring_entries;
  u32 overflow;
  u32 cqes;
  u32 flags;
  u32 resv1;
  u64 user_addr;
};

struct params {
  u32 sq_entries;
  u32 cq_entries;
  u32 flags;
  u32 sq_thread_cpu;
  u32 sq_thread_idle;
  u32 features;
  u32 wq_fd;
  u32 resv[3];
  sq_offsets sq_off;
  cq_offsets cq_off;
};

static_assert(sizeof(sq_offsets) == 40, "io_sqring_offsets ABI");
static_assert(sizeof(cq_offsets) == 40, "io_cqring_offsets ABI");
static_assert(sizeof(params) == 120, "io_uring_params ABI");
static_assert(__builtin_offsetof(params, features) == 20, "io_uring_params.features ABI");
static_assert(__builtin_offsetof(params, sq_off) == 40, "io_uring_params.sq_off ABI");
static_assert(__builtin_offsetof(params, cq_off) == 80, "io_uring_params.cq_off ABI");

struct sqe {
  u8 opcode;
  u8 flags;        // sqe_* bits
  u16 ioprio;      // or recvsend_* / accept_* op flags
  i32 fd;

  union {         // offset 8
    u64 off;      // offset into file
    u64 addr2;
    u32 cmd_op;      // uring_cmd; high half stays zero
  };

  union {          // offset 16
    u64 addr;      // buffer or iovecs
    u64 splice_off_in;
    u32 level;      // sockopt level; optname is the high half (see __sockopt)
  };

  u32 len;      // buffer size or number of iovecs

  union {      // offset 28: op-specific flags
    u32 rw_flags;
    u32 fsync_flags;
    u16 poll_events;      // compat; low half of poll32_events
    u32 poll32_events;
    u32 sync_range_flags;
    u32 msg_flags;
    u32 timeout_flags;
    u32 accept_flags;
    u32 cancel_flags;
    u32 open_flags;
    u32 statx_flags;
    u32 fadvise_advice;
    u32 splice_flags;
    u32 rename_flags;
    u32 unlink_flags;
    u32 hardlink_flags;
    u32 xattr_flags;
    u32 msg_ring_flags;
    u32 uring_cmd_flags;
    u32 waitid_flags;
    u32 futex_flags;
    u32 install_fd_flags;
    u32 nop_flags;
    u32 pipe_flags;
  };

  u64 user_data;

  union {               // offset 40
    u16 buf_index;      // fixed-buffer index
    u16 buf_group;      // grouped buffer selection
  };

  u16 personality;

  union {      // offset 44
    i32 splice_fd_in;
    u32 file_index;      // slot+1 (0 = none); file_index_alloc to auto-allocate
    u32 zcrx_ifq_idx;
    u32 optlen;
    u16 addr_len;         // high half stays zero
    u8 write_stream;      // high bytes stay zero
  };

  union {           // offset 48 (UAPI folds 48..63 into one 16-byte union; two consecutive
    u64 addr3;      //  8-byte unions produce the identical layout)
    u64 attr_ptr;
    u64 optval;
  };

  union {      // offset 56
    u64 __pad2;
    u64 attr_type_mask;
  };

  // sqe128 payload window: 16 bytes here, 80 bytes when the ring is setup_sqe128/sqe_mixed
  [[gnu::always_inline]] u8 *
  cmd() noexcept
  {
    return reinterpret_cast<u8 *>(&addr3);
  }
};

struct cqe {
  u64 user_data;
  i32 res;        // result or -errno
  u32 flags;      // cqe_f_* bits

  [[gnu::always_inline]] u64 *
  big_cqe() noexcept
  {
    return reinterpret_cast<u64 *>(this + 1);
  }

  [[gnu::always_inline]] const u64 *
  big_cqe() const noexcept
  {
    return reinterpret_cast<const u64 *>(this + 1);
  }
};

static_assert(sizeof(sqe) == 64, "io_uring_sqe ABI");
static_assert(__builtin_offsetof(sqe, fd) == 4 && __builtin_offsetof(sqe, off) == 8 && __builtin_offsetof(sqe, addr) == 16
                  && __builtin_offsetof(sqe, len) == 24 && __builtin_offsetof(sqe, rw_flags) == 28
                  && __builtin_offsetof(sqe, user_data) == 32 && __builtin_offsetof(sqe, buf_index) == 40
                  && __builtin_offsetof(sqe, personality) == 42 && __builtin_offsetof(sqe, file_index) == 44
                  && __builtin_offsetof(sqe, addr3) == 48,
              "io_uring_sqe ABI");
static_assert(__builtin_offsetof(sqe, addr2) == 8 && __builtin_offsetof(sqe, cmd_op) == 8 && __builtin_offsetof(sqe, splice_off_in) == 16
                  && __builtin_offsetof(sqe, level) == 16 && __builtin_offsetof(sqe, poll32_events) == 28
                  && __builtin_offsetof(sqe, buf_group) == 40 && __builtin_offsetof(sqe, splice_fd_in) == 44
                  && __builtin_offsetof(sqe, addr_len) == 44 && __builtin_offsetof(sqe, write_stream) == 44
                  && __builtin_offsetof(sqe, attr_ptr) == 48 && __builtin_offsetof(sqe, optval) == 48
                  && __builtin_offsetof(sqe, attr_type_mask) == 56 && __builtin_offsetof(sqe, __pad2) == 56,
              "io_uring_sqe union alias ABI");
static_assert(sizeof(cqe) == 16 && __builtin_offsetof(cqe, res) == 8 && __builtin_offsetof(cqe, flags) == 12, "io_uring_cqe ABI");

struct region_desc {      // >=6.13
  u64 user_addr;
  u64 size;
  u32 flags;      // mem_region_type_user
  u32 id;
  u64 mmap_offset;
  u64 __resv[4];
};

struct mem_region_reg {      // >=6.13
  u64 region_uptr;           // region_desc *
  u64 flags;                 // mem_region_reg_wait_arg
  u64 __resv[2];
};

struct rsrc_register {      // >=5.13
  u32 nr;
  u32 flags;      // rsrc_register_sparse
  u64 resv2;
  u64 data;
  u64 tags;
};

struct rsrc_update {      // >=5.13
  u32 offset;
  u32 resv;
  u64 data;
};

struct rsrc_update2 {      // >=5.13
  u32 offset;
  u32 resv;
  u64 data;
  u64 tags;
  u32 nr;
  u32 resv2;
};

struct files_update {      // deprecated; use rsrc_update
  u32 offset;
  u32 resv;
  u64 fds;      // i32 *
};

struct probe_op {
  u8 op;
  u8 resv;
  u16 flags;      // probe_op_supported
  u32 resv2;
};

struct probe {      // fixed capacity: the kernel copies min(ops_len, 256) entries
  static constexpr u32 max_ops = 256;

  u8 last_op;      // last opcode supported
  u8 ops_len;
  u16 resv;
  u32 resv2[3];
  probe_op ops[max_ops];

  [[nodiscard]] bool
  supported(u8 __op) const noexcept
  {
    return __op < ops_len && (ops[__op].flags & probe_op_supported) != 0;
  }
};

struct restriction {      // >=5.10
  u16 opcode;             // restriction_*
  u8 arg;                 // register_op / sqe_op / sqe_flags
  u8 resv;
  u32 resv2[3];
};

struct clock_register {      // >=6.12
  u32 clockid;
  u32 __resv[3];
};

struct clone_buffers {      // >=6.12
  u32 src_fd;
  u32 flags;      // clone_src_registered | clone_dst_replace
  u32 src_off;
  u32 dst_off;
  u32 nr;
  u32 pad[3];
};

struct buf {      // >=5.19
  u64 addr;
  u32 len;
  u16 bid;
  u16 resv;
};

struct buf_ring {      // >=5.19
  u64 __resv1;
  u32 __resv2;
  u16 __resv3;
  u16 tail;

  [[gnu::always_inline]] buf *
  bufs() noexcept
  {
    return reinterpret_cast<buf *>(this);
  }
};

struct buf_reg {      // >=5.19
  u64 ring_addr;
  u32 ring_entries;
  u16 bgid;
  u16 flags;      // pbuf_ring_mmap | pbuf_ring_inc
  u32 min_left;
  u32 resv[5];
};

struct buf_status {      // >=6.8
  u32 buf_group;         // input
  u32 head;              // output
  u32 resv[8];
};

struct napi {      // >=6.9
  u32 busy_poll_to;
  u8 prefer_busy_poll;
  u8 opcode;      // napi_register_op / napi_static_add_id / napi_static_del_id
  u8 pad[2];
  u32 op_param;      // tracking strategy or napi id
  u32 resv;
};

struct getevents_arg {      // >=5.11 (feat_ext_arg)
  u64 sigmask;
  u32 sigmask_sz;
  u32 min_wait_usec;      // >=6.12 (feat_min_timeout)
  u64 ts;                 // ktimespec *
};

struct sync_cancel_reg {      // >=6.0
  u64 addr;
  i32 fd;
  u32 flags;      // async_cancel_*
  ktimespec timeout;
  u8 opcode;      // >=6.8 (async_cancel_op)
  u8 pad[7];
  u64 pad2[3];
};

struct file_index_range {      // >=6.0
  u32 off;
  u32 len;
  u64 resv;
};

struct recvmsg_out {      // >=6.0 (recvmsg multishot payload header)
  u32 namelen;
  u32 controllen;
  u32 payloadlen;
  u32 flags;
};

struct reg_wait {      // >=6.13 (registered wait regions; enter_ext_arg_reg)
  ktimespec ts;
  u32 min_wait_usec;
  u32 flags;      // reg_wait_ts
  u64 sigmask;
  u32 sigmask_sz;
  u32 pad[3];
  u64 pad2[2];
};

struct attr_pi {      // >=6.13 (rw_attr_flag_pi payload)
  u16 flags;
  u16 app_tag;
  u32 len;
  u64 addr;
  u64 seed;
  u64 rsvd;
};

struct io_timespec {      // >=6.17 (tx timestamps)
  u64 tv_sec;
  u64 tv_nsec;
};

// zcrx family (>=6.15)
struct zcrx_rqe {
  u64 off;
  u32 len;
  u32 __pad;
};

struct zcrx_cqe {
  u64 off;
  u64 __pad;
};

struct zcrx_offsets {
  u32 head;
  u32 tail;
  u32 rqes;
  u32 __resv2;
  u64 __resv[2];
};

struct zcrx_area_reg {
  u64 addr;
  u64 len;
  u64 rq_area_token;
  u32 flags;      // zcrx_area_dmabuf
  u32 dmabuf_fd;
  u64 __resv2[2];
};

struct zcrx_ifq_reg {
  u32 if_idx;
  u32 if_rxq;
  u32 rq_entries;
  u32 flags;           // zcrx_reg_import | zcrx_reg_nodev
  u64 area_ptr;        // zcrx_area_reg *
  u64 region_ptr;      // region_desc *
  zcrx_offsets offsets;
  u32 zcrx_id;
  u32 rx_buf_len;
  u64 __resv[3];
};

struct zcrx_ctrl_export {      // >=6.18
  u32 zcrx_fd;
  u32 __resv1[11];
};

struct zcrx_ctrl_flush_rq {      // >=6.18
  u64 __resv[6];
};

struct zcrx_ctrl {      // >=6.18
  u32 zcrx_id;
  u32 op;      // zcrx_ctrl_op_flush_rq / zcrx_ctrl_op_export
  u64 __resv[2];

  union {
    zcrx_ctrl_export zc_export;
    zcrx_ctrl_flush_rq zc_flush;
  };
};

// query family (>=6.17)
struct query_hdr {
  u64 next_entry;
  u64 query_data;
  u32 query_op;      // query_opcodes / query_zcrx / query_scq
  u32 size;
  i32 result;
  u32 __resv[3];
};

struct query_opcode {
  u32 nr_request_opcodes;
  u32 nr_register_opcodes;
  u64 feature_flags;
  u64 ring_setup_flags;
  u64 enter_flags;
  u64 sqe_flags;
  u32 nr_query_opcodes;
  u32 __pad;
};

static_assert(sizeof(region_desc) == 64 && sizeof(mem_region_reg) == 32, "region ABI");
static_assert(sizeof(rsrc_register) == 32 && sizeof(rsrc_update) == 16 && sizeof(rsrc_update2) == 32, "rsrc ABI");
static_assert(sizeof(files_update) == 16 && sizeof(probe_op) == 8, "files_update/probe_op ABI");
static_assert(__builtin_offsetof(probe, ops) == 16, "io_uring_probe ABI");
static_assert(sizeof(restriction) == 16 && sizeof(clock_register) == 16 && sizeof(clone_buffers) == 32, "register ABI");
static_assert(sizeof(buf) == 16 && sizeof(buf_ring) == 16 && __builtin_offsetof(buf_ring, tail) == 14, "pbuf ABI");
static_assert(sizeof(buf_reg) == 40 && sizeof(buf_status) == 40 && sizeof(napi) == 16, "pbuf/napi ABI");
static_assert(sizeof(getevents_arg) == 24 && sizeof(sync_cancel_reg) == 64, "wait/cancel ABI");
static_assert(sizeof(file_index_range) == 16 && sizeof(recvmsg_out) == 16, "range/recvmsg ABI");
static_assert(sizeof(reg_wait) == 64 && sizeof(attr_pi) == 32 && sizeof(io_timespec) == 16, "wait/attr ABI");
static_assert(sizeof(zcrx_rqe) == 16 && sizeof(zcrx_cqe) == 16 && sizeof(zcrx_offsets) == 32, "zcrx ABI");
static_assert(sizeof(zcrx_area_reg) == 48 && sizeof(zcrx_ifq_reg) == 96 && sizeof(zcrx_ctrl) == 72, "zcrx ABI");
static_assert(sizeof(query_hdr) == 40 && sizeof(query_opcode) == 48, "query ABI");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// raw syscalls

inline long
__io_uring_setup(u32 entries, params *p) noexcept
{
  return micron::syscall(SYS_io_uring_setup, entries, p);
}

inline long
__io_uring_enter(i32 fd, u32 to_submit, u32 min_complete, u32 flags, void *argp, usize argsz) noexcept
{
  return micron::syscall(SYS_io_uring_enter, fd, to_submit, min_complete, flags, argp, argsz);
}

inline long
__io_uring_register(i32 fd, u32 opcode, void *arg, u32 nr_args) noexcept
{
  return micron::syscall(SYS_io_uring_register, fd, opcode, arg, nr_args);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sqe preparation
[[gnu::always_inline]] inline void
__prep(sqe *s, u8 op, i32 fd, u64 addr, u32 len, u64 off) noexcept
{
  *s = sqe{};
  s->opcode = op;
  s->fd = fd;
  s->addr = addr;
  s->len = len;
  s->off = off;
}

// sockopt packing: optname is the high half of the addr union
[[gnu::always_inline]] inline void
__sockopt(sqe *s, u32 level, u32 optname) noexcept
{
  s->addr = (static_cast<u64>(optname) << 32) | level;
}

[[gnu::always_inline]] inline void
sqe_set_flags(sqe *s, u8 flags) noexcept
{
  s->flags = flags;
}

[[gnu::always_inline]] inline void
sqe_set_buf_group(sqe *s, u16 bgid) noexcept
{
  s->buf_group = bgid;
  s->flags |= sqe_buffer_select;
}

[[gnu::always_inline]] inline void
sqe_set_personality(sqe *s, u16 pers) noexcept
{
  s->personality = pers;
}

[[gnu::always_inline]] inline void
sqe_set_fixed_file(sqe *s) noexcept
{
  s->flags |= sqe_fixed_file;
}

[[gnu::always_inline]] inline void
sqe_set_file_index(sqe *s, u32 slot) noexcept
{
  s->file_index = slot == file_index_alloc ? file_index_alloc : slot + 1;
}

[[gnu::always_inline]] inline u16
cqe_buffer_id(u32 cqe_flags) noexcept
{
  return static_cast<u16>(cqe_flags >> cqe_buffer_shift);
}

[[gnu::always_inline]] inline void
prep_nop(sqe *s) noexcept
{
  __prep(s, op_nop, -1, 0, 0, 0);
}

[[gnu::always_inline]] inline void
prep_read(sqe *s, i32 fd, void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  __prep(s, op_read, fd, reinterpret_cast<u64>(buf), n, off);      // off==-1: use the file position
}

[[gnu::always_inline]] inline void
prep_write(sqe *s, i32 fd, const void *buf, u32 n, u64 off = static_cast<u64>(-1)) noexcept
{
  __prep(s, op_write, fd, reinterpret_cast<u64>(buf), n, off);
}

[[gnu::always_inline]] inline void
prep_readv(sqe *s, i32 fd, const iovec *iov, u32 nr, u64 off = static_cast<u64>(-1), u32 rw_flags = 0) noexcept
{
  __prep(s, op_readv, fd, reinterpret_cast<u64>(iov), nr, off);
  s->rw_flags = rw_flags;      // RWF_* flags
}

[[gnu::always_inline]] inline void
prep_writev(sqe *s, i32 fd, const iovec *iov, u32 nr, u64 off = static_cast<u64>(-1), u32 rw_flags = 0) noexcept
{
  __prep(s, op_writev, fd, reinterpret_cast<u64>(iov), nr, off);
  s->rw_flags = rw_flags;
}

[[gnu::always_inline]] inline void
prep_read_fixed(sqe *s, i32 fd, void *buf, u32 n, u64 off, u16 buf_index) noexcept
{
  __prep(s, op_read_fixed, fd, reinterpret_cast<u64>(buf), n, off);
  s->buf_index = buf_index;
}

[[gnu::always_inline]] inline void
prep_write_fixed(sqe *s, i32 fd, const void *buf, u32 n, u64 off, u16 buf_index) noexcept
{
  __prep(s, op_write_fixed, fd, reinterpret_cast<u64>(buf), n, off);
  s->buf_index = buf_index;
}

// >=6.15
[[gnu::always_inline]] inline void
prep_readv_fixed(sqe *s, i32 fd, const iovec *iov, u32 nr, u64 off, u16 buf_index, u32 rw_flags = 0) noexcept
{
  __prep(s, op_readv_fixed, fd, reinterpret_cast<u64>(iov), nr, off);
  s->buf_index = buf_index;
  s->rw_flags = rw_flags;
}

// >=6.15
[[gnu::always_inline]] inline void
prep_writev_fixed(sqe *s, i32 fd, const iovec *iov, u32 nr, u64 off, u16 buf_index, u32 rw_flags = 0) noexcept
{
  __prep(s, op_writev_fixed, fd, reinterpret_cast<u64>(iov), nr, off);
  s->buf_index = buf_index;
  s->rw_flags = rw_flags;
}

// >=6.7; multishot read into a provided-buffer group (pollable fds)
[[gnu::always_inline]] inline void
prep_read_multishot(sqe *s, i32 fd, u16 bgid, u64 off = static_cast<u64>(-1)) noexcept
{
  __prep(s, op_read_multishot, fd, 0, 0, off);
  sqe_set_buf_group(s, bgid);
}

[[gnu::always_inline]] inline void
prep_fsync(sqe *s, i32 fd, u32 fsync_flags = 0) noexcept
{
  __prep(s, op_fsync, fd, 0, 0, 0);
  s->fsync_flags = fsync_flags;      // fsync_datasync
}

[[gnu::always_inline]] inline void
prep_sync_file_range(sqe *s, i32 fd, u32 nbytes, u64 off, u32 range_flags = 0) noexcept
{
  __prep(s, op_sync_file_range, fd, 0, nbytes, off);
  s->sync_range_flags = range_flags;
}

// >=5.6
[[gnu::always_inline]] inline void
prep_fallocate(sqe *s, i32 fd, i32 mode, u64 off, u64 len) noexcept
{
  __prep(s, op_fallocate, fd, len, static_cast<u32>(mode), off);
}

// >=6.9
[[gnu::always_inline]] inline void
prep_ftruncate(sqe *s, i32 fd, u64 len) noexcept
{
  __prep(s, op_ftruncate, fd, 0, 0, len);
}

// >=5.6
[[gnu::always_inline]] inline void
prep_fadvise(sqe *s, i32 fd, u64 off, u32 len, i32 advice) noexcept
{
  __prep(s, op_fadvise, fd, 0, len, off);
  s->fadvise_advice = static_cast<u32>(advice);
}

// >=5.6
[[gnu::always_inline]] inline void
prep_madvise(sqe *s, void *addr, u32 len, i32 advice) noexcept
{
  __prep(s, op_madvise, -1, reinterpret_cast<u64>(addr), len, 0);
  s->fadvise_advice = static_cast<u32>(advice);
}

// >=5.6; statx_buf: posix::statx_t * (opaque here)
[[gnu::always_inline]] inline void
prep_statx(sqe *s, i32 dirfd, const char *path, u32 statx_flags, u32 mask, void *statx_buf) noexcept
{
  __prep(s, op_statx, dirfd, reinterpret_cast<u64>(path), mask, reinterpret_cast<u64>(statx_buf));
  s->statx_flags = statx_flags;      // AT_* flags
}

[[gnu::always_inline]] inline void
prep_openat(sqe *s, i32 dirfd, const char *path, u32 open_flags, u32 mode) noexcept
{
  __prep(s, op_openat, dirfd, reinterpret_cast<u64>(path), mode, 0);
  s->open_flags = open_flags;
}

// >=5.15 (slot form) / >=5.19 (alloc): open straight into a fixed-file slot
[[gnu::always_inline]] inline void
prep_openat_direct(sqe *s, i32 dirfd, const char *path, u32 open_flags, u32 mode, u32 slot = file_index_alloc) noexcept
{
  prep_openat(s, dirfd, path, open_flags, mode);
  sqe_set_file_index(s, slot);
}

// >=5.6; how: posix::open_how * (opaque here), how_len = sizeof(open_how) = 24
[[gnu::always_inline]] inline void
prep_openat2(sqe *s, i32 dirfd, const char *path, const void *how, u32 how_len = 24) noexcept
{
  __prep(s, op_openat2, dirfd, reinterpret_cast<u64>(path), how_len, reinterpret_cast<u64>(how));
}

[[gnu::always_inline]] inline void
prep_openat2_direct(sqe *s, i32 dirfd, const char *path, const void *how, u32 how_len = 24, u32 slot = file_index_alloc) noexcept
{
  prep_openat2(s, dirfd, path, how, how_len);
  sqe_set_file_index(s, slot);
}

[[gnu::always_inline]] inline void
prep_close(sqe *s, i32 fd) noexcept
{
  __prep(s, op_close, fd, 0, 0, 0);
}

// >=5.15; close a fixed-file slot instead of an fd
[[gnu::always_inline]] inline void
prep_close_direct(sqe *s, u32 slot) noexcept
{
  __prep(s, op_close, 0, 0, 0, 0);
  sqe_set_file_index(s, slot);
}

// >=6.8; install a fixed-file slot as a regular fd (returned in cqe.res)
[[gnu::always_inline]] inline void
prep_fixed_fd_install(sqe *s, u32 slot, u32 install_flags = 0) noexcept
{
  __prep(s, op_fixed_fd_install, static_cast<i32>(slot), 0, 0, 0);
  s->flags = sqe_fixed_file;
  s->install_fd_flags = install_flags;      // fixed_fd_no_cloexec
}

// >=5.6; register/update fixed files from the sq (fds may contain register_files_skip)
[[gnu::always_inline]] inline void
prep_files_update(sqe *s, const i32 *fds, u32 nr, u32 offset) noexcept
{
  __prep(s, op_files_update, -1, reinterpret_cast<u64>(fds), nr, offset);
}

// >=5.11
[[gnu::always_inline]] inline void
prep_renameat(sqe *s, i32 olddirfd, const char *oldpath, i32 newdirfd, const char *newpath, u32 rename_flags = 0) noexcept
{
  __prep(s, op_renameat, olddirfd, reinterpret_cast<u64>(oldpath), static_cast<u32>(newdirfd), reinterpret_cast<u64>(newpath));
  s->rename_flags = rename_flags;
}

// >=5.11
[[gnu::always_inline]] inline void
prep_unlinkat(sqe *s, i32 dirfd, const char *path, u32 unlink_flags = 0) noexcept
{
  __prep(s, op_unlinkat, dirfd, reinterpret_cast<u64>(path), 0, 0);
  s->unlink_flags = unlink_flags;      // at_removedir
}

// >=5.15
[[gnu::always_inline]] inline void
prep_mkdirat(sqe *s, i32 dirfd, const char *path, u32 mode) noexcept
{
  __prep(s, op_mkdirat, dirfd, reinterpret_cast<u64>(path), mode, 0);
}

// >=5.15
[[gnu::always_inline]] inline void
prep_symlinkat(sqe *s, const char *target, i32 newdirfd, const char *linkpath) noexcept
{
  __prep(s, op_symlinkat, newdirfd, reinterpret_cast<u64>(target), 0, reinterpret_cast<u64>(linkpath));
}

// >=5.15
[[gnu::always_inline]] inline void
prep_linkat(sqe *s, i32 olddirfd, const char *oldpath, i32 newdirfd, const char *newpath, u32 hardlink_flags = 0) noexcept
{
  __prep(s, op_linkat, olddirfd, reinterpret_cast<u64>(oldpath), static_cast<u32>(newdirfd), reinterpret_cast<u64>(newpath));
  s->hardlink_flags = hardlink_flags;
}

[[gnu::always_inline]] inline void
prep_getxattr(sqe *s, const char *name, void *value, const char *path, u32 len) noexcept
{
  __prep(s, op_getxattr, 0, reinterpret_cast<u64>(name), len, reinterpret_cast<u64>(value));
  s->addr3 = reinterpret_cast<u64>(path);
}

[[gnu::always_inline]] inline void
prep_setxattr(sqe *s, const char *name, const void *value, const char *path, u32 len, u32 xattr_flags = 0) noexcept
{
  __prep(s, op_setxattr, 0, reinterpret_cast<u64>(name), len, reinterpret_cast<u64>(value));
  s->addr3 = reinterpret_cast<u64>(path);
  s->xattr_flags = xattr_flags;
}

[[gnu::always_inline]] inline void
prep_fgetxattr(sqe *s, i32 fd, const char *name, void *value, u32 len) noexcept
{
  __prep(s, op_fgetxattr, fd, reinterpret_cast<u64>(name), len, reinterpret_cast<u64>(value));
}

[[gnu::always_inline]] inline void
prep_fsetxattr(sqe *s, i32 fd, const char *name, const void *value, u32 len, u32 xattr_flags = 0) noexcept
{
  __prep(s, op_fsetxattr, fd, reinterpret_cast<u64>(name), len, reinterpret_cast<u64>(value));
  s->xattr_flags = xattr_flags;
}

// >=5.7; fd_in/off_in read side, fd_out/off_out write side; off==-1 means "use the pipe position"
[[gnu::always_inline]] inline void
prep_splice(sqe *s, i32 fd_in, u64 off_in, i32 fd_out, u64 off_out, u32 nbytes, u32 splice_flags = 0) noexcept
{
  __prep(s, op_splice, fd_out, 0, nbytes, off_out);
  s->splice_off_in = off_in;
  s->splice_fd_in = fd_in;
  s->splice_flags = splice_flags;      // splice_f_fd_in_fixed for a fixed-file fd_in
}

// >=5.8
[[gnu::always_inline]] inline void
prep_tee(sqe *s, i32 fd_in, i32 fd_out, u32 nbytes, u32 splice_flags = 0) noexcept
{
  __prep(s, op_tee, fd_out, 0, nbytes, 0);
  s->splice_off_in = 0;
  s->splice_fd_in = fd_in;
  s->splice_flags = splice_flags;
}

// >=6.16; fds[2] filled on completion
[[gnu::always_inline]] inline void
prep_pipe(sqe *s, i32 *fds, u32 pipe_flags = 0) noexcept
{
  __prep(s, op_pipe, -1, reinterpret_cast<u64>(fds), 0, 0);
  s->pipe_flags = pipe_flags;      // O_* pipe2 flags
}

// %%%% provided buffers (legacy op form, >=5.7)

[[gnu::always_inline]] inline void
prep_provide_buffers(sqe *s, void *addr, u32 buf_len, u32 nbufs, u16 bgid, u32 bid) noexcept
{
  __prep(s, op_provide_buffers, static_cast<i32>(nbufs), reinterpret_cast<u64>(addr), buf_len, bid);
  s->buf_group = bgid;
}

[[gnu::always_inline]] inline void
prep_remove_buffers(sqe *s, u32 nbufs, u16 bgid) noexcept
{
  __prep(s, op_remove_buffers, static_cast<i32>(nbufs), 0, 0, 0);
  s->buf_group = bgid;
}

[[gnu::always_inline]] inline void
prep_accept(sqe *s, i32 fd, void *addr, u32 *addrlen, u32 accept_flags = 0) noexcept
{
  __prep(s, op_accept, fd, reinterpret_cast<u64>(addr), 0, reinterpret_cast<u64>(addrlen));
  s->accept_flags = accept_flags;      // SOCK_* flags
}

// >=5.19
[[gnu::always_inline]] inline void
prep_accept_multishot(sqe *s, i32 fd, void *addr, u32 *addrlen, u32 accept_flags = 0) noexcept
{
  prep_accept(s, fd, addr, addrlen, accept_flags);
  s->ioprio |= accept_multishot;
}

// >=5.19
[[gnu::always_inline]] inline void
prep_accept_direct(sqe *s, i32 fd, void *addr, u32 *addrlen, u32 accept_flags = 0, u32 slot = file_index_alloc) noexcept
{
  prep_accept(s, fd, addr, addrlen, accept_flags);
  sqe_set_file_index(s, slot);
}

[[gnu::always_inline]] inline void
prep_connect(sqe *s, i32 fd, const void *addr, u32 addrlen) noexcept
{
  __prep(s, op_connect, fd, reinterpret_cast<u64>(addr), 0, addrlen);
}

// >=6.11
[[gnu::always_inline]] inline void
prep_bind(sqe *s, i32 fd, const void *addr, u32 addrlen) noexcept
{
  __prep(s, op_bind, fd, reinterpret_cast<u64>(addr), 0, addrlen);
}

// >=6.11
[[gnu::always_inline]] inline void
prep_listen(sqe *s, i32 fd, u32 backlog) noexcept
{
  __prep(s, op_listen, fd, 0, backlog, 0);
}

// >=5.19
[[gnu::always_inline]] inline void
prep_socket(sqe *s, i32 domain, i32 type, i32 protocol, u32 sock_flags = 0) noexcept
{
  __prep(s, op_socket, domain, 0, static_cast<u32>(protocol), static_cast<u64>(static_cast<u32>(type)));
  s->rw_flags = sock_flags;
}

// >=5.19
[[gnu::always_inline]] inline void
prep_socket_direct(sqe *s, i32 domain, i32 type, i32 protocol, u32 sock_flags = 0, u32 slot = file_index_alloc) noexcept
{
  prep_socket(s, domain, type, protocol, sock_flags);
  sqe_set_file_index(s, slot);
}

[[gnu::always_inline]] inline void
prep_send(sqe *s, i32 fd, const void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  __prep(s, op_send, fd, reinterpret_cast<u64>(buf), n, 0);
  s->msg_flags = msg_flags;
}

[[gnu::always_inline]] inline void
prep_recv(sqe *s, i32 fd, void *buf, u32 n, u32 msg_flags = 0) noexcept
{
  __prep(s, op_recv, fd, reinterpret_cast<u64>(buf), n, 0);
  s->msg_flags = msg_flags;
}

// >=6.0; multishot recv into a provided-buffer group
[[gnu::always_inline]] inline void
prep_recv_multishot(sqe *s, i32 fd, u16 bgid, u32 msg_flags = 0) noexcept
{
  prep_recv(s, fd, nullptr, 0, msg_flags);
  s->ioprio |= recv_multishot;
  sqe_set_buf_group(s, bgid);
}

// msg: posix msghdr * (opaque here)
[[gnu::always_inline]] inline void
prep_sendmsg(sqe *s, i32 fd, const void *msg, u32 msg_flags = 0) noexcept
{
  __prep(s, op_sendmsg, fd, reinterpret_cast<u64>(msg), 1, 0);
  s->msg_flags = msg_flags;
}

[[gnu::always_inline]] inline void
prep_recvmsg(sqe *s, i32 fd, void *msg, u32 msg_flags = 0) noexcept
{
  __prep(s, op_recvmsg, fd, reinterpret_cast<u64>(msg), 1, 0);
  s->msg_flags = msg_flags;
}

// >=6.0
[[gnu::always_inline]] inline void
prep_recvmsg_multishot(sqe *s, i32 fd, void *msg, u16 bgid, u32 msg_flags = 0) noexcept
{
  prep_recvmsg(s, fd, msg, msg_flags);
  s->ioprio |= recv_multishot;
  sqe_set_buf_group(s, bgid);
}

// >=6.0; zerocopy send: completion cqe + a cqe_f_notif cqe when the buffer is reusable
[[gnu::always_inline]] inline void
prep_send_zc(sqe *s, i32 fd, const void *buf, u32 n, u32 msg_flags = 0, u16 zc_flags = 0) noexcept
{
  __prep(s, op_send_zc, fd, reinterpret_cast<u64>(buf), n, 0);
  s->msg_flags = msg_flags;
  s->ioprio = zc_flags;      // recvsend_* bits
}

// >=6.0
[[gnu::always_inline]] inline void
prep_send_zc_fixed(sqe *s, i32 fd, const void *buf, u32 n, u16 buf_index, u32 msg_flags = 0, u16 zc_flags = 0) noexcept
{
  prep_send_zc(s, fd, buf, n, msg_flags, static_cast<u16>(zc_flags | recvsend_fixed_buf));
  s->buf_index = buf_index;
}

// >=6.1
[[gnu::always_inline]] inline void
prep_sendmsg_zc(sqe *s, i32 fd, const void *msg, u32 msg_flags = 0) noexcept
{
  prep_sendmsg(s, fd, msg, msg_flags);
  s->opcode = op_sendmsg_zc;
}

[[gnu::always_inline]] inline void
prep_shutdown(sqe *s, i32 fd, i32 how) noexcept
{
  __prep(s, op_shutdown, fd, 0, static_cast<u32>(how), 0);
}

// >=5.6
[[gnu::always_inline]] inline void
prep_epoll_ctl(sqe *s, i32 epfd, i32 fd, i32 op, void *ev) noexcept
{
  __prep(s, op_epoll_ctl, epfd, reinterpret_cast<u64>(ev), static_cast<u32>(op), static_cast<u64>(static_cast<u32>(fd)));
}

// >=6.15
[[gnu::always_inline]] inline void
prep_epoll_wait(sqe *s, i32 epfd, void *events, u32 maxevents, u32 wait_flags = 0) noexcept
{
  __prep(s, op_epoll_wait, epfd, reinterpret_cast<u64>(events), maxevents, 0);
  s->rw_flags = wait_flags;
}

// %%%% poll

[[gnu::always_inline]] inline void
prep_poll_add(sqe *s, i32 fd, u32 poll_mask) noexcept
{
  __prep(s, op_poll_add, fd, 0, 0, 0);
  s->poll32_events = poll_mask;      // (little-endian layout on all micron targets)
}

// >=5.13; keeps posting cqe_f_more completions until removed/error
[[gnu::always_inline]] inline void
prep_poll_multishot(sqe *s, i32 fd, u32 poll_mask) noexcept
{
  prep_poll_add(s, fd, poll_mask);
  s->len = poll_add_multi;
}

[[gnu::always_inline]] inline void
prep_poll_remove(sqe *s, u64 target_user_data) noexcept
{
  __prep(s, op_poll_remove, -1, target_user_data, 0, 0);
}

// >=5.13
[[gnu::always_inline]] inline void
prep_poll_update(sqe *s, u64 old_user_data, u64 new_user_data, u32 poll_mask, u32 update_flags) noexcept
{
  __prep(s, op_poll_remove, -1, old_user_data, update_flags, new_user_data);
  s->poll32_events = poll_mask;
}

[[gnu::always_inline]] inline void
prep_timeout(sqe *s, const ktimespec *ts, u32 timeout_flags = 0, u32 wait_nr = 0) noexcept
{
  __prep(s, op_timeout, -1, reinterpret_cast<u64>(ts), 1, wait_nr);      // off = completion count trigger
  s->timeout_flags = timeout_flags;                                      // timeout_abs -> ts is a CLOCK_MONOTONIC deadline
}

[[gnu::always_inline]] inline void
prep_timeout_remove(sqe *s, u64 target_user_data) noexcept
{
  __prep(s, op_timeout_remove, -1, target_user_data, 0, 0);
}

// >=5.11
[[gnu::always_inline]] inline void
prep_timeout_update(sqe *s, const ktimespec *ts, u64 target_user_data, u32 timeout_flags = 0) noexcept
{
  __prep(s, op_timeout_remove, -1, target_user_data, 0, reinterpret_cast<u64>(ts));
  s->timeout_flags = timeout_flags | timeout_update;
}

// links a timeout to the PREVIOUS sqe (which must carry sqe_io_link)
[[gnu::always_inline]] inline void
prep_link_timeout(sqe *s, const ktimespec *ts, u32 timeout_flags = 0) noexcept
{
  __prep(s, op_link_timeout, -1, reinterpret_cast<u64>(ts), 1, 0);
  s->timeout_flags = timeout_flags;
}

[[gnu::always_inline]] inline void
prep_cancel(sqe *s, u64 target_user_data, u32 cancel_flags = 0) noexcept
{
  __prep(s, op_async_cancel, -1, target_user_data, 0, 0);
  s->cancel_flags = cancel_flags;
}

// >=5.19; cancel by fd (async_cancel_fd implied)
[[gnu::always_inline]] inline void
prep_cancel_fd(sqe *s, i32 fd, u32 cancel_flags = 0) noexcept
{
  __prep(s, op_async_cancel, fd, 0, 0, 0);
  s->cancel_flags = cancel_flags | async_cancel_fd;
}

[[gnu::always_inline]] inline void
prep_msg_ring(sqe *s, i32 target_ring_fd, u32 res, u64 data, u32 ring_flags = 0) noexcept
{
  __prep(s, op_msg_ring, target_ring_fd, msg_data, res, data);
  s->msg_ring_flags = ring_flags;      // msg_ring_cqe_skip
}

// >=6.3; also pass cqe flags through to the target (msg_ring_flags_pass)
[[gnu::always_inline]] inline void
prep_msg_ring_cqe_flags(sqe *s, i32 target_ring_fd, u32 res, u64 data, u32 cqe_flags, u32 ring_flags = 0) noexcept
{
  prep_msg_ring(s, target_ring_fd, res, data, ring_flags | msg_ring_flags_pass);
  s->file_index = cqe_flags;
}

// >=6.0; pass a registered fd (source slot) into the target ring's file table
[[gnu::always_inline]] inline void
prep_msg_ring_fd(sqe *s, i32 target_ring_fd, u32 source_slot, u32 target_slot, u64 data, u32 ring_flags = 0) noexcept
{
  __prep(s, op_msg_ring, target_ring_fd, msg_send_fd, 0, data);
  s->addr3 = source_slot;
  sqe_set_file_index(s, target_slot);
  s->msg_ring_flags = ring_flags;
}

// >=5.19; payload via s->cmd(): 16 bytes, or 80 on sqe128/sqe_mixed rings
[[gnu::always_inline]] inline void
prep_uring_cmd(sqe *s, i32 fd, u32 cmd_op, u32 cmd_flags = 0) noexcept
{
  *s = sqe{};
  s->opcode = op_uring_cmd;
  s->fd = fd;
  s->cmd_op = cmd_op;
  s->uring_cmd_flags = cmd_flags;      // uring_cmd_fixed (+ buf_index)
}

// >=6.7; socket-level uring_cmd (get/setsockopt through the ring)
[[gnu::always_inline]] inline void
prep_cmd_sock(sqe *s, u32 sock_op, i32 fd, u32 level, u32 optname, void *optval, u32 optlen) noexcept
{
  prep_uring_cmd(s, fd, sock_op);
  __sockopt(s, level, optname);
  s->optval = reinterpret_cast<u64>(optval);
  s->optlen = optlen;
}

// >=6.7; waitid without blocking a thread; infop: posix siginfo_t * (opaque)
[[gnu::always_inline]] inline void
prep_waitid(sqe *s, i32 idtype, i32 id, void *infop, u32 options, u32 waitid_flags = 0) noexcept
{
  __prep(s, op_waitid, id, 0, static_cast<u32>(idtype), 0);
  s->waitid_flags = waitid_flags;
  s->file_index = options;      // WEXITED etc.
  s->addr2 = reinterpret_cast<u64>(infop);
}

[[gnu::always_inline]] inline void
prep_futex_wait(sqe *s, u32 *word, u32 expected, u64 mask = futex_match_any) noexcept
{
  __prep(s, op_futex_wait, static_cast<i32>(futex2_size_u32 | futex2_private), reinterpret_cast<u64>(word), 0, expected);
  s->addr3 = mask;
}

[[gnu::always_inline]] inline void
prep_futex_wake(sqe *s, u32 *word, u32 nwake, u64 mask = futex_match_any) noexcept
{
  __prep(s, op_futex_wake, static_cast<i32>(futex2_size_u32 | futex2_private), reinterpret_cast<u64>(word), 0, nwake);
  s->addr3 = mask;
}

// waitv: struct futex_waitv array (opaque here; <linux/futex.h> layout)
[[gnu::always_inline]] inline void
prep_futex_waitv(sqe *s, void *futexv, u32 nr_futex, u32 futex_flags = 0) noexcept
{
  __prep(s, op_futex_waitv, 0, reinterpret_cast<u64>(futexv), nr_futex, 0);
  s->futex_flags = futex_flags;
}

// 128-byte ops (>=7.1)

[[gnu::always_inline]] inline void
prep_nop128(sqe *s) noexcept
{
  __prep(s, op_nop128, -1, 0, 0, 0);
}

[[gnu::always_inline]] inline void
prep_uring_cmd128(sqe *s, i32 fd, u32 cmd_op, u32 cmd_flags = 0) noexcept
{
  prep_uring_cmd(s, fd, cmd_op, cmd_flags);
  s->opcode = op_uring_cmd128;
}

struct bufring {
  buf_ring *br = nullptr;
  u32 entries = 0;      // power of two

  [[nodiscard]] u32
  mask() const noexcept
  {
    return entries - 1;
  }

  [[gnu::always_inline]] void
  add(void *a, u32 len, u16 bid, u32 offset = 0) noexcept
  {
    buf *b = &br->bufs()[(br->tail + offset) & mask()];
    b->addr = reinterpret_cast<u64>(a);
    b->len = len;
    b->bid = bid;
  }

  [[gnu::always_inline]] void
  advance(u32 n) noexcept
  {
    atom::store(&br->tail, static_cast<u16>(br->tail + n), __ATOMIC_RELEASE);
  }
};

constexpr u32
setup_mask_for_kernel(u32 maj, u32 min) noexcept
{
  const u32 v = (maj << 8) | min;
  u32 mask = setup_iopoll | setup_sqpoll | setup_sq_aff | setup_cqsize | setup_clamp | setup_attach_wq;      // 5.1-5.6 era
  if ( v >= ((5u << 8) | 10) ) mask |= setup_r_disabled;
  if ( v >= ((5u << 8) | 18) ) mask |= setup_submit_all;
  if ( v >= ((5u << 8) | 19) ) mask |= setup_coop_taskrun | setup_taskrun_flag | setup_sqe128 | setup_cqe32;
  if ( v >= ((6u << 8) | 0) ) mask |= setup_single_issuer;
  if ( v >= ((6u << 8) | 1) ) mask |= setup_defer_taskrun;
  if ( v >= ((6u << 8) | 5) ) mask |= setup_no_mmap | setup_registered_fd_only;
  if ( v >= ((6u << 8) | 6) ) mask |= setup_no_sqarray;
  if ( v >= ((6u << 8) | 13) ) mask |= setup_hybrid_iopoll;
  if ( v >= ((6u << 8) | 18) ) mask |= setup_cqe_mixed;
  if ( v >= ((7u << 8) | 1) ) mask |= setup_sqe_mixed | setup_sq_rewind;
  return mask;
}

struct ring {
  i32 fd = -1;
  u32 features = 0;
  u32 setup_flags = 0;      // kernel-echoed p.flags
  u32 to_submit = 0;        // unsubmitted sqe count; accessed atomically (see advance_sq/enter) - a parked sentinel may enter() lock-free
  i32 enter_fd = -1;        // fd, or the registered ring index after register_ring_fd()
  u32 *sq_head = nullptr;
  u32 *sq_tail = nullptr;
  u32 *sq_mask = nullptr;
  u32 *sq_array = nullptr;      // nullptr under setup_no_sqarray
  u32 *sq_flags = nullptr;      // sq_need_wakeup / sq_cq_overflow / sq_taskrun
  u32 *sq_dropped = nullptr;
  u32 *cq_head = nullptr;
  u32 *cq_tail = nullptr;
  u32 *cq_mask = nullptr;
  u32 *cq_overflow = nullptr;
  u32 *cq_flags = nullptr;
  sqe *sqes = nullptr;
  cqe *cqes = nullptr;
  byte *__sq_ptr = nullptr;
  byte *__cq_ptr = nullptr;
  usize __sq_len = 0, __cq_len = 0, __sqes_len = 0;
  u32 __sq_entries = 0;
  u32 __cq_entries = 0;
  u8 __sqe_shift = 0;        // 1 on setup_sqe128 rings only (sqes stride in 64B units)
  u8 __cqe_shift = 0;        // 1 on setup_cqe32 rings (cqes stride in 16B units)
  u8 __enter_flags = 0;      // enter_registered_ring once the ring fd is registered
  bool __sqpoll = false;
  bool __cqe_mixed = false;
  bool __probed = false;
  u64 __op_bits[4]{};      // supports() cache from register_probe

  ring() noexcept = default;
  ring(const ring &) = delete;
  ring &operator=(const ring &) = delete;
  ring(ring &&) = delete;
  ring &operator=(ring &&) = delete;

  ~ring() { shutdown(); }

  [[gnu::always_inline]] static bool
  __map_failed(const void *p) noexcept
  {
    return reinterpret_cast<usize>(p) >= static_cast<usize>(-4095);      // raw mmap returns -errno
  }

  int
  init(u32 entries, params &p) noexcept
  {
    long r = __io_uring_setup(entries, &p);
    if ( r < 0 ) return static_cast<int>(r);
    fd = static_cast<i32>(r);
    enter_fd = fd;
    __enter_flags = 0;
    features = p.features;
    setup_flags = p.flags;
    __sq_entries = p.sq_entries;
    __cq_entries = p.cq_entries;
    __sqe_shift = (p.flags & setup_sqe128) != 0 ? 1 : 0;
    __cqe_shift = (p.flags & setup_cqe32) != 0 ? 1 : 0;
    __cqe_mixed = (p.flags & setup_cqe_mixed) != 0;
    __sqpoll = (p.flags & setup_sqpoll) != 0;

    if ( (p.flags & setup_no_sqarray) != 0 && (features & feat_single_mmap) == 0 ) {      // never happens on real kernels
      __close_fd();
      return -22;      // EINVAL
    }
    __sq_len = p.sq_off.array + p.sq_entries * sizeof(u32);
    __cq_len = p.cq_off.cqes + ((static_cast<usize>(p.cq_entries) * sizeof(cqe)) << __cqe_shift);
    if ( features & feat_single_mmap ) {
      if ( __cq_len > __sq_len ) __sq_len = __cq_len;
      __cq_len = __sq_len;
    }
    __sq_ptr = reinterpret_cast<byte *>(
        micron::mmap(nullptr, __sq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sq_ring)));
    if ( __map_failed(__sq_ptr) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(__sq_ptr));
      __sq_ptr = nullptr;
      __close_fd();
      return __e;
    }
    if ( features & feat_single_mmap ) {
      __cq_ptr = __sq_ptr;
    } else {
      __cq_ptr = reinterpret_cast<byte *>(
          micron::mmap(nullptr, __cq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_cq_ring)));
      if ( __map_failed(__cq_ptr) ) {
        int __e = static_cast<int>(reinterpret_cast<i64>(__cq_ptr));
        __cq_ptr = nullptr;
        shutdown();
        return __e;
      }
    }
    __sqes_len = (static_cast<usize>(p.sq_entries) * sizeof(sqe)) << __sqe_shift;
    sqes = reinterpret_cast<sqe *>(
        micron::mmap(nullptr, __sqes_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sqes)));
    if ( __map_failed(sqes) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(sqes));
      sqes = nullptr;
      shutdown();
      return __e;
    }

    __wire(p);
    return 0;
  }

  int
  init(u32 entries, u32 flags = 0) noexcept
  {
    params p{};
    p.flags = flags;
    return init(entries, p);
  }

  static constexpr u32 __strip_order[]
      = { setup_sq_rewind,    setup_sqe_mixed,     setup_cqe_mixed,    setup_hybrid_iopoll, setup_no_sqarray, setup_registered_fd_only,
          setup_no_mmap,      setup_defer_taskrun, setup_taskrun_flag, setup_single_issuer, setup_cqe32,      setup_sqe128,
          setup_coop_taskrun, setup_submit_all,    setup_r_disabled,   setup_attach_wq,     setup_clamp,      setup_cqsize };

  int
  init_best(u32 entries, params &p) noexcept
  {
    const params __saved = p;
    u32 f = p.flags;
    for ( ;; ) {
      p = __saved;
      p.flags = f;
      int r = init(entries, p);
      if ( r != -22 /*EINVAL*/ ) return r;
      usize k = 0;
      for ( ; k < sizeof(__strip_order) / sizeof(__strip_order[0]); k++ )
        if ( (f & __strip_order[k]) != 0 ) {
          f &= ~__strip_order[k];
          break;
        }
      if ( k == sizeof(__strip_order) / sizeof(__strip_order[0]) ) return r;      // nothing left to strip
    }
  }

  int
  init_best(u32 entries, u32 want_flags) noexcept
  {
    params p{};
    p.flags = want_flags;
    return init_best(entries, p);
  }

  [[nodiscard]] bool
  live() const noexcept
  {
    return fd >= 0;
  }

  void
  shutdown() noexcept
  {
    if ( sqes != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(sqes), __sqes_len);
    if ( __cq_ptr != nullptr && __cq_ptr != __sq_ptr ) micron::munmap(reinterpret_cast<addr_t *>(__cq_ptr), __cq_len);
    if ( __sq_ptr != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(__sq_ptr), __sq_len);
    sqes = nullptr;
    __sq_ptr = nullptr;
    __cq_ptr = nullptr;
    __close_fd();
  }

  [[nodiscard]] sqe *
  get_sqe() noexcept
  {
    const u32 __t = *sq_tail;      // single preparer: plain read of our own tail
    if ( __t - atom::load(sq_head, __ATOMIC_ACQUIRE) >= __sq_entries ) return nullptr;
    const u32 __i = __t & *sq_mask;
    if ( sq_array != nullptr ) sq_array[__i] = __i;
    return &sqes[static_cast<usize>(__i) << __sqe_shift];
  }

  [[nodiscard]] sqe *
  peek_sqe(u32 k) noexcept
  {
    const u32 __t = *sq_tail + k;
    if ( __t - atom::load(sq_head, __ATOMIC_ACQUIRE) >= __sq_entries ) return nullptr;
    const u32 __i = __t & *sq_mask;
    if ( sq_array != nullptr ) sq_array[__i] = __i;
    return &sqes[static_cast<usize>(__i) << __sqe_shift];
  }

  [[nodiscard]] sqe *
  get_sqe128() noexcept
  {
    if ( (setup_flags & setup_sqe128) != 0 ) return get_sqe();      // pure 128B ring: every entry is wide
    for ( ;; ) {
      const u32 __t = *sq_tail;
      const u32 __head = atom::load(sq_head, __ATOMIC_ACQUIRE);
      if ( __t - __head + 2 > __sq_entries ) return nullptr;
      const u32 __i = __t & *sq_mask;
      if ( __i + 1 < __sq_entries ) {
        if ( sq_array != nullptr ) {
          sq_array[__i] = __i;
          sq_array[(__t + 1) & *sq_mask] = (__t + 1) & *sq_mask;
        }
        return &sqes[static_cast<usize>(__i)];      // 64B stride: entry i and i+1 form the 128B slot
      }
      sqe *__pad = get_sqe();
      if ( __pad == nullptr ) return nullptr;
      prep_nop(__pad);
      __pad->flags = sqe_cqe_skip_success;
      advance_sq();
    }
  }

  [[gnu::always_inline]] void
  advance_sq(u32 n = 1) noexcept
  {
    atom::store(sq_tail, *sq_tail + n, __ATOMIC_RELEASE);
    atom::fetch_add(&to_submit, n, __ATOMIC_ACQ_REL);
  }

  [[nodiscard]] u32
  sq_ready() const noexcept
  {
    return *sq_tail - atom::load(sq_head, __ATOMIC_ACQUIRE);
  }

  [[nodiscard]] u32
  sq_space_left() const noexcept
  {
    return __sq_entries - sq_ready();
  }

  [[nodiscard]] bool
  need_wakeup() const noexcept
  {
    return sq_flags != nullptr && (atom::load(sq_flags, __ATOMIC_ACQUIRE) & sq_need_wakeup) != 0;
  }

  [[nodiscard]] bool
  cq_overflowed() const noexcept
  {
    return sq_flags != nullptr && (atom::load(sq_flags, __ATOMIC_ACQUIRE) & sq_cq_overflow) != 0;
  }

  [[nodiscard]] bool
  taskrun_pending() const noexcept
  {
    return sq_flags != nullptr && (atom::load(sq_flags, __ATOMIC_ACQUIRE) & sq_taskrun) != 0;
  }

  // raw enter against the (possibly registered) ring fd; flags are OR-ed with the registered-ring bit
  long
  enter2(u32 submit_n, u32 min_complete, u32 flags, void *argp, usize argsz) noexcept
  {
    return __io_uring_enter(enter_fd, submit_n, min_complete, flags | __enter_flags, argp, argsz);
  }

  long
  enter(u32 wait_nr = 0) noexcept
  {
    if ( __sqpoll ) [[unlikely]]
      return __enter_sqpoll(wait_nr);
    const u32 __n = atom::exchange(&to_submit, 0u, __ATOMIC_ACQ_REL);
    long r = __io_uring_enter(enter_fd, __n, wait_nr, (wait_nr != 0 ? enter_getevents : 0u) | __enter_flags, nullptr, 0);
    if ( r >= 0 ) {
      if ( static_cast<u32>(r) < __n ) atom::fetch_add(&to_submit, __n - static_cast<u32>(r), __ATOMIC_ACQ_REL);
      return r;
    }
    atom::fetch_add(&to_submit, __n, __ATOMIC_ACQ_REL);
    if ( r == -4 /*EINTR*/ && wait_nr != 0 ) return 0;
    return r;
  }

  [[gnu::always_inline]] long
  submit() noexcept
  {
    return enter(0);
  }

  [[gnu::always_inline]] long
  submit_and_wait(u32 wait_nr) noexcept
  {
    return enter(wait_nr);
  }

  int
  submit_and_wait_timeout(u32 wait_nr, const ktimespec *ts, u64 sigmask = 0, u32 min_wait_usec = 0, bool abs = false) noexcept
  {
    if ( (features & feat_ext_arg) == 0 ) return -95;
    long s = enter(0);      // flush via the path with correct short-submit re-crediting
    if ( s < 0 && s != -4 /*EINTR*/ ) return static_cast<int>(s);
    getevents_arg __a{};
    __a.sigmask = sigmask;
    __a.sigmask_sz = 8;      // _NSIG / 8
    __a.min_wait_usec = (features & feat_min_timeout) != 0 ? min_wait_usec : 0;
    __a.ts = reinterpret_cast<u64>(ts);
    const u32 __n = atom::exchange(&to_submit, 0u, __ATOMIC_ACQ_REL);
    long r = enter2(__n, wait_nr, enter_getevents | enter_ext_arg | (abs ? enter_abs_timer : 0u), &__a, sizeof(__a));
    if ( r >= 0 ) {
      if ( static_cast<u32>(r) < __n ) atom::fetch_add(&to_submit, __n - static_cast<u32>(r), __ATOMIC_ACQ_REL);
    } else {
      atom::fetch_add(&to_submit, __n, __ATOMIC_ACQ_REL);
    }
    if ( r == -4 /*EINTR*/ ) return 0;
    return r < 0 ? static_cast<int>(r) : 0;
  }

  int
  wait_cqe_timeout(cqe *out, const ktimespec *ts) noexcept
  {
    if ( peek_cqe(out) ) return 0;
    int r = submit_and_wait_timeout(1, ts);
    if ( r < 0 ) return r;
    return peek_cqe(out) ? 0 : -62 /*ETIME*/;
  }

  [[nodiscard]] u32
  cq_ready() const noexcept
  {
    return atom::load(cq_tail, __ATOMIC_ACQUIRE) - *cq_head;
  }

  // in-ring pointer to the i-th ready cqe (big_cqe()-capable); valid until cq_advance
  [[nodiscard]] const cqe *
  cq_peek(u32 i = 0) const noexcept
  {
    if ( i >= cq_ready() ) return nullptr;
    return &cqes[(static_cast<usize>((*cq_head + i) & *cq_mask)) << __cqe_shift];
  }

  [[gnu::always_inline]] void
  cq_advance(u32 n) noexcept
  {
    atom::store(cq_head, *cq_head + n, __ATOMIC_RELEASE);
  }

  [[nodiscard]] bool
  peek_cqe(cqe *out) noexcept
  {
    for ( ;; ) {
      const u32 __h = *cq_head;
      if ( __h == atom::load(cq_tail, __ATOMIC_ACQUIRE) ) return false;
      const cqe *__c = &cqes[(static_cast<usize>(__h & *cq_mask)) << __cqe_shift];
      if ( __cqe_mixed ) [[unlikely]] {
        if ( (__c->flags & cqe_f_skip) != 0 ) {      // wrap filler: consume invisibly
          atom::store(cq_head, __h + 1, __ATOMIC_RELEASE);
          continue;
        }
        *out = *__c;
        atom::store(cq_head, __h + ((__c->flags & cqe_f_32) != 0 ? 2u : 1u), __ATOMIC_RELEASE);
        return true;
      }
      *out = *__c;
      atom::store(cq_head, __h + 1, __ATOMIC_RELEASE);
      return true;
    }
  }

  u32
  peek_batch_cqe(cqe *out, u32 max) noexcept
  {
    u32 n = cq_ready();
    if ( n > max ) n = max;
    const u32 __h = *cq_head;
    for ( u32 k = 0; k < n; k++ ) out[k] = cqes[(static_cast<usize>((__h + k) & *cq_mask)) << __cqe_shift];
    if ( n != 0 ) cq_advance(n);
    return n;
  }

  // NOTE: classic/cqe32 rings only
  template<typename F>
  u32
  for_each_cqe(F &&fn) noexcept
  {
    const u32 __h = *cq_head;
    const u32 __t = atom::load(cq_tail, __ATOMIC_ACQUIRE);
    for ( u32 h = __h; h != __t; h++ ) fn(static_cast<const cqe &>(cqes[(static_cast<usize>(h & *cq_mask)) << __cqe_shift]));
    const u32 n = __t - __h;
    if ( n != 0 ) cq_advance(n);
    return n;
  }

  int
  wait_cqe(cqe *out) noexcept
  {
    for ( ;; ) {
      if ( peek_cqe(out) ) return 0;
      long r = enter(1);
      if ( r < 0 ) return static_cast<int>(r);
    }
  }

  long
  __register(u32 opcode, void *arg, u32 nr) noexcept
  {
    if ( (__enter_flags & enter_registered_ring) != 0 && (features & feat_reg_reg_ring) != 0 )
      return __io_uring_register(enter_fd, opcode | reg_use_registered_ring, arg, nr);
    return __io_uring_register(fd, opcode, arg, nr);
  }

  long
  register_buffers(const iovec *iov, u32 n) noexcept
  {
    return __register(reg_register_buffers, const_cast<iovec *>(iov), n);
  }

  long
  register_buffers_tags(const iovec *iov, const u64 *tags, u32 n) noexcept      // >=5.13
  {
    rsrc_register rr{};
    rr.nr = n;
    rr.data = reinterpret_cast<u64>(iov);
    rr.tags = reinterpret_cast<u64>(tags);
    return __register(reg_register_buffers2, &rr, sizeof(rr));
  }

  long
  register_buffers_sparse(u32 n) noexcept      // >=5.19
  {
    rsrc_register rr{};
    rr.nr = n;
    rr.flags = rsrc_register_sparse;
    return __register(reg_register_buffers2, &rr, sizeof(rr));
  }

  long
  register_buffers_update(u32 off, const iovec *iov, const u64 *tags, u32 n) noexcept      // >=5.13
  {
    rsrc_update2 up{};
    up.offset = off;
    up.data = reinterpret_cast<u64>(iov);
    up.tags = reinterpret_cast<u64>(tags);
    up.nr = n;
    return __register(reg_register_buffers_update, &up, sizeof(up));
  }

  long
  unregister_buffers() noexcept
  {
    return __register(reg_unregister_buffers, nullptr, 0);
  }

  long
  register_files(const i32 *fds, u32 n) noexcept
  {
    return __register(reg_register_files, const_cast<i32 *>(fds), n);
  }

  long
  register_files_tags(const i32 *fds, const u64 *tags, u32 n) noexcept      // >=5.13
  {
    rsrc_register rr{};
    rr.nr = n;
    rr.data = reinterpret_cast<u64>(fds);
    rr.tags = reinterpret_cast<u64>(tags);
    return __register(reg_register_files2, &rr, sizeof(rr));
  }

  long
  register_files_sparse(u32 n) noexcept      // >=5.19
  {
    rsrc_register rr{};
    rr.nr = n;
    rr.flags = rsrc_register_sparse;
    return __register(reg_register_files2, &rr, sizeof(rr));
  }

  long
  register_files_update(u32 off, const i32 *fds, u32 n) noexcept
  {
    rsrc_update up{};
    up.offset = off;
    up.data = reinterpret_cast<u64>(fds);
    return __register(reg_register_files_update, &up, n);
  }

  long
  register_file_alloc_range(u32 off, u32 len) noexcept      // >=6.0
  {
    file_index_range r{};
    r.off = off;
    r.len = len;
    return __register(reg_register_file_alloc_range, &r, 0);
  }

  long
  unregister_files() noexcept
  {
    return __register(reg_unregister_files, nullptr, 0);
  }

  long
  register_eventfd(i32 efd, bool async = false) noexcept
  {
    return __register(async ? reg_register_eventfd_async : reg_register_eventfd, &efd, 1);
  }

  long
  unregister_eventfd() noexcept
  {
    return __register(reg_unregister_eventfd, nullptr, 0);
  }

  long
  register_probe(probe *p) noexcept      // >=5.6
  {
    return __register(reg_register_probe, p, probe::max_ops);
  }

  long
  register_personality() noexcept      // >=5.6; returns the personality id
  {
    return __register(reg_register_personality, nullptr, 0);
  }

  long
  unregister_personality(i32 id) noexcept
  {
    return __register(reg_unregister_personality, nullptr, static_cast<u32>(id));
  }

  long
  register_restrictions(restriction *r, u32 n) noexcept      // >=5.10; ring must be r_disabled
  {
    return __register(reg_register_restrictions, r, n);
  }

  long
  enable_rings() noexcept      // >=5.10; lift setup_r_disabled
  {
    return __register(reg_register_enable_rings, nullptr, 0);
  }

  long
  register_iowq_aff(usize cpusz, const void *mask) noexcept      // >=5.14
  {
    return __register(reg_register_iowq_aff, const_cast<void *>(mask), static_cast<u32>(cpusz));
  }

  long
  unregister_iowq_aff() noexcept
  {
    return __register(reg_unregister_iowq_aff, nullptr, 0);
  }

  long
  register_iowq_max_workers(u32 vals[2]) noexcept      // >=5.15; vals[wq_bound], vals[wq_unbound]; 0 = query
  {
    return __register(reg_register_iowq_max_workers, vals, 2);
  }

  // >=5.18; register the ring fd itself: enter(2) skips the fdget on every syscall afterwards
  int
  register_ring_fd() noexcept
  {
    if ( (__enter_flags & enter_registered_ring) != 0 ) return 0;
    rsrc_update up{};
    up.offset = static_cast<u32>(-1);      // kernel picks the index
    up.data = static_cast<u64>(static_cast<u32>(fd));
    long r = __io_uring_register(fd, reg_register_ring_fds, &up, 1);
    if ( r != 1 ) return r < 0 ? static_cast<int>(r) : -22;
    enter_fd = static_cast<i32>(up.offset);
    __enter_flags |= enter_registered_ring;
    return 0;
  }

  int
  unregister_ring_fd() noexcept
  {
    if ( (__enter_flags & enter_registered_ring) == 0 ) return 0;
    rsrc_update up{};
    up.offset = static_cast<u32>(enter_fd);
    long r = __io_uring_register(fd, reg_unregister_ring_fds, &up, 1);
    if ( r != 1 ) return r < 0 ? static_cast<int>(r) : -22;
    enter_fd = fd;
    __enter_flags &= static_cast<u8>(~enter_registered_ring);
    return 0;
  }

  long
  register_pbuf_ring(const buf_reg &reg) noexcept      // >=5.19
  {
    return __register(reg_register_pbuf_ring, const_cast<buf_reg *>(&reg), 1);
  }

  long
  unregister_pbuf_ring(u16 bgid) noexcept
  {
    buf_reg reg{};
    reg.bgid = bgid;
    return __register(reg_unregister_pbuf_ring, &reg, 1);
  }

  long
  register_pbuf_status(buf_status &st) noexcept      // >=6.8
  {
    return __register(reg_register_pbuf_status, &st, 1);
  }

  // >=6.4
  int
  map_pbuf_ring(u16 bgid, u32 entries, bufring *out) noexcept
  {
    buf_reg reg{};
    reg.ring_entries = entries;
    reg.bgid = bgid;
    reg.flags = pbuf_ring_mmap;
    long r = register_pbuf_ring(reg);
    if ( r < 0 ) return static_cast<int>(r);
    void *p = micron::mmap(nullptr, static_cast<usize>(entries) * sizeof(buf), prot_read | prot_write, map_shared | map_populate, fd,
                           static_cast<posix::off_t>(off_pbuf_ring | (static_cast<u64>(bgid) << off_pbuf_shift)));
    if ( __map_failed(p) ) {
      (void)unregister_pbuf_ring(bgid);
      return static_cast<int>(reinterpret_cast<i64>(p));
    }
    out->br = reinterpret_cast<buf_ring *>(p);
    out->entries = entries;
    return 0;
  }

  long
  register_napi(napi &n) noexcept      // >=6.9
  {
    return __register(reg_register_napi, &n, 1);
  }

  long
  unregister_napi(napi &n) noexcept
  {
    return __register(reg_unregister_napi, &n, 1);
  }

  long
  register_clock(u32 clockid) noexcept      // >=6.12; clock for enter_abs_timer deadlines
  {
    clock_register c{};
    c.clockid = clockid;
    return __register(reg_register_clock, &c, 0);
  }

  // >=6.12; clone src's registered buffers into this ring (zero-copy handover)
  long
  clone_buffers_from(ring &src, u32 flags = 0, u32 src_off = 0, u32 dst_off = 0, u32 nr = 0) noexcept
  {
    clone_buffers cb{};
    cb.src_fd = static_cast<u32>(src.fd);
    cb.flags = flags;
    cb.src_off = src_off;
    cb.dst_off = dst_off;
    cb.nr = nr;
    return __register(reg_register_clone_buffers, &cb, 1);
  }

  long
  sync_cancel(sync_cancel_reg &sc) noexcept      // >=6.0; synchronous cancel without an sqe
  {
    return __register(reg_register_sync_cancel, &sc, 1);
  }

  long
  register_mem_region(mem_region_reg &mr) noexcept      // >=6.13
  {
    return __register(reg_register_mem_region, &mr, 1);
  }

  long
  register_send_msg_ring(sqe &s) noexcept      // >=6.13; msg_ring into this ring from a ringless task
  {
    return __register(reg_register_send_msg_ring, &s, 1);
  }

  long
  register_zcrx_ifq(zcrx_ifq_reg &z) noexcept      // >=6.15
  {
    return __register(reg_register_zcrx_ifq, &z, 1);
  }

  long
  register_zcrx_ctrl(zcrx_ctrl &z) noexcept      // >=6.18
  {
    return __register(reg_register_zcrx_ctrl, &z, 1);
  }

  long
  register_query(query_hdr &q) noexcept      // >=6.17
  {
    return __register(reg_register_query, &q, 0);
  }

  long
  register_bpf_filter(void *arg, u32 nr) noexcept      // >=7.1; raw pass-through
  {
    return __register(reg_register_bpf_filter, arg, nr);
  }

  // >=6.13
  int
  resize_rings(params &p) noexcept
  {
    long r = __register(reg_register_resize_rings, &p, 1);
    if ( r < 0 ) return static_cast<int>(r);
    if ( sqes != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(sqes), __sqes_len);
    sqes = nullptr;
    __sqes_len = 0;
    if ( __cq_ptr != nullptr && __cq_ptr != __sq_ptr ) micron::munmap(reinterpret_cast<addr_t *>(__cq_ptr), __cq_len);
    if ( __sq_ptr != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(__sq_ptr), __sq_len);
    __sq_ptr = nullptr;
    __cq_ptr = nullptr;
    __sq_entries = p.sq_entries;
    __cq_entries = p.cq_entries;
    __sq_len = p.sq_off.array + p.sq_entries * sizeof(u32);
    __cq_len = p.cq_off.cqes + ((static_cast<usize>(p.cq_entries) * sizeof(cqe)) << __cqe_shift);
    if ( features & feat_single_mmap ) {
      if ( __cq_len > __sq_len ) __sq_len = __cq_len;
      __cq_len = __sq_len;
    }
    __sq_ptr = reinterpret_cast<byte *>(
        micron::mmap(nullptr, __sq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sq_ring)));
    if ( __map_failed(__sq_ptr) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(__sq_ptr));
      __sq_ptr = nullptr;
      return __e;      // ring is unusable now; caller should shutdown()
    }
    if ( features & feat_single_mmap ) {
      __cq_ptr = __sq_ptr;
    } else {
      __cq_ptr = reinterpret_cast<byte *>(
          micron::mmap(nullptr, __cq_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_cq_ring)));
      if ( __map_failed(__cq_ptr) ) {
        int __e = static_cast<int>(reinterpret_cast<i64>(__cq_ptr));
        __cq_ptr = nullptr;
        return __e;
      }
    }
    __sqes_len = (static_cast<usize>(p.sq_entries) * sizeof(sqe)) << __sqe_shift;
    sqes = reinterpret_cast<sqe *>(
        micron::mmap(nullptr, __sqes_len, prot_read | prot_write, map_shared | map_populate, fd, static_cast<posix::off_t>(off_sqes)));
    if ( __map_failed(sqes) ) {
      int __e = static_cast<int>(reinterpret_cast<i64>(sqes));
      sqes = nullptr;
      __sqes_len = 0;
      return __e;      // ring is unusable now; caller should shutdown()
    }
    __wire(p);
    return 0;
  }

  [[nodiscard]] bool
  supports(u8 op) noexcept
  {
    if ( !__probed ) {
      probe __p{};
      if ( register_probe(&__p) >= 0 )
        for ( u32 i = 0; i < __p.ops_len && i < probe::max_ops; i++ )
          if ( (__p.ops[i].flags & probe_op_supported) != 0 ) __op_bits[__p.ops[i].op >> 6] |= 1ull << (__p.ops[i].op & 63);
      __probed = true;
    }
    return ((__op_bits[op >> 6] >> (op & 63)) & 1) != 0;
  }

private:
  void
  __wire(const params &p) noexcept
  {
    sq_head = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.head);
    sq_tail = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.tail);
    sq_mask = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.ring_mask);
    sq_flags = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.flags);
    sq_dropped = reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.dropped);
    sq_array = (p.flags & setup_no_sqarray) != 0 ? nullptr : reinterpret_cast<u32 *>(__sq_ptr + p.sq_off.array);
    cq_head = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.head);
    cq_tail = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.tail);
    cq_mask = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.ring_mask);
    cq_overflow = reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.overflow);
    cq_flags = p.cq_off.flags != 0 ? reinterpret_cast<u32 *>(__cq_ptr + p.cq_off.flags) : nullptr;
    cqes = reinterpret_cast<cqe *>(__cq_ptr + p.cq_off.cqes);
  }

  long
  __enter_sqpoll(u32 wait_nr) noexcept
  {
    (void)atom::exchange(&to_submit, 0u, __ATOMIC_ACQ_REL);
    u32 __f = static_cast<u32>(__enter_flags);
    if ( need_wakeup() ) __f |= enter_sq_wakeup;
    if ( wait_nr != 0 ) __f |= enter_getevents;
    if ( (__f & (enter_sq_wakeup | enter_getevents)) == 0 ) return 0;      // thread awake, nothing to wait for
    long r = __io_uring_enter(enter_fd, 0, wait_nr, __f, nullptr, 0);
    if ( r == -4 /*EINTR*/ && wait_nr != 0 ) return 0;
    return r;
  }

  void
  __close_fd() noexcept
  {
    if ( fd >= 0 ) micron::syscall(SYS_close, fd);
    fd = -1;
    enter_fd = -1;
    __enter_flags = 0;
  }
};

};      // namespace uring
};      // namespace micron
