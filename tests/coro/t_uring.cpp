

#include "../../src/kernel.hpp"
#include "../../src/linux/sys/uring.hpp"

#include "../../src/sync/futex.hpp"
#include "../../src/thread/thread.hpp"

#include "../snowball/snowball.hpp"

namespace ur = micron::uring;
namespace kn = micron::kernel;
static int FAILS = 0;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group A: self-ABI -- always compiled, needs no UAPI header. sizes/offsets of our own structs.

static_assert(sizeof(ur::sqe) == 64 && sizeof(ur::cqe) == 16 && sizeof(ur::params) == 120);
static_assert(ur::op_last == 65 && ur::reg_last == 38);
static_assert(__builtin_offsetof(ur::sqe, off) == 8 && __builtin_offsetof(ur::sqe, addr) == 16 && __builtin_offsetof(ur::sqe, len) == 24
              && __builtin_offsetof(ur::sqe, rw_flags) == 28 && __builtin_offsetof(ur::sqe, user_data) == 32
              && __builtin_offsetof(ur::sqe, buf_index) == 40 && __builtin_offsetof(ur::sqe, personality) == 42
              && __builtin_offsetof(ur::sqe, file_index) == 44 && __builtin_offsetof(ur::sqe, addr3) == 48);
static_assert(__builtin_offsetof(ur::sqe, addr2) == 8 && __builtin_offsetof(ur::sqe, cmd_op) == 8
              && __builtin_offsetof(ur::sqe, splice_off_in) == 16 && __builtin_offsetof(ur::sqe, level) == 16
              && __builtin_offsetof(ur::sqe, buf_group) == 40 && __builtin_offsetof(ur::sqe, splice_fd_in) == 44
              && __builtin_offsetof(ur::sqe, optval) == 48 && __builtin_offsetof(ur::sqe, attr_type_mask) == 56);
static_assert(sizeof(ur::rsrc_register) == 32 && sizeof(ur::rsrc_update) == 16 && sizeof(ur::rsrc_update2) == 32
              && __builtin_offsetof(ur::probe, ops) == 16 && sizeof(ur::buf) == 16 && sizeof(ur::buf_ring) == 16
              && __builtin_offsetof(ur::buf_ring, tail) == 14 && sizeof(ur::buf_reg) == 40 && sizeof(ur::napi) == 16
              && sizeof(ur::getevents_arg) == 24 && sizeof(ur::sync_cancel_reg) == 64 && sizeof(ur::reg_wait) == 64
              && sizeof(ur::region_desc) == 64 && sizeof(ur::zcrx_ifq_reg) == 96 && sizeof(ur::query_opcode) == 48);

#if __has_include(<linux/io_uring.h>)
#define T_URING_HAVE_UAPI 1

namespace uapi
{
#include <linux/io_uring.h>
};

using namespace uapi;      // brings the IOSQE_*_BIT enumerators (referenced by the IOSQE_* macros) into scope

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group B: #define constants -- individually #ifdef-guarded (robust against older build-host headers)

// sqe/cqe/params struct-shape cross-check
static_assert(sizeof(ur::sqe) == sizeof(uapi::io_uring_sqe) && sizeof(ur::cqe) == sizeof(uapi::io_uring_cqe)
              && sizeof(ur::params) == sizeof(uapi::io_uring_params));
static_assert(__builtin_offsetof(ur::sqe, user_data) == __builtin_offsetof(uapi::io_uring_sqe, user_data)
              && __builtin_offsetof(ur::sqe, addr3) == __builtin_offsetof(uapi::io_uring_sqe, addr3)
              && __builtin_offsetof(ur::sqe, buf_index) == __builtin_offsetof(uapi::io_uring_sqe, buf_index)
              && __builtin_offsetof(ur::sqe, file_index) == __builtin_offsetof(uapi::io_uring_sqe, file_index));

// setup flags
#ifdef IORING_SETUP_IOPOLL
static_assert(ur::setup_iopoll == IORING_SETUP_IOPOLL && ur::setup_sqpoll == IORING_SETUP_SQPOLL && ur::setup_sq_aff == IORING_SETUP_SQ_AFF
              && ur::setup_cqsize == IORING_SETUP_CQSIZE && ur::setup_clamp == IORING_SETUP_CLAMP
              && ur::setup_attach_wq == IORING_SETUP_ATTACH_WQ && ur::setup_r_disabled == IORING_SETUP_R_DISABLED
              && ur::setup_submit_all == IORING_SETUP_SUBMIT_ALL);
