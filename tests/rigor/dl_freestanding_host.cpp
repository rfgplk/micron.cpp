// Copyright (c) 2026 David Lucius Severus. Boost Software License 1.0.
#include "../../src/bits/eh/cxa_eh_globals.hpp"
#include "../../src/dynamic.hpp"
#include "../../src/linux/sys/limits.hpp"
#include "../../src/thread/thread.hpp"
#include "../snowball/snowball.hpp"

namespace
{
int event = 0;
u32 ready = 0, failed = 0, prepared = 0, released = 0, destroyed = 0;
uintptr_t globals[2]{};
int (*next_value)();
micron::dynamic_t finalizer_module;

struct tls_finalizer {
  ~tls_finalizer()
  {
    if ( next_value() != 43 ) __atomic_fetch_add(&failed, 1u, __ATOMIC_SEQ_CST);
    __atomic_fetch_add(&destroyed, 1u, __ATOMIC_SEQ_CST);
  }
};

int
answer()
{
  return 42;
}

void
note(int n)
{
  event = n;
}

void *
resolve(void *, const char *name, const char *)
{
  if ( micron::strcmp(name, "host_answer") == 0 ) return reinterpret_cast<void *>(&answer);
  if ( micron::strcmp(name, "host_event") == 0 ) return reinterpret_cast<void *>(&note);
  if ( micron::strcmp(name, "__tls_get_addr") == 0 ) return reinterpret_cast<void *>(&__tls_get_addr);
  return nullptr;
}

const char *
refuse(void *, const micron::elf::module_t &)
{
  return "test policy rejected module";
}

void
worker(unsigned id)
{
  thread_local tls_finalizer final_call;
  (void)&final_call;
  globals[id] = reinterpret_cast<uintptr_t>(__cxa_get_globals());
  if ( next_value() != 41 || next_value() != 42 ) __atomic_fetch_add(&failed, 1u, __ATOMIC_SEQ_CST);
  __atomic_fetch_add(&prepared, 1u, __ATOMIC_SEQ_CST);
  while ( !__atomic_load_n(&released, __ATOMIC_SEQ_CST) ) micron::posix::sched_yield();
  try {
    throw id;
  } catch ( unsigned value ) {
    __atomic_fetch_add(&ready, 1u, __ATOMIC_SEQ_CST);
    while ( __atomic_load_n(&ready, __ATOMIC_SEQ_CST) != 2 ) micron::posix::sched_yield();
    try {
      throw;
    } catch ( unsigned again ) {
      if ( value != id || again != id ) __atomic_fetch_add(&failed, 1u, __ATOMIC_SEQ_CST);
    }
  }
}

void
check_finalizers()
{
  sb::test_case("process finalizers retain dynamic TLS values");
  sb::require(next_value(), 42);
  sb::require(micron::dynamic_finalize(finalizer_module));
  sb::require(event, 42);
  event = 0;
  sb::require(micron::dynamic_finalize(finalizer_module));
  sb::require(event, 0);
  sb::require(micron::dynamic_close(finalizer_module));
  sb::require(event, 0);
}
}      // namespace

int
main()
{
  sb::test_case("host resolution and validation before constructors");
  constexpr const char *path = "tests/support/dl/x64/libmc_dl_host.so";
  bool rejected = false;
  try {
    (void)micron::dynamic_open(path, micron::rtld::now, { resolve, refuse, nullptr });
  } catch ( ... ) {
    rejected = true;
  }
  sb::require(rejected && event == 0);
  auto module = micron::dynamic_open(path, micron::rtld::now, { resolve, nullptr, nullptr });
  sb::require(event == 10);
  auto call = micron::dynamic_sym_as<int (*)()>(module, "host_call");
  next_value = micron::dynamic_sym_as<int (*)()>(module, "tls_next");
  sb::require(call && next_value && call() == 42);
  for ( unsigned emergency = 0; emergency < 2; ++emergency ) {
    sb::test_case(emergency ? "emergency exceptions and worker TLS destruction" : "exceptions and worker TLS destruction");
    ready = prepared = released = 0;
    auto a = micron::solo::spawn(worker, 0u);
    auto b = micron::solo::spawn(worker, 1u);
    while ( __atomic_load_n(&prepared, __ATOMIC_SEQ_CST) != 2 ) micron::posix::sched_yield();
    micron::posix::rlimit_t old{};
    sb::require(micron::posix::getrlimit(micron::posix::limits::virtual_memory, old) == 0);
    auto limited = old;
    limited.rlim_cur = 0;
    if ( emergency ) sb::require(micron::posix::setrlimit(micron::posix::limits::virtual_memory, limited) == 0);
    __atomic_store_n(&released, 1u, __ATOMIC_SEQ_CST);
    micron::solo::join(a);
    micron::solo::join(b);
    if ( emergency ) sb::require(micron::posix::setrlimit(micron::posix::limits::virtual_memory, old) == 0);
    sb::require(failed, 0u);
    sb::require(globals[0] && globals[1] && globals[0] != globals[1]);
    sb::require(destroyed, 2 * (emergency + 1));
    sb::require(micron::elf::__tls_thread_head == nullptr);
  }
  sb::test_case("main thread starts with its own dynamic TLS");
  sb::require(next_value(), 41);
  finalizer_module = module;
  sb::require(micron::atexit(check_finalizers) == 0);
  return 1;
}
