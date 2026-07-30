//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// the TLS template capture, both ways round
//
// micron builds every freestanding thread's TLS block from a template. that template used to come only from /proc/self/auxv, so a chroot or
// a container without /proc could not spawn a single thread -- the spawn does not degrade, it aborts the process. find_tls_from_ehdr() is
// the /proc-free twin; this pins it against the auxv answer, which is the oracle wherever both are valid

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/linux/elf/auxv.hpp"
#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/sys/micthread/tls.hpp"
#include "../../src/thread/threads.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{

struct auxv_answer {
  micron::tls_image img{ nullptr, 0, 0, 0 };
  unsigned long pagesz = 0;
  bool read_ok = false;
};

// the oracle: what the kernel itself put in the aux vector
auxv_answer
from_proc(void)
{
  auxv_answer out;
  alignas(16) unsigned long buf[512];
  i32 fd = micron::posix::open("/proc/self/auxv", micron::posix::o_rdonly);
  if ( fd < 0 ) return out;
  usize total = 0;
  for ( ;; ) {
    long n = micron::posix::read(fd, reinterpret_cast<char *>(buf) + total, sizeof(buf) - total);
    if ( n <= 0 ) break;
    total += static_cast<usize>(n);
    if ( total >= sizeof(buf) ) break;
  }
  micron::posix::close(fd);

  unsigned long phdr = 0, phent = 0, phnum = 0;
  for ( usize i = 0; i < total / (2 * sizeof(unsigned long)); ++i ) {
    const unsigned long t = buf[2 * i], v = buf[2 * i + 1];
    if ( t == micron::at_null ) break;
    if ( t == micron::at_phdr )
      phdr = v;
    else if ( t == micron::at_phent )
      phent = v;
    else if ( t == micron::at_phnum )
      phnum = v;
    else if ( t == micron::at_pagesz )
      out.pagesz = v;
  }
  if ( phdr == 0 or phent == 0 or phnum == 0 ) return out;
  out.img = micron::find_tls_in_phdrs(phdr, phent, phnum);
  out.read_ok = true;
  return out;
}

thread_local int __tl_probe = 0xa5;

}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== TLS TEMPLATE RIGOR ===");

  test_case("the linker gave us a usable ELF header");
  {
    require_true(__ehdr_start != nullptr);
    require_true(__ehdr_usable());
    const ehdr_t *eh = reinterpret_cast<const ehdr_t *>(__ehdr_start);
    require(eh->e_ident[0], static_cast<byte>(0x7f));
    require(eh->e_ident[1], static_cast<byte>('E'));
    require(eh->e_ident[2], static_cast<byte>('L'));
    require(eh->e_ident[3], static_cast<byte>('F'));
    require_true(eh->e_phoff != 0);
    require_true(eh->e_phentsize != 0);
    require_true(eh->e_phnum != 0);
  }
  end_test_case();

  test_case("find_tls_from_ehdr agrees with the /proc/self/auxv oracle");
  {
    const auxv_answer o = from_proc();
    const tls_image e = find_tls_from_ehdr();
    if ( !o.read_ok ) {
      // no /proc to compare against -- which is exactly the case this whole change exists for
      sb::print("no /proc/self/auxv; oracle comparison SKIPPED");
      require_true(e.image != nullptr);
    } else {
      // WARNING: a mismatch here is a wrong load bias, which hands __tls_make_frame a wild image pointer to memcpy the .tdata out of
      require(reinterpret_cast<p64>(e.image), reinterpret_cast<p64>(o.img.image));
      require(e.filesz, o.img.filesz);
      require(e.memsz, o.img.memsz);
      require(e.align, o.img.align);
    }
  }
  end_test_case();

  test_case("this binary really has a PT_TLS to capture");
  {
    // the thread_local above guarantees one; without it the rest of the file would pass vacuously
    __tl_probe = 0x5a;
    require(__tl_probe, 0x5a);
    const tls_image e = find_tls_from_ehdr();
    require_true(e.image != nullptr);
    require_true(e.memsz != 0);
    require_true(e.align != 0);
  }
  end_test_case();

  test_case("__tls_capture_from_ehdr fills the template");
  {
    require_true(__tls_capture_from_ehdr());
    require_true(__micron_tls_template.valid);
    require_true(__micron_tls_template.image != nullptr);
    require_true(__micron_tls_template.memsz != 0);
    // the 4K floor, deliberately not micron::getpagesize() -- see the WARNING at __tls_capture_from_ehdr
    require(__micron_tls_template.pagesz, static_cast<usize>(4096));
  }
  end_test_case();

  test_case("a frame built from the ehdr template is well formed");
  {
    const __tls_template_t &t = __micron_tls_template;
    __tls_frame f = __tls_make_frame(t.image, t.filesz, t.memsz, t.align, t.pagesz);
    require_true(f.base != nullptr);
    require_true(f.tp != nullptr);
    require(f.size % 4096, static_cast<usize>(0));
    require(f.image_size, __tls_round_up(t.memsz, t.align));
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
    // variant II: the TCB self-pointer at tp[0] is what %fs:0 has to resolve to
    require_true(*reinterpret_cast<void **>(f.tp) == static_cast<void *>(f.tp));
#endif
    __tls_free_frame(f);
  }
  end_test_case();

  test_case("__tls_make_frame refuses a p_align its page size cannot back");
  {
    // pins the guard at __tls_make_frame: mmap only ever guarantees page alignment, so an align above the page must be refused rather than
    // silently handed back misaligned. this is what would break if page_sz were ever widened to micron::getpagesize() (64K on arm64)
    const __tls_template_t &t = __micron_tls_template;
    __tls_frame bad = __tls_make_frame(t.image, t.filesz, t.memsz, 8192, 4096);
    require_true(bad.base == nullptr);
  }
  end_test_case();

  test_case("threads_available");
  {
    require_true(threads_available());
    require_true(micthread::available());
    require_true(micthread::level() != micthread::tier::none);
  }
  end_test_case();

  sb::print("=== ALL TLS TEMPLATE TESTS PASSED ===");
  return 1;
}
