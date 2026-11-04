//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#include "header_odr_surface.hpp"

void *header_odr_arena();
void *header_odr_stream();
i32 *header_odr_errno();
void header_odr_release(byte *);
void *header_odr_foreign_malloc();
void header_odr_foreign_free(void *);
unsigned long *header_odr_foreign_new();
void header_odr_foreign_delete(unsigned long *);
void *header_odr_foreign_aligned();

#if defined(__micron_freestanding)
extern "C"
#endif
int
main()
{
  byte *p = abc::alloc(96);
  if ( !p || !abc::is_present(p) ) return 6;
  p[0] = 0x51;
  if ( header_odr_arena() != abc::__tls_arena ) return 6;
  header_odr_release(p);
  if ( abc::is_present(p) ) return 6;

  if ( header_odr_errno() != __micron_errno_location() ) return 6;
  micron::set_errno(123);
  if ( *header_odr_errno() != 123 ) return 6;

  if ( !micron::io::__global_buffer_stdout || !micron::io::__global_buffer_stderr ) return 6;
  if ( header_odr_stream() != micron::io::__global_buffer_stdout ) return 6;
  if ( micron::io::__global_buffer_stdout != &micron::io::__global_buffer_stdout_store ) return 6;
  // Re-entering boot must not reset an existing stream or repeat descriptor initialization.
  auto *saved = micron::io::__global_buffer_stdout;
  auto oldfd = micron::io::stdout;
  micron::io::stdout = micron::posix::invalid_fd;
  micron::io::__boot_io_buffers();
  if ( micron::io::stdout.open() || saved != micron::io::__global_buffer_stdout ) return 6;
  micron::io::stdout = oldfd;

  void *foreign = header_odr_foreign_malloc();
  if ( !foreign || !abc::is_present(static_cast<byte *>(foreign)) ) return 6;
  header_odr_foreign_free(foreign);
  if ( abc::is_present(static_cast<byte *>(foreign)) ) return 6;

  auto *scalar = header_odr_foreign_new();
  if ( !scalar || *scalar != 0x12345678ul || !abc::is_present(reinterpret_cast<byte *>(scalar)) ) return 6;
  header_odr_foreign_delete(scalar);
  if ( abc::is_present(reinterpret_cast<byte *>(scalar)) ) return 6;

  void *aligned = header_odr_foreign_aligned();
  if ( !aligned || (reinterpret_cast<addr_t>(aligned) & 255) != 0 ) return 6;
  byte *base = abc::__aligned_base_of(static_cast<byte *>(aligned));
  if ( !base || !abc::is_present(base) ) return 6;
  header_odr_foreign_free(aligned);
  if ( abc::is_present(base) ) return 6;
  return 1;
}