#endif
#ifdef IORING_SETUP_COOP_TASKRUN
static_assert(ur::setup_coop_taskrun == IORING_SETUP_COOP_TASKRUN && ur::setup_taskrun_flag == IORING_SETUP_TASKRUN_FLAG
              && ur::setup_sqe128 == IORING_SETUP_SQE128 && ur::setup_cqe32 == IORING_SETUP_CQE32
              && ur::setup_single_issuer == IORING_SETUP_SINGLE_ISSUER && ur::setup_defer_taskrun == IORING_SETUP_DEFER_TASKRUN);
#endif
#ifdef IORING_SETUP_NO_MMAP
static_assert(ur::setup_no_mmap == IORING_SETUP_NO_MMAP && ur::setup_registered_fd_only == IORING_SETUP_REGISTERED_FD_ONLY
              && ur::setup_no_sqarray == IORING_SETUP_NO_SQARRAY);
#endif
#ifdef IORING_SETUP_HYBRID_IOPOLL
static_assert(ur::setup_hybrid_iopoll == IORING_SETUP_HYBRID_IOPOLL);
#endif
#ifdef IORING_SETUP_CQE_MIXED
static_assert(ur::setup_cqe_mixed == IORING_SETUP_CQE_MIXED);
#endif
#ifdef IORING_SETUP_SQ_REWIND
static_assert(ur::setup_sqe_mixed == IORING_SETUP_SQE_MIXED && ur::setup_sq_rewind == IORING_SETUP_SQ_REWIND);
#endif

// enter flags
#ifdef IORING_ENTER_GETEVENTS
static_assert(ur::enter_getevents == IORING_ENTER_GETEVENTS && ur::enter_sq_wakeup == IORING_ENTER_SQ_WAKEUP
              && ur::enter_sq_wait == IORING_ENTER_SQ_WAIT && ur::enter_ext_arg == IORING_ENTER_EXT_ARG
              && ur::enter_registered_ring == IORING_ENTER_REGISTERED_RING);
#endif
#ifdef IORING_ENTER_ABS_TIMER
static_assert(ur::enter_abs_timer == IORING_ENTER_ABS_TIMER && ur::enter_ext_arg_reg == IORING_ENTER_EXT_ARG_REG
              && ur::enter_no_iowait == IORING_ENTER_NO_IOWAIT);
#endif

// feature bits
#ifdef IORING_FEAT_SINGLE_MMAP
static_assert(ur::feat_single_mmap == IORING_FEAT_SINGLE_MMAP && ur::feat_nodrop == IORING_FEAT_NODROP
              && ur::feat_submit_stable == IORING_FEAT_SUBMIT_STABLE && ur::feat_fast_poll == IORING_FEAT_FAST_POLL
              && ur::feat_ext_arg == IORING_FEAT_EXT_ARG && ur::feat_cqe_skip == IORING_FEAT_CQE_SKIP);
#endif
#ifdef IORING_FEAT_REG_REG_RING
static_assert(ur::feat_reg_reg_ring == IORING_FEAT_REG_REG_RING);
#endif
#ifdef IORING_FEAT_MIN_TIMEOUT
static_assert(ur::feat_recvsend_bundle == IORING_FEAT_RECVSEND_BUNDLE && ur::feat_min_timeout == IORING_FEAT_MIN_TIMEOUT);
#endif
#ifdef IORING_FEAT_RW_ATTR
static_assert(ur::feat_rw_attr == IORING_FEAT_RW_ATTR);
#endif

// iosqe flags
#ifdef IOSQE_FIXED_FILE
static_assert(ur::sqe_fixed_file == IOSQE_FIXED_FILE && ur::sqe_io_drain == IOSQE_IO_DRAIN && ur::sqe_io_link == IOSQE_IO_LINK
              && ur::sqe_io_hardlink == IOSQE_IO_HARDLINK && ur::sqe_async == IOSQE_ASYNC && ur::sqe_buffer_select == IOSQE_BUFFER_SELECT
              && ur::sqe_cqe_skip_success == IOSQE_CQE_SKIP_SUCCESS);
#endif

