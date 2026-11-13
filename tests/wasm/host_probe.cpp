// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
#include "../../src/bits/wasm/host.hpp"
#include "../../src/bits/wasm/memory.hpp"
#include "../../src/bits/wasm/print_io.hpp"
#include "../../src/bits/wasm/threads.hpp"

extern "C" __attribute__((export_name("test_reserve"))) unsigned
test_reserve(unsigned bytes)
{
  auto *p = static_cast<unsigned char *>(micron::wasm::linear_reserve(bytes, 16));
  if ( !p ) return 0;
  p[0] = 0x35;
  p[bytes - 1] = 0xa7;
  return reinterpret_cast<unsigned>(p);
}

extern "C" __attribute__((export_name("test_write"))) int
test_write(unsigned p, unsigned n)
{
  return micron::wasm::write_diag(reinterpret_cast<const char *>(p), n, 1);
}

extern "C" __attribute__((export_name("test_write_byte"))) int
test_write_byte(unsigned byte, unsigned stream)
{
  return micron::io::fput(static_cast<char>(byte), micron::posix::fd_t{ static_cast<int>(stream) });
}

static int
write_partial_utf8(void *)
{
  const char stdout_bytes[] = "\xe2\x82\xac\xf0";
  for ( char c : stdout_bytes )
    if ( c && micron::io::fput(c, micron::io::stdout) != 1 ) return 6;
  return micron::posix::write(2, "\xe2", 1) == 1 ? 1 : 6;
}

static int
write_utf8_continuations(void *)
{
  return micron::posix::write(1, "\x9f\x98\x80", 3) == 3 && micron::posix::write(2, "\x82\xac", 2) == 2 ? 1 : 6;
}

extern "C" __attribute__((export_name("test_utf8_workers"))) int
test_utf8_workers()
{
#if defined(__micron_wasm_threads)
  micron::wasm::thread_handle worker;
  int result = 0;
  if ( micron::wasm::spawn(write_partial_utf8, nullptr, worker) || micron::wasm::join(worker, &result) || result != 1 ) return 6;
  if ( micron::io::fput('J', micron::io::stdout) != 1 ) return 6;
  if ( micron::wasm::spawn(write_utf8_continuations, nullptr, worker) || micron::wasm::join(worker, &result) || result != 1 ) return 6;
  return micron::io::fput('K', micron::io::stdout) == 1 ? 1 : 6;
#else
  return -95;
#endif
}

int
main()
{
  return 1;
}
