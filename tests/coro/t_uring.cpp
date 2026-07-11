

#include "../../src/linux/sys/uring.hpp"

#include "../../src/sync/futex.hpp"
#include "../../src/thread/thread.hpp"

#include "../snowball/snowball.hpp"

#if __has_include(<linux/io_uring.h>)
#define T_URING_HAVE_UAPI 1

namespace uapi
{
#include <linux/io_uring.h>
};

using namespace uapi;

namespace ur = micron::uring;
static int FAILS = 0;

static_assert(ur::op_nop == uapi::IORING_OP_NOP && ur::op_read == uapi::IORING_OP_READ && ur::op_write == uapi::IORING_OP_WRITE);
static_assert(ur::op_openat == uapi::IORING_OP_OPENAT && ur::op_close == uapi::IORING_OP_CLOSE && ur::op_fsync == uapi::IORING_OP_FSYNC);
static_assert(ur::op_accept == uapi::IORING_OP_ACCEPT && ur::op_connect == uapi::IORING_OP_CONNECT);
static_assert(ur::op_send == uapi::IORING_OP_SEND && ur::op_recv == uapi::IORING_OP_RECV && ur::op_shutdown == uapi::IORING_OP_SHUTDOWN);
static_assert(ur::op_poll_add == uapi::IORING_OP_POLL_ADD && ur::op_timeout == uapi::IORING_OP_TIMEOUT
              && ur::op_timeout_remove == uapi::IORING_OP_TIMEOUT_REMOVE && ur::op_async_cancel == uapi::IORING_OP_ASYNC_CANCEL);
static_assert(ur::op_msg_ring == uapi::IORING_OP_MSG_RING && ur::op_futex_wait == uapi::IORING_OP_FUTEX_WAIT
              && ur::op_futex_wake == uapi::IORING_OP_FUTEX_WAKE);
static_assert(ur::setup_clamp == IORING_SETUP_CLAMP && ur::setup_coop_taskrun == IORING_SETUP_COOP_TASKRUN
              && ur::setup_single_issuer == IORING_SETUP_SINGLE_ISSUER && ur::setup_defer_taskrun == IORING_SETUP_DEFER_TASKRUN);
static_assert(ur::enter_getevents == IORING_ENTER_GETEVENTS && ur::feat_single_mmap == IORING_FEAT_SINGLE_MMAP
              && ur::feat_cqe_skip == IORING_FEAT_CQE_SKIP);
static_assert(ur::sqe_cqe_skip_success == IOSQE_CQE_SKIP_SUCCESS && ur::sqe_io_link == IOSQE_IO_LINK);
static_assert(ur::off_sq_ring == IORING_OFF_SQ_RING && ur::off_cq_ring == IORING_OFF_CQ_RING && ur::off_sqes == IORING_OFF_SQES);
static_assert(ur::timeout_abs == IORING_TIMEOUT_ABS);
static_assert(sizeof(ur::sqe) == sizeof(uapi::io_uring_sqe) && sizeof(ur::cqe) == sizeof(uapi::io_uring_cqe)
              && sizeof(ur::params) == sizeof(uapi::io_uring_params));
static_assert(__builtin_offsetof(ur::sqe, user_data) == __builtin_offsetof(uapi::io_uring_sqe, user_data)
              && __builtin_offsetof(ur::sqe, addr3) == __builtin_offsetof(uapi::io_uring_sqe, addr3));
#endif

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

  sb::require(FAILS == 0);
  sb::print("=== ALL URING SUBSTRATE TESTS PASSED ===");
  return 1;
}