// cqe flags, sq/cq ring flags, mmap offsets
#ifdef IORING_CQE_F_BUFFER
static_assert(ur::cqe_f_buffer == IORING_CQE_F_BUFFER && ur::cqe_f_more == IORING_CQE_F_MORE
              && ur::cqe_f_sock_nonempty == IORING_CQE_F_SOCK_NONEMPTY && ur::cqe_f_notif == IORING_CQE_F_NOTIF
              && ur::cqe_buffer_shift == IORING_CQE_BUFFER_SHIFT);
#endif
#ifdef IORING_CQE_F_SKIP
static_assert(ur::cqe_f_buf_more == IORING_CQE_F_BUF_MORE && ur::cqe_f_skip == IORING_CQE_F_SKIP && ur::cqe_f_32 == IORING_CQE_F_32);
#endif
#ifdef IORING_SQ_NEED_WAKEUP
static_assert(ur::sq_need_wakeup == IORING_SQ_NEED_WAKEUP && ur::sq_cq_overflow == IORING_SQ_CQ_OVERFLOW
              && ur::sq_taskrun == IORING_SQ_TASKRUN && ur::cq_eventfd_disabled == IORING_CQ_EVENTFD_DISABLED);
#endif
#ifdef IORING_OFF_SQ_RING
static_assert(ur::off_sq_ring == IORING_OFF_SQ_RING && ur::off_cq_ring == IORING_OFF_CQ_RING && ur::off_sqes == IORING_OFF_SQES);
#endif
#ifdef IORING_OFF_PBUF_RING
static_assert(ur::off_pbuf_ring == IORING_OFF_PBUF_RING && ur::off_pbuf_shift == IORING_OFF_PBUF_SHIFT
              && ur::off_mmap_mask == IORING_OFF_MMAP_MASK);
#endif

// per-op flag families
#ifdef IORING_FSYNC_DATASYNC
static_assert(ur::fsync_datasync == IORING_FSYNC_DATASYNC);
#endif
#ifdef IORING_TIMEOUT_ABS
static_assert(ur::timeout_abs == IORING_TIMEOUT_ABS && ur::timeout_update == IORING_TIMEOUT_UPDATE
              && ur::timeout_boottime == IORING_TIMEOUT_BOOTTIME && ur::timeout_realtime == IORING_TIMEOUT_REALTIME
              && ur::timeout_etime_success == IORING_TIMEOUT_ETIME_SUCCESS && ur::timeout_multishot == IORING_TIMEOUT_MULTISHOT);
#endif
#ifdef IORING_POLL_ADD_MULTI
static_assert(ur::poll_add_multi == IORING_POLL_ADD_MULTI && ur::poll_update_events == IORING_POLL_UPDATE_EVENTS
              && ur::poll_update_user_data == IORING_POLL_UPDATE_USER_DATA && ur::poll_add_level == IORING_POLL_ADD_LEVEL);
#endif
#ifdef IORING_ASYNC_CANCEL_ALL
static_assert(ur::async_cancel_all == IORING_ASYNC_CANCEL_ALL && ur::async_cancel_fd == IORING_ASYNC_CANCEL_FD
              && ur::async_cancel_any == IORING_ASYNC_CANCEL_ANY && ur::async_cancel_fd_fixed == IORING_ASYNC_CANCEL_FD_FIXED
              && ur::async_cancel_userdata == IORING_ASYNC_CANCEL_USERDATA && ur::async_cancel_op == IORING_ASYNC_CANCEL_OP);
#endif
#ifdef IORING_RECVSEND_POLL_FIRST
static_assert(ur::recvsend_poll_first == IORING_RECVSEND_POLL_FIRST && ur::recv_multishot == IORING_RECV_MULTISHOT
              && ur::recvsend_fixed_buf == IORING_RECVSEND_FIXED_BUF);
