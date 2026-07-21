//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../tuple.hpp"

#include "../../linux/__includes.hpp"

#include "sig_callbacks.hpp"

#include "../../linux/sys/micthread/threads.hpp"
#include "../../linux/sys/resource.hpp"
#include "../../linux/sys/system.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../new.hpp"
#include "../../queue/lambda_queue.hpp"
#include "../../sync/futex.hpp"

#include "../../except.hpp"

#include "../../atomic/atomic.hpp"
#include "../../bits/__thread_exit_hook.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tuple.hpp"

#if defined(__micron_attach_capable)
#include "../../attach/info.hpp"
#endif

// NOTE: this file should contain all the constructors and implementations for the underlying threads (separate from the
// classes themselves), for readability

namespace micron
{
enum thread_returns : i32 { return_success, return_force, return_fail };

enum join_status { join_success, join_fail, join_allowed, join_busy };

// give a thread's TLS frame back to whoever built it.
// WARNING: when attached, the frame came from the host's info->make_child_frame() and must go back
// through info->free_frame() -- that callback exists in the ABI precisely so the host can own frame
// lifetime (a recycling pool, landlord accounting). A guest that munmaps it itself hands back memory
// the host still believes it owns, and the next attached thread handed that frame faults on its
// first thread_local access. Every teardown path routes here, not just the spawn-failure one.
inline void
__tls_release_frame(const micron::__tls_frame &f) noexcept
{
#if defined(__micron_attach_capable)
  if ( micron::__micron_attach_info != nullptr ) {
    micron::__micron_attach_info->free_frame(&f);
    return;
  }
#endif
  micron::__tls_free_frame(f);
}

// bounded budget for a hard-stopped (SIGTERM) thread to publish alive=false before it gets reaped
inline constexpr f64 __thread_term_timeout_ms = 1000.0;

template<typename T> struct __async_payload {
  atomic_token<bool> alive;
  atomic_token<u64> ret_val;
  posix::rusage_t usage;      // non atomic

  void
  clear(void)
  {
    alive.store(false);
    ret_val.store(0);
    usage = posix::rusage_t{};
  }
};

struct __thread_payload {
  atomic_token<bool> alive;
  // the child sets this true the instant it reaches __thread_kernel
  atomic_token<bool> started{ false };
  // native park/sleep/awake/terminate control word
  atomic_token<u32> park{ __park_run };
  // NOTE:
  // the pointer is used as the operand of a static_cast (7.6.1.8), except when the conversion is to pointer
  // to cv void, or to pointer to cv void and subsequently to pointer to cv char, cv unsigned char, or
  // cv std::byte (17.2.1)....
  // If a program attempts to access (3.1) the stored value of an object through a glvalue whose type is not
  // similar (7.3.5) to one of the following types the behavior is undefined:51
  //(11.1) — the dynamic type of the object,
  //(11.2) — a type that is the signed or unsigned type corresponding to the dynamic type of the object, or
  //(11.3) — a char, unsigned char, or std::byte type.
  atomic_ptr<byte *> ret_val;      // stop gap measure for returning arbitrary return types
  enum class tag : u8 { none = 0, literal, heap, __end };
  atomic_token<u8> tag_val{ (u8)tag::none };
  void (*deleter)(byte *) = nullptr;

  posix::rusage_t usage;      // non atomic

