//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// think about moving to /memory

#include "../numerics.hpp"
#include "../linux/linux_types.hpp"
#include "../syscall.hpp"
#include "../types.hpp"
// #include <unistd.h>

namespace micron
{

// NOTE: these defines have been pushed through like this because half the code base has already been written with or'ing
// these constants in mind. much easier to do it like this
// TODO: replace this with an enum'd system

#ifndef PROT_READ     // rationale: if PROT_READ is already defined it almost certainly means sys/mman.h is being
                      // included
constexpr static const addr_t *map_failed = numeric_limits<addr_t*>::max();
constexpr static const int prot_read = 0x1;             /* page can be read.  */
constexpr static const int prot_write = 0x2;            /* page can be written.  */
constexpr static const int prot_exec = 0x4;             /* page can be executed.  */
constexpr static const int prot_none = 0x0;             /* page can not be accessed.  */
constexpr static const int prot_growsdown = 0x01000000; /* extend change to start of
                                      growsdown vma (mprotect only).  */
constexpr static const int prot_growsup = 0x02000000;   /* extend change to start of
                                      growsup vma (mprotect only).  */

/* sharing types (must choose one and only one of these).  */
constexpr static const int map_shared = 0x01;          /* share changes.  */
constexpr static const int map_private = 0x02;         /* changes are private.  */
constexpr static const int map_shared_validate = 0x03; /* share changes and validate
                                     extension flags.  */
constexpr static const int map_droppable = 0x08;       /* zero memory under memory pressure.  */
constexpr static const int map_type = 0x0f;            /* mask for type of mapping.  */

/* other flags.  */
constexpr static const int map_fixed = 0x10; /* interpret addr exactly.  */
constexpr static const int map_file = 0;
constexpr static const int map_anonymous = 0x20; /* don't use a file.  */
constexpr static const int map_anon = map_anonymous;

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
constexpr static const int map_populate = 0x008000; /* populate (prefault) pagetables */
constexpr static const int map_nonblock = 0x010000; /* do not block on io */
constexpr static const int map_stack = 0x020000; /* give out an address that is best suited for process/thread stacks */
constexpr static const int map_hugetlb = 0x040000;         /* create a huge page mapping */
constexpr static const int map_sync = 0x080000;            /* perform synchronous page faults for the mapping */
constexpr static const int map_fixed_noreplace = 0x100000; /* map_fixed which doesn't unmap underlying mapping */

constexpr static const int map_uninitialized = 0x4000000;

/* when map_hugetlb is set, bits [26:31] encode the log2 of the huge page size.
the following definitions are associated with this huge page size encoding.
it is responsibility of the application to know which sizes are supported on
the running system.  see mmap(2) man page for details.  */

constexpr static const int map_huge_shift = 26;
constexpr static const int map_huge_mask = 0x3f;

constexpr static const int map_huge_16kb = (14 << map_huge_shift);
constexpr static const int map_huge_64kb = (16 << map_huge_shift);
constexpr static const int map_huge_512kb = (19 << map_huge_shift);
constexpr static const int map_huge_1mb = (20 << map_huge_shift);
constexpr static const int map_huge_2mb = (21 << map_huge_shift);
constexpr static const int map_huge_8mb = (23 << map_huge_shift);
constexpr static const int map_huge_16mb = (24 << map_huge_shift);
constexpr static const int map_huge_32mb = (25 << map_huge_shift);
constexpr static const int map_huge_256mb = (28 << map_huge_shift);
constexpr static const int map_huge_512mb = (29 << map_huge_shift);
constexpr static const int map_huge_1gb = (30 << map_huge_shift);
constexpr static const int map_huge_2gb = (31 << map_huge_shift);
constexpr static const int map_huge_16gb = (34u << map_huge_shift);

/* flags to `msync'.  */
constexpr static const int ms_async = 1;      /* sync memory asynchronously.  */
constexpr static const int ms_sync = 4;       /* synchronous memory sync.  */
constexpr static const int ms_invalidate = 2; /* invalidate the caches.  */

constexpr static const int madv_normal = 0;           /* no further special treatment.  */
constexpr static const int madv_random = 1;           /* expect random page references.  */
constexpr static const int madv_sequential = 2;       /* expect sequential page references.  */
constexpr static const int madv_willneed = 3;         /* will need these pages.  */
constexpr static const int madv_dontneed = 4;         /* don't need these pages.  */
constexpr static const int madv_free = 8;             /* free pages only if memory pressure.  */
constexpr static const int madv_remove = 9;           /* remove these pages and resources.  */
constexpr static const int madv_dontfork = 10;        /* do not inherit across fork.  */
constexpr static const int madv_dofork = 11;          /* do inherit across fork.  */
constexpr static const int madv_mergeable = 12;       /* ksm may merge identical pages.  */
constexpr static const int madv_unmergeable = 13;     /* ksm may not merge identical pages.  */
constexpr static const int madv_hugepage = 14;        /* worth backing with hugepages.  */
constexpr static const int madv_nohugepage = 15;      /* not worth backing with hugepages.  */
constexpr static const int madv_dontdump = 16;        /* explicitly exclude from the core dump,
                                   overrides the coredump filter bits.  */
constexpr static const int madv_dodump = 17;          /* clear the madv_dontdump flag.  */
constexpr static const int madv_wipeonfork = 18;      /* zero memory on fork, child only.  */
constexpr static const int madv_keeponfork = 19;      /* undo madv_wipeonfork.  */
constexpr static const int madv_cold = 20;            /* deactivate these pages.  */
constexpr static const int madv_pageout = 21;         /* reclaim these pages.  */
constexpr static const int madv_populate_read = 22;   /* populate (prefault) page tables
                                    readable.  */
constexpr static const int madv_populate_write = 23;  /* populate (prefault) page tables
                                    writable.  */
constexpr static const int madv_dontneed_locked = 24; /* like madv_dontneed, but drop
                                    locked pages too.  */
constexpr static const int madv_collapse = 25;        /* synchronous hugepage collapse.  */
constexpr static const int madv_hwpoison = 100;       /* poison a page for testing.  */

constexpr static const int posix_madv_normal = 0;     /* no further special treatment.  */
constexpr static const int posix_madv_random = 1;     /* expect random page references.  */
constexpr static const int posix_madv_sequential = 2; /* expect sequential page references.  */
constexpr static const int posix_madv_willneed = 3;   /* will need these pages.  */
constexpr static const int posix_madv_dontneed = 4;   /* don't need these pages.  */

constexpr static const int mcl_current = 1; /* lock all currently mapped pages.  */
constexpr static const int mcl_future = 2;  /* lock all additions to address
                          space.  */
constexpr static const int mcl_onfault = 4;
#endif

enum class mem_prots : int {
  read = 0x1,
  write = 0x2,
  exec = 0x4,
  none = 0x0,
  growsdown = 0x01000000,
  growsup = 0x02000000,
  __end
};

enum class map_types : int {
  shared = 0x01,
  mmap_private = 0x02,     // boo ;c
  shared_validate = 0x03,
  droppable = 0x08,
  type = 0x0f,
  fixed = 0x10,
  file = 0x0,
  anonymous = 0x20,
  anon = 0x20,
  __end
};

// NOTE: mmap returns -1 on failure, will roll over
inline __attribute__((always_inline)) addr_t *
mmap(addr_t *__restrict addr, size_t len, int prot, int flags, int fd, posix::off_t offset)
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_mmap, addr, len, prot, flags, fd, offset));
}
// addr_t *mmap64();
int
munmap(addr_t *__restrict addr, size_t len)
{
  return (int)micron::syscall(SYS_munmap, addr, len);
}
int
mprotect(addr_t *addr, size_t len, int prot)
{
  return (int)micron::syscall(SYS_mprotect, addr, len, prot);
}
int
msync(addr_t *addr, size_t len, int flags)
{
  return (int)micron::syscall(SYS_msync, addr, len, flags);
}
int
madvise(addr_t *addr, size_t len, int advice)
{

  return (int)micron::syscall(SYS_madvise, addr, len, advice);
};
int
mlock(const addr_t *addr, size_t len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}
int
munlock(const addr_t *addr, size_t len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}

int
memfd_create(const char *name, unsigned int flags)
{
  return (int)micron::syscall(SYS_memfd_create, name, flags);
}

int mlockall();
int munlockall();

template <typename T>
auto
mprotect(T *addr, size_t __length, int prot)
{
  return micron::syscall(SYS_mprotect, addr, __length, prot);
}

template <typename T>
auto
brk(T *addr)
{
  return micron::syscall(SYS_brk, addr);
}

addr_t *
brk(nullptr_t t [[maybe_unused]])
{
  return reinterpret_cast<addr_t *>(micron::syscall(SYS_brk, 0));
}

addr_t *
sbrk(intptr_t increment)
{
  addr_t *__program_break = nullptr;
  __program_break = brk(nullptr);

  if ( increment == 0 ) [[likely]]
    return __program_break;

  brk(__program_break + increment);
  return __program_break;
}

};