#endif
#ifdef IORING_ACCEPT_MULTISHOT
static_assert(ur::accept_multishot == IORING_ACCEPT_MULTISHOT);
#endif
#ifdef IORING_MSG_RING_CQE_SKIP
static_assert(ur::msg_ring_cqe_skip == IORING_MSG_RING_CQE_SKIP && ur::msg_ring_flags_pass == IORING_MSG_RING_FLAGS_PASS);
#endif
#ifdef IORING_FIXED_FD_NO_CLOEXEC
static_assert(ur::fixed_fd_no_cloexec == IORING_FIXED_FD_NO_CLOEXEC);
#endif
#ifdef IORING_NOP_INJECT_RESULT
static_assert(ur::nop_inject_result == IORING_NOP_INJECT_RESULT);
#endif
#ifdef IORING_URING_CMD_FIXED
static_assert(ur::uring_cmd_fixed == IORING_URING_CMD_FIXED);
#endif
#ifdef IORING_FILE_INDEX_ALLOC
static_assert(ur::file_index_alloc == IORING_FILE_INDEX_ALLOC);
#endif
#ifdef IORING_REGISTER_FILES_SKIP
static_assert(ur::register_files_skip == IORING_REGISTER_FILES_SKIP);
#endif
#ifdef IORING_RSRC_REGISTER_SPARSE
static_assert(ur::rsrc_register_sparse == IORING_RSRC_REGISTER_SPARSE);
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// group C: enum opcodes/register-ops + struct cross-checks -- enums can't be #ifdef'd, so gate each
// wave behind a #define macro (or __has_include) from that wave's kernel release. proxy_ver is >= the
// wave it guards, so an intermediate-version header never sees a missing enumerator. host is 7.1 =
// every tier active.

// tier 5.19: ops nop..uring_cmd (0..46), register ops 0..23, the classic register/pbuf structs
#ifdef IORING_ASYNC_CANCEL_ALL
static_assert(ur::op_nop == uapi::IORING_OP_NOP && ur::op_readv == uapi::IORING_OP_READV && ur::op_writev == uapi::IORING_OP_WRITEV
              && ur::op_fsync == uapi::IORING_OP_FSYNC && ur::op_read_fixed == uapi::IORING_OP_READ_FIXED
              && ur::op_write_fixed == uapi::IORING_OP_WRITE_FIXED && ur::op_poll_add == uapi::IORING_OP_POLL_ADD
              && ur::op_poll_remove == uapi::IORING_OP_POLL_REMOVE && ur::op_sync_file_range == uapi::IORING_OP_SYNC_FILE_RANGE
              && ur::op_sendmsg == uapi::IORING_OP_SENDMSG && ur::op_recvmsg == uapi::IORING_OP_RECVMSG);
static_assert(ur::op_timeout == uapi::IORING_OP_TIMEOUT && ur::op_timeout_remove == uapi::IORING_OP_TIMEOUT_REMOVE
              && ur::op_accept == uapi::IORING_OP_ACCEPT && ur::op_async_cancel == uapi::IORING_OP_ASYNC_CANCEL
              && ur::op_link_timeout == uapi::IORING_OP_LINK_TIMEOUT && ur::op_connect == uapi::IORING_OP_CONNECT
              && ur::op_fallocate == uapi::IORING_OP_FALLOCATE && ur::op_openat == uapi::IORING_OP_OPENAT
              && ur::op_close == uapi::IORING_OP_CLOSE && ur::op_files_update == uapi::IORING_OP_FILES_UPDATE
              && ur::op_statx == uapi::IORING_OP_STATX);
static_assert(ur::op_read == uapi::IORING_OP_READ && ur::op_write == uapi::IORING_OP_WRITE && ur::op_fadvise == uapi::IORING_OP_FADVISE
              && ur::op_madvise == uapi::IORING_OP_MADVISE && ur::op_send == uapi::IORING_OP_SEND && ur::op_recv == uapi::IORING_OP_RECV
              && ur::op_openat2 == uapi::IORING_OP_OPENAT2 && ur::op_epoll_ctl == uapi::IORING_OP_EPOLL_CTL
              && ur::op_splice == uapi::IORING_OP_SPLICE && ur::op_provide_buffers == uapi::IORING_OP_PROVIDE_BUFFERS
              && ur::op_remove_buffers == uapi::IORING_OP_REMOVE_BUFFERS && ur::op_tee == uapi::IORING_OP_TEE);
static_assert(ur::op_shutdown == uapi::IORING_OP_SHUTDOWN && ur::op_renameat == uapi::IORING_OP_RENAMEAT
              && ur::op_unlinkat == uapi::IORING_OP_UNLINKAT && ur::op_mkdirat == uapi::IORING_OP_MKDIRAT
              && ur::op_symlinkat == uapi::IORING_OP_SYMLINKAT && ur::op_linkat == uapi::IORING_OP_LINKAT
              && ur::op_msg_ring == uapi::IORING_OP_MSG_RING && ur::op_fsetxattr == uapi::IORING_OP_FSETXATTR
              && ur::op_setxattr == uapi::IORING_OP_SETXATTR && ur::op_fgetxattr == uapi::IORING_OP_FGETXATTR
              && ur::op_getxattr == uapi::IORING_OP_GETXATTR && ur::op_socket == uapi::IORING_OP_SOCKET
              && ur::op_uring_cmd == uapi::IORING_OP_URING_CMD);