  void
  clear(void)
  {
    alive.store(false);
    started.store(false, memory_order_release);
    park.store(__park_run, memory_order_release);
    tag_val.store((u8)tag::none, memory_order_release);
    ret_val.store(nullptr, memory_order_release);
    deleter = nullptr;
    usage = posix::rusage_t{};
  }
};

// the running thread kernel self-exit routine
inline void
__micron_thread_die_impl() noexcept
{
  // run this thread's C++ thread_local dtors (guest modules) before the arena hook
  micron::__run_thread_dtors();
  if ( micron::__thread_exit_hook ) micron::__thread_exit_hook();
  if ( micron::__micron_thread_alive_word )
    static_cast<atomic_token<bool> *>(micron::__micron_thread_alive_word)->store(false, memory_order_seq_cst);
  micthread::__exit_thread(0);
}

struct __worker_payload {
  atomic_token<u32> has_work{ 0 };             // for futex compat.
  atomic_token<bool> should_die{ false };      // rude :/
  micron::lambda_queue<256> queue;
  atomic_token<u32> usage_seq{ 0 };
  posix::rusage_t usage;      // non atomic
};

// The kernel of the thread, encapsulating the req'd function
// status flags if the thread is running or not
// NOTE: args is always instantiated with decayed (value) types by __as_*_thread_attached
// taking args by value gives the kernel local storage; invoking fn(args...) passes lvalues
template<typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args &...>)
i32
__thread_kernel(__thread_payload *payload, Fn fn, Args... args)
{
  // prologue
  using ret_t = micron::invoke_result_t<Fn, Args &...>;
  __thread_handler();
  payload->alive.store(true, memory_order_seq_cst);
  // confirm to the spawner that we actually reached our kernel
  payload->started.store(true, memory_order_seq_cst);
  // publish this thread's native control state: alive flag + park word + self-exit routine, consumed by
  micron::__micron_thread_alive_word = static_cast<void *>(&payload->alive);
  micron::__micron_thread_park = payload->park.ptr();
  micron::__micron_thread_die = &__micron_thread_die_impl;

  // work

  if constexpr ( micron::is_void_v<ret_t> ) {
    fn(args...);
    // function has _no_ return value, signal that it returned successfully (1)
    // NOTE: this may cause erronous behavior if 1 happens to be a valid pointer on your platform. be careful!
    payload->deleter = nullptr;
    payload->tag_val.store((u8)__thread_payload::tag::none, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(1), memory_order_release);
  } else if constexpr ( micron::is_class_v<ret_t> and !micron::is_trivially_constructible_v<ret_t> and !micron::is_literal_type_v<ret_t> ) {
    // NOTE: IMPORTANT
    // make sure this is freed either by the calling thread, or whatever handles returns
    ret_t *ret = new ret_t(fn(args...));
    payload->deleter = +[](byte *p) { delete reinterpret_cast<ret_t *>(p); };
    payload->tag_val.store((u8)__thread_payload::tag::heap, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
  } else if constexpr ( sizeof(ret_t) > sizeof(byte *) ) {
    // bigger than a pointer (8B on 64-bit, 4B on 32-bit): cannot be packed into ret_val, use new
    // NOTE: the bound is sizeof(byte*), NOT a hard-coded 8
    ret_t *ret = new ret_t(fn(args...));
    payload->deleter = +[](byte *p) { delete reinterpret_cast<ret_t *>(p); };
    payload->tag_val.store((u8)__thread_payload::tag::heap, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
  } else {
    // type is literal, can be stored.
    ret_t __ret_val_storage = fn(args...);
    u64 __ret_raw = 0;
    micron::cbytecpy<sizeof(ret_t)>(reinterpret_cast<byte *>(&__ret_raw), reinterpret_cast<const byte *>(&__ret_val_storage));
    payload->deleter = nullptr;
    payload->tag_val.store((u8)__thread_payload::tag::literal, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(__ret_raw), memory_order_release);
  }
  // end work

  // epilogue
  // NOTE: run any registered per-thread cleanup (abcmalloc releasing this thread's arena slot) on the exiting thread while its TLS is still
  // valid. thread_local C++ dtors (guest modules) run first, while the arena is still live
  micron::__run_thread_dtors();
  if ( micron::__thread_exit_hook ) micron::__thread_exit_hook();
  posix::getrusage(posix::rusage_thread, payload->usage);
  payload->alive.store(false, memory_order_seq_cst);
  return return_success;
}

// worker kernel, for concurrent arenas and parallelism
// main difference compared to a regular kernel is that this function NEVER returns, instead constantly loops checking
// for new work

inline i32
__worker_loop(__worker_payload *payload)
{
  for ( ;; ) {
    while ( payload->queue.empty() ) {
      if ( payload->should_die.get(memory_order_acquire) ) return return_force;
      // WARNING: consume a pending wake token via CAS 1->0 instead of an unconditional store(0)
      // an unconditional clear races the producer/stop() path: if our store(0) lands __AFTER__
      // the producer's store(1) and the wake fired before we reach wait_futex, the wakeup is
      // lost and we sleep on 0 forever
      u32 __hw_token = 1;
      payload->has_work.compare_exchange_strong(__hw_token, 0u, memory_order_acq_rel, memory_order_acquire);
      if ( !payload->queue.empty() ) break;      // a producer raced the clear; don't sleep
      if ( payload->should_die.get(memory_order_acquire) ) return return_force;
      wait_futex(payload->has_work.ptr(), 0);
    }
    if ( payload->should_die.get(memory_order_acquire) ) return return_force;
    while ( !payload->queue.empty() ) {
      payload->queue.execute();
      if ( payload->should_die.get(memory_order_acquire) ) return return_force;
    }
    // epilogue (seqlock-protected usage snapshot)
    payload->usage_seq.add_fetch(1, memory_order_acq_rel);
    posix::getrusage(posix::rusage_thread, payload->usage);
    payload->usage_seq.add_fetch(1, memory_order_acq_rel);
  }
  return return_success;
}

i32
__worker_kernel(__worker_payload *payload)
{
  // prologue
  __thread_handler();
  const i32 rc = __worker_loop(payload);
  // epilogue
  // NOTE: a worker exits through here, not through __thread_kernel, so it needs the same teardown:
  // thread_local C++ dtors (guest modules) then the arena hook, while this thread's TLS is still
  // mapped. Without it a worker's thread_local RAII objects (a uring ring, a buffered file, a socket)
  // are simply munmap'd with the frame and their fds leak for the life of the process
  micron::__run_thread_dtors();
  if ( micron::__thread_exit_hook ) micron::__thread_exit_hook();
  return rc;
}

// hosted backend
#if !defined(__micron_freestanding)
template<auto KFn, typename... Args> struct __pth_box {
  void *args[sizeof...(Args)];

  static void *
  trampoline(void *p)
  {
    auto *self = static_cast<__pth_box *>(p);
    __invoke(self, micron::make_index_sequence<sizeof...(Args)>{});
    __cleanup(self, micron::make_index_sequence<sizeof...(Args)>{});
    delete self;
    return nullptr;
  }

  static void
  destroy(__pth_box *self)
  {
    __cleanup(self, micron::make_index_sequence<sizeof...(Args)>{});
    delete self;
  }

private:
  template<usize... I>
  static void
  __invoke(__pth_box *self, micron::index_sequence<I...>)
  {
    KFn(static_cast<Args &&>(*static_cast<Args *>(self->args[I]))...);
  }

  template<usize... I>
  static void
  __cleanup([[maybe_unused]] __pth_box *self, micron::index_sequence<I...>)
  {
    (delete static_cast<Args *>(self->args[I]), ...);
  }
};
#endif

inline bool
__link_valid([[maybe_unused]] pid_t tid, [[maybe_unused]] unsigned long pth) noexcept
{
#if defined(__micron_freestanding)
  return tid != 0;
#else
  return pth != 0;
#endif
}

// NOTE: moving an already spawned thread (one whos kernel points back to a thread controlled control block is strictly UB
inline void
__thread_assert_movable_src([[maybe_unused]] pid_t tid, [[maybe_unused]] unsigned long pth)
{
  if ( __link_valid(tid, pth) )
    micron::exc<except::thread_error>("micron thread: cannot move a live/un-joined thread; join, cancel, or terminate it first");
}

// kernel tid for tgkill / identity
inline pid_t
__link_tid([[maybe_unused]] pid_t tid, [[maybe_unused]] unsigned long pth) noexcept
{
#if defined(__micron_freestanding)
  return tid;
#else
  return pth ? static_cast<pid_t>(pthread_gettid_np(static_cast<pthread_t>(pth))) : 0;
#endif
}

template<int Stack_Size, auto KFn, typename P, typename... KArgs>
void
__link_launch([[maybe_unused]] pid_t &tid, [[maybe_unused]] int &ctid, [[maybe_unused]] micron::__tls_frame &tls,
              [[maybe_unused]] unsigned long &pth, P *payload, addr_t *fstack, KArgs &&...kargs)
{
  if ( payload == nullptr || fstack == nullptr ) micron::exc<except::thread_error>("micron thread::__link_launch(): invalid arguments");
#if defined(__micron_freestanding)
  micron::__tls_frame frame{};
#if defined(__micron_attach_capable)
  // a guest (attached) thread's frame must be built by the host
  if ( micron::__micron_attach_info != nullptr ) {
    if ( micron::__micron_attach_info->make_child_frame(&frame) != 0 || frame.base == nullptr )
      micron::exc<except::thread_error>("micron thread::__link_launch(): host failed to build attached TLS");
  } else
#endif
  {
    frame = micron::__tls_make_child_frame();
    if ( frame.base == nullptr ) micron::exc<except::thread_error>("micron thread::__link_launch(): failed to build thread TLS");
  }
  auto reg = micthread::guard_stack(fstack, static_cast<usize>(Stack_Size));
  pid_t t = micthread::spawn<KFn>(reg.low, reg.usable, reinterpret_cast<unsigned long>(frame.tp), &ctid, payload,
                                  micron::forward<KArgs>(kargs)...);
  if ( t == micthread::thread_failed ) {
    micron::__tls_release_frame(frame);
    micron::exc<except::thread_error>("micron thread::__link_launch(): thread failed to spawn");
  }
  tls = frame;
  tid = t;
#else
  using box_t = __pth_box<KFn, P *, micron::decay_t<KArgs>...>;
  auto *box = new box_t{ { reinterpret_cast<void *>(new P *(payload)),
                           reinterpret_cast<void *>(new micron::decay_t<KArgs>(static_cast<KArgs &&>(kargs)))... } };
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  auto reg = micthread::guard_stack(fstack, static_cast<usize>(Stack_Size));
  pthread_attr_setstack(&attr, reg.low, reg.usable);
  pthread_t th{};
  int err = pthread_create(&th, &attr, &box_t::trampoline, box);
  pthread_attr_destroy(&attr);
  if ( err != 0 ) {
    box_t::destroy(box);
    micron::exc<except::thread_error>("micron thread::__link_launch(): thread failed to spawn");
  }
  pth = static_cast<unsigned long>(th);
#endif
}

inline int
__link_join([[maybe_unused]] pid_t tid, [[maybe_unused]] int *ctid, [[maybe_unused]] unsigned long pth)
{
  if ( !__link_valid(tid, pth) ) return 0;
#if defined(__micron_freestanding)
  micthread::join(ctid);
#else
  pthread_join(static_cast<pthread_t>(pth), nullptr);
#endif
  return 0;
}

// non-blocking: 0 if exited/none, error::busy if still running
inline int
__link_try_join([[maybe_unused]] pid_t tid, [[maybe_unused]] int *ctid, [[maybe_unused]] unsigned long pth)
{
  if ( !__link_valid(tid, pth) ) return 0;
#if defined(__micron_freestanding)
  return micthread::try_join(ctid) == 0 ? 0 : static_cast<int>(error::busy);
#else
  return pthread_tryjoin_np(static_cast<pthread_t>(pth), nullptr) == 0 ? 0 : static_cast<int>(error::busy);
#endif
}

struct thread_attr_t {
  ~thread_attr_t() = default;

  thread_attr_t(pid_t a)
      : parent(a), pid(0), ctid(0), tls{}, pth(0), sched_policy(0), sched_priority(0), sched_niceness(0), detach_state(0), stack_size(0),
        stack_addr(nullptr)
  {
  }

  thread_attr_t(pid_t a, const pthread_attr_t &)
      : parent(a), pid(0), ctid(0), tls{}, pth(0), sched_policy(0), sched_priority(0), sched_niceness(0), detach_state(0), stack_size(0),
        stack_addr(nullptr)
  {
  }

  void
  clear()
  {
    pid = 0;
    ctid = 0;
    tls = micron::__tls_frame{};
    pth = 0;
    stack_size = 0;
    stack_addr = nullptr;
  }

  thread_attr_t(thread_attr_t &&o) noexcept
      : parent(o.parent), pid(o.pid), ctid(o.ctid), tls(o.tls), pth(o.pth), sched_policy(o.sched_policy), sched_priority(o.sched_priority),
        sched_niceness(o.sched_niceness), detach_state(o.detach_state), stack_size(o.stack_size), stack_addr(o.stack_addr)
  {
    o.pid = 0;
    o.ctid = 0;
    o.tls = micron::__tls_frame{};
    o.pth = 0;
    o.stack_size = 0;
    o.stack_addr = nullptr;
  }

  thread_attr_t &
  operator=(thread_attr_t &&o) noexcept
  {
    if ( this == &o ) return *this;
    parent = o.parent;
    pid = o.pid;
    ctid = o.ctid;
    tls = o.tls;
    pth = o.pth;
    sched_policy = o.sched_policy;
    sched_priority = o.sched_priority;
    sched_niceness = o.sched_niceness;
    detach_state = o.detach_state;
    stack_size = o.stack_size;
    stack_addr = o.stack_addr;
    o.pid = 0;
    o.ctid = 0;
    o.tls = micron::__tls_frame{};
    o.pth = 0;
    o.stack_size = 0;
    o.stack_addr = nullptr;
    return *this;
  }

  pid_t parent;
  pid_t pid;                    // kernel tid (freestanding; 0 = none/joined)
  int ctid;                     // CHILD_CLEARTID join futex word (freestanding; address must stay stable while running)
  micron::__tls_frame tls;      // per-thread TLS block (freestanding; joiner frees)
  unsigned long pth;            // pthread_t handle (hosted backend)
  i32 sched_policy;
  u32 sched_priority;
  u32 sched_niceness;
  i32 detach_state;
  usize stack_size;
  addr_t *stack_addr;
};

inline thread_attr_t
__thread_attr_with_stack(pid_t parent, int policy, addr_t *stack, usize sz, int prio = 0) noexcept
{
  thread_attr_t a(parent);
  a.stack_addr = stack;
  a.stack_size = sz;
  a.sched_policy = policy;
  a.sched_priority = static_cast<u32>(prio);
  return a;
}

};      // namespace micron
