//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// think about moving to /memory

#include "../syscall.hpp"
#include <unistd.h>

namespace micron
{

// NOTE: these defines have been pushed through like this because half the code base has already been written with or'ing
// these constants in mind. much easier to do it like this
// TODO: replace this with an enum'd system

#ifndef PROT_READ     // rationale: if PROT_READ is already defined it almost certainly means sys/mman.h is being
                      // included
#define MAP_FAILED (void *)-1
constexpr int PROT_READ = 0x1;             /* Page can be read.  */
constexpr int PROT_WRITE = 0x2;            /* Page can be written.  */
constexpr int PROT_EXEC = 0x4;             /* Page can be executed.  */
constexpr int PROT_NONE = 0x0;             /* Page can not be accessed.  */
constexpr int PROT_GROWSDOWN = 0x01000000; /* Extend change to start of
                                      growsdown vma (mprotect only).  */
constexpr int PROT_GROWSUP = 0x02000000;   /* Extend change to start of
                                      growsup vma (mprotect only).  */

/* Sharing types (must choose one and only one of these).  */
constexpr int MAP_SHARED = 0x01;          /* Share changes.  */
constexpr int MAP_PRIVATE = 0x02;         /* Changes are private.  */
constexpr int MAP_SHARED_VALIDATE = 0x03; /* Share changes and validate
                                     extension flags.  */
constexpr int MAP_DROPPABLE = 0x08;       /* Zero memory under memory pressure.  */
constexpr int MAP_TYPE = 0x0f;            /* Mask for type of mapping.  */

/* Other flags.  */
constexpr int MAP_FIXED = 0x10; /* Interpret addr exactly.  */
constexpr int MAP_FILE = 0;
constexpr int MAP_ANONYMOUS = 0x20; /* Don't use a file.  */
constexpr int MAP_ANON = MAP_ANONYMOUS;

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
constexpr int MAP_POPULATE = 0x008000;        /* populate (prefault) pagetables */
constexpr int MAP_NONBLOCK = 0x010000;        /* do not block on IO */
constexpr int MAP_STACK = 0x020000;           /* give out an address that is best suited for process/thread stacks */
constexpr int MAP_HUGETLB = 0x040000;         /* create a huge page mapping */
constexpr int MAP_SYNC = 0x080000;            /* perform synchronous page faults for the mapping */
constexpr int MAP_FIXED_NOREPLACE = 0x100000; /* MAP_FIXED which doesn't unmap underlying mapping */

constexpr int MAP_UNINITIALIZED =  0x4000000; /* For anonymous mmap, memory could be

/* When MAP_HUGETLB is set, bits [26:31] encode the log2 of the huge page size.
The following definitions are associated with this huge page size encoding.
It is responsibility of the application to know which sizes are supported on
the running system.  See mmap(2) man page for details.  */

constexpr int MAP_HUGE_SHIFT = 26;
constexpr int MAP_HUGE_MASK = 0x3f;

constexpr int MAP_HUGE_16KB = (14 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_64KB = (16 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_512KB = (19 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_1MB = (20 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_2MB = (21 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_8MB = (23 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_16MB = (24 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_32MB = (25 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_256MB = (28 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_512MB = (29 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_1GB = (30 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_2GB = (31 << MAP_HUGE_SHIFT);
constexpr int MAP_HUGE_16GB = (34U << MAP_HUGE_SHIFT);

/* Flags to `msync'.  */
constexpr int MS_ASYNC = 1;      /* Sync memory asynchronously.  */
constexpr int MS_SYNC = 4;       /* Synchronous memory sync.  */
constexpr int MS_INVALIDATE = 2; /* Invalidate the caches.  */

constexpr int MADV_NORMAL = 0;           /* No further special treatment.  */
constexpr int MADV_RANDOM = 1;           /* Expect random page references.  */
constexpr int MADV_SEQUENTIAL = 2;       /* Expect sequential page references.  */
constexpr int MADV_WILLNEED = 3;         /* Will need these pages.  */
constexpr int MADV_DONTNEED = 4;         /* Don't need these pages.  */
constexpr int MADV_FREE = 8;             /* Free pages only if memory pressure.  */
constexpr int MADV_REMOVE = 9;           /* Remove these pages and resources.  */
constexpr int MADV_DONTFORK = 10;        /* Do not inherit across fork.  */
constexpr int MADV_DOFORK = 11;          /* Do inherit across fork.  */
constexpr int MADV_MERGEABLE = 12;       /* KSM may merge identical pages.  */
constexpr int MADV_UNMERGEABLE = 13;     /* KSM may not merge identical pages.  */
constexpr int MADV_HUGEPAGE = 14;        /* Worth backing with hugepages.  */
constexpr int MADV_NOHUGEPAGE = 15;      /* Not worth backing with hugepages.  */
constexpr int MADV_DONTDUMP = 16;        /* Explicitly exclude from the core dump,
                                   overrides the coredump filter bits.  */
constexpr int MADV_DODUMP = 17;          /* Clear the MADV_DONTDUMP flag.  */
constexpr int MADV_WIPEONFORK = 18;      /* Zero memory on fork, child only.  */
constexpr int MADV_KEEPONFORK = 19;      /* Undo MADV_WIPEONFORK.  */
constexpr int MADV_COLD = 20;            /* Deactivate these pages.  */
constexpr int MADV_PAGEOUT = 21;         /* Reclaim these pages.  */
constexpr int MADV_POPULATE_READ = 22;   /* Populate (prefault) page tables
                                    readable.  */
constexpr int MADV_POPULATE_WRITE = 23;  /* Populate (prefault) page tables
                                    writable.  */
constexpr int MADV_DONTNEED_LOCKED = 24; /* Like MADV_DONTNEED, but drop
                                    locked pages too.  */
constexpr int MADV_COLLAPSE = 25;        /* Synchronous hugepage collapse.  */
constexpr int MADV_HWPOISON = 100;       /* Poison a page for testing.  */

#ifdef __USE_XOPEN2K
constexpr int POSIX_MADV_NORMAL = 0;     /* No further special treatment.  */
constexpr int POSIX_MADV_RANDOM = 1;     /* Expect random page references.  */
constexpr int POSIX_MADV_SEQUENTIAL = 2; /* Expect sequential page references.  */
constexpr int POSIX_MADV_WILLNEED = 3;   /* Will need these pages.  */
constexpr int POSIX_MADV_DONTNEED = 4;   /* Don't need these pages.  */
#endif

constexpr int MCL_CURRENT = 1; /* Lock all currently mapped pages.  */
constexpr int MCL_FUTURE = 2;  /* Lock all additions to address
                          space.  */
constexpr int MCL_ONFAULT = 4;
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
  map_private = 0x02,     // boo ;c
  shared_validate = 0x03,
  droppable = 0x08,
  type = 0x0f,
  fixed = 0x10,
  file = 0x0,
  anonymous = 0x20,
  anon = 0x20,
  __end
};

inline __attribute__((always_inline)) void *
mmap(void *__restrict addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  return reinterpret_cast<void *>(micron::syscall(SYS_mmap, addr, len, prot, flags, fd, offset));
}
// void *mmap64();
int
munmap(void *__restrict addr, size_t len)
{
  return (int)micron::syscall(SYS_munmap, addr, len);
}
int
mprotect(void *addr, size_t len, int prot, int pkey)
{
  return (int)micron::syscall(SYS_mprotect, addr, len, prot, pkey);
}
int
msync(void *addr, size_t len, int flags)
{
  return (int)micron::syscall(SYS_msync, addr, len, flags);
}
int
madvise(void *addr, size_t len, int advice)
{

  return (int)micron::syscall(SYS_madvise, addr, len, advice);
};
int
mlock(const void *addr, size_t len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}
int
munlock(const void *addr, size_t len)
{
  return (int)micron::syscall(SYS_mlock, addr, len);
}

int
memfd_create(const char* name, unsigned int flags)
{
  return (int)micron::syscall(SYS_memfd_create, name, flags);
}

int mlockall();
int munlockall();

};