static_assert(ur::reg_register_buffers == uapi::IORING_REGISTER_BUFFERS && ur::reg_unregister_buffers == uapi::IORING_UNREGISTER_BUFFERS
              && ur::reg_register_files == uapi::IORING_REGISTER_FILES && ur::reg_register_probe == uapi::IORING_REGISTER_PROBE
              && ur::reg_register_files2 == uapi::IORING_REGISTER_FILES2 && ur::reg_register_buffers2 == uapi::IORING_REGISTER_BUFFERS2
              && ur::reg_register_ring_fds == uapi::IORING_REGISTER_RING_FDS
              && ur::reg_register_pbuf_ring == uapi::IORING_REGISTER_PBUF_RING
              && ur::reg_unregister_pbuf_ring == uapi::IORING_UNREGISTER_PBUF_RING);
static_assert(sizeof(ur::rsrc_register) == sizeof(uapi::io_uring_rsrc_register)
              && sizeof(ur::rsrc_update) == sizeof(uapi::io_uring_rsrc_update)
              && sizeof(ur::rsrc_update2) == sizeof(uapi::io_uring_rsrc_update2) && sizeof(ur::probe_op) == sizeof(uapi::io_uring_probe_op)
              && sizeof(ur::buf) == sizeof(uapi::io_uring_buf) && sizeof(ur::buf_ring) == sizeof(uapi::io_uring_buf_ring)
              && sizeof(ur::buf_reg) == sizeof(uapi::io_uring_buf_reg)
              && sizeof(ur::getevents_arg) == sizeof(uapi::io_uring_getevents_arg));
static_assert(ur::reg_use_registered_ring == (u32)uapi::IORING_REGISTER_USE_REGISTERED_RING);
#endif

// tier 6.8: ops send_zc..fixed_fd_install (47..54), register sync_cancel..unregister_napi (24..28), napi/status
#ifdef IORING_FIXED_FD_NO_CLOEXEC
static_assert(ur::op_send_zc == uapi::IORING_OP_SEND_ZC && ur::op_sendmsg_zc == uapi::IORING_OP_SENDMSG_ZC
              && ur::op_read_multishot == uapi::IORING_OP_READ_MULTISHOT && ur::op_waitid == uapi::IORING_OP_WAITID
              && ur::op_futex_wait == uapi::IORING_OP_FUTEX_WAIT && ur::op_futex_wake == uapi::IORING_OP_FUTEX_WAKE
              && ur::op_futex_waitv == uapi::IORING_OP_FUTEX_WAITV && ur::op_fixed_fd_install == uapi::IORING_OP_FIXED_FD_INSTALL);
static_assert(ur::reg_register_sync_cancel == uapi::IORING_REGISTER_SYNC_CANCEL
              && ur::reg_register_file_alloc_range == uapi::IORING_REGISTER_FILE_ALLOC_RANGE
              && ur::reg_register_pbuf_status == uapi::IORING_REGISTER_PBUF_STATUS && ur::reg_register_napi == uapi::IORING_REGISTER_NAPI
              && ur::reg_unregister_napi == uapi::IORING_UNREGISTER_NAPI);
static_assert(sizeof(ur::napi) == sizeof(uapi::io_uring_napi) && sizeof(ur::buf_status) == sizeof(uapi::io_uring_buf_status)
              && sizeof(ur::sync_cancel_reg) == sizeof(uapi::io_uring_sync_cancel_reg)
              && sizeof(ur::file_index_range) == sizeof(uapi::io_uring_file_index_range));
#endif

// tier 6.13: ops ftruncate/bind/listen (55..57), register clock..mem_region (29..34), region/reg_wait structs
#ifdef IORING_SETUP_HYBRID_IOPOLL
static_assert(ur::op_ftruncate == uapi::IORING_OP_FTRUNCATE && ur::op_bind == uapi::IORING_OP_BIND
              && ur::op_listen == uapi::IORING_OP_LISTEN);
static_assert(ur::reg_register_clock == uapi::IORING_REGISTER_CLOCK && ur::reg_register_clone_buffers == uapi::IORING_REGISTER_CLONE_BUFFERS
              && ur::reg_register_send_msg_ring == uapi::IORING_REGISTER_SEND_MSG_RING
              && ur::reg_register_resize_rings == uapi::IORING_REGISTER_RESIZE_RINGS
              && ur::reg_register_mem_region == uapi::IORING_REGISTER_MEM_REGION);
static_assert(sizeof(ur::region_desc) == sizeof(uapi::io_uring_region_desc)
              && sizeof(ur::mem_region_reg) == sizeof(uapi::io_uring_mem_region_reg)
              && sizeof(ur::reg_wait) == sizeof(uapi::io_uring_reg_wait)
              && sizeof(ur::clock_register) == sizeof(uapi::io_uring_clock_register)
              && sizeof(ur::clone_buffers) == sizeof(uapi::io_uring_clone_buffers)
              && sizeof(ur::recvmsg_out) == sizeof(uapi::io_uring_recvmsg_out) && sizeof(ur::attr_pi) == sizeof(uapi::io_uring_attr_pi));
#endif

// tier 6.15: zcrx ops (58..61) + zcrx structs + register zcrx_ifq
#if __has_include(<linux/io_uring/zcrx.h>)
static_assert(ur::op_recv_zc == uapi::IORING_OP_RECV_ZC && ur::op_epoll_wait == uapi::IORING_OP_EPOLL_WAIT
              && ur::op_readv_fixed == uapi::IORING_OP_READV_FIXED && ur::op_writev_fixed == uapi::IORING_OP_WRITEV_FIXED);
static_assert(sizeof(ur::zcrx_rqe) == sizeof(uapi::io_uring_zcrx_rqe) && sizeof(ur::zcrx_cqe) == sizeof(uapi::io_uring_zcrx_cqe)
              && sizeof(ur::zcrx_area_reg) == sizeof(uapi::io_uring_zcrx_area_reg)
              && sizeof(ur::zcrx_ifq_reg) == sizeof(uapi::io_uring_zcrx_ifq_reg));
#endif

// tier 6.18: op pipe (62), register query/zcrx_ctrl (35/36)
#ifdef IORING_SETUP_CQE_MIXED
static_assert(ur::op_pipe == uapi::IORING_OP_PIPE && ur::reg_register_query == uapi::IORING_REGISTER_QUERY
              && ur::reg_register_zcrx_ctrl == uapi::IORING_REGISTER_ZCRX_CTRL);
#endif

// tier 7.x: nop128/uring_cmd128 (63/64), op_last, bpf_filter (37), reg_last
#ifdef IORING_SETUP_SQ_REWIND
static_assert(ur::op_nop128 == uapi::IORING_OP_NOP128 && ur::op_uring_cmd128 == uapi::IORING_OP_URING_CMD128
              && ur::op_last == uapi::IORING_OP_LAST && ur::reg_register_bpf_filter == uapi::IORING_REGISTER_BPF_FILTER
              && ur::reg_last == uapi::IORING_REGISTER_LAST);
#endif
#endif      // T_URING_HAVE_UAPI

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  ur::ring r;
  if ( int rc = r.init(8); rc != 0 ) {
    sb::print("io_uring unavailable (init rc=", rc, "); substrate smoke SKIPPED");
    return 1;
  }

  sb::test_case("nop: fused submit+wait roundtrip");
  {
    ur::sqe *s = r.get_sqe();
    sb::require(s != nullptr);
    ur::prep_nop(s);
    s->user_data = 0x1101;
    r.advance_sq();
    ur::cqe c{};
    sb::check(r.wait_cqe(&c) == 0);
    sb::check(c.user_data == 0x1101 && c.res == 0);
  }
  sb::end_test_case();

  const bool have_futex = kn::has(kn::feature::uring_futex) && r.supports(ur::op_futex_wait);
  const bool have_msg_ring = kn::has(kn::feature::uring_msg_ring) && r.supports(ur::op_msg_ring);

  if ( have_futex ) {
    sb::test_case("futex_wait op: arm-time mismatch completes -EAGAIN");
    {
      u32 word = 7;
      ur::sqe *s = r.get_sqe();
      ur::prep_futex_wait(s, &word, 0);
      s->user_data = 51;
      r.advance_sq();
      ur::cqe c{};
      sb::check(r.wait_cqe(&c) == 0);
      sb::check(c.user_data == 51 && c.res == -11);
    }
    sb::end_test_case();

    sb::test_case("futex_wait op is woken by a plain FUTEX_WAKE (hybrid-park property)");
    {
      static u32 word = 0;
      auto t = micron::solo::spawn([]() {
        micron::timespec_t ts{ 0, 50000000 };
        micron::nanosleep(ts);
        micron::atom::store(&word, 1u, __ATOMIC_RELEASE);
        micron::wake_futex(&word, 1);
      });
      ur::sqe *s = r.get_sqe();
      ur::prep_futex_wait(s, &word, 0);
      s->user_data = 99;
      r.advance_sq();
      ur::cqe c{};
      sb::check(r.wait_cqe(&c) == 0);
      sb::check(c.user_data == 99);
      sb::check(c.res == 0 || c.res == -11);
      sb::check(word == 1);
      t->join();
    }
    sb::end_test_case();
  } else {
    sb::print("futex ops unsupported (kernel < 6.7); SKIPPED");
  }

  if ( have_msg_ring ) {
    sb::test_case("msg_ring: cross-ring wake with local success cqe suppressed");
    {
      ur::ring r2;
      sb::require(r2.init(8) == 0);
      ur::sqe *s = r.get_sqe();
      ur::prep_msg_ring(s, r2.fd, 7, 42);
      if ( r.features & ur::feat_cqe_skip ) s->flags |= ur::sqe_cqe_skip_success;
      r.advance_sq();
      sb::check(r.enter(0) >= 0);
      ur::cqe c{};
      sb::check(r2.wait_cqe(&c) == 0);
      sb::check(c.user_data == 42 && c.res == 7);
    }
    sb::end_test_case();
  } else {
    sb::print("msg_ring op unsupported (kernel < 5.18); SKIPPED");
  }

  sb::test_case("timeout op: relative 30ms fires within sane bounds");
  {
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    ur::ktimespec rel{ 0, 30000000 };
    ur::sqe *s = r.get_sqe();
    ur::prep_timeout(s, &rel);
    s->user_data = 11;
    r.advance_sq();
    ur::cqe c{};
    sb::check(r.wait_cqe(&c) == 0);
    micron::clock_gettime(micron::clock_monotonic, t1);
    sb::check(c.user_data == 11 && c.res == -62);
    const i64 ns = (t1.tv_sec - t0.tv_sec) * 1000000000ll + (t1.tv_nsec - t0.tv_nsec);
    sb::check(ns >= 29000000ll && ns < 500000000ll);
  }
  sb::end_test_case();

  sb::test_case("timeout op: a pure timer ignores unrelated completions on the same ring");
  {
    // the case above runs on an IDLE ring, so a sequence timeout (sqe->off != 0) still expires
    // normally and looks identical to a pure timer. only a second, unrelated completion tells them
    // apart: with off == 1 the timer fires the moment the nop lands, collapsing every sleep on a
    // busy ring to ~0.
    micron::timespec_t t0{}, t1{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    ur::ktimespec rel{ 0, 200000000 };      // 200ms
    ur::sqe *s = r.get_sqe();
    ur::prep_timeout(s, &rel);      // default wait_nr
    s->user_data = 12;
    r.advance_sq();
    ur::sqe *nop = r.get_sqe();
    ur::prep_nop(nop);
    nop->user_data = 13;
    r.advance_sq();

    ur::cqe c1{}, c2{};
    sb::check(r.wait_cqe(&c1) == 0);
    sb::check(c1.user_data == 13 && c1.res == 0);      // the nop lands first
    sb::check(r.wait_cqe(&c2) == 0);
    micron::clock_gettime(micron::clock_monotonic, t1);
    sb::check(c2.user_data == 12 && c2.res == -62);      // -ETIME: expired, not sequence-triggered
    const i64 ns = (t1.tv_sec - t0.tv_sec) * 1000000000ll + (t1.tv_nsec - t0.tv_nsec);
    sb::check(ns >= 190000000ll);      // the nop must NOT have cut the timer short
  }
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL URING SUBSTRATE TESTS PASSED ===");
  return 1;
}
