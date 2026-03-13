//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

// included
constexpr static const addr_t *map_failed = numeric_limits<addr_t *>::max();
constexpr static const i32 prot_read = 0x1;             /* page can be read.  */
constexpr static const i32 prot_write = 0x2;            /* page can be written.  */
constexpr static const i32 prot_exec = 0x4;             /* page can be executed.  */
constexpr static const i32 prot_none = 0x0;             /* page can not be accessed.  */
constexpr static const i32 prot_growsdown = 0x01000000; /* extend change to start of
                                      growsdown vma (mprotect only).  */
constexpr static const i32 prot_growsup = 0x02000000;   /* extend change to start of
                                      growsup vma (mprotect only).  */

/* sharing types (must choose one and only one of these).  */
constexpr static const i32 map_shared = 0x01;          /* share changes.  */
constexpr static const i32 map_private = 0x02;         /* changes are private.  */
constexpr static const i32 map_shared_validate = 0x03; /* share changes and validate
                                     extension flags.  */
constexpr static const i32 map_droppable = 0x08;       /* zero memory under memory pressure.  */
constexpr static const i32 map_type = 0x0f;            /* mask for type of mapping.  */

/* other flags.  */
constexpr static const i32 map_fixed = 0x10; /* i32erpret addr exactly.  */
constexpr static const i32 map_file = 0;
constexpr static const i32 map_anonymous = 0x20; /* don't use a file.  */
constexpr static const i32 map_anon = map_anonymous;

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
constexpr static const i32 map_populate = 0x008000;        /* populate (prefault) pagetables */
constexpr static const i32 map_nonblock = 0x010000;        /* do not block on io */
constexpr static const i32 map_stack = 0x020000;           /* give out an address that is best suited for process/thread stacks */
constexpr static const i32 map_hugetlb = 0x040000;         /* create a huge page mapping */
constexpr static const i32 map_sync = 0x080000;            /* perform synchronous page faults for the mapping */
constexpr static const i32 map_fixed_noreplace = 0x100000; /* map_fixed which doesn't unmap underlying mapping */

constexpr static const i32 map_uninitialized = 0x4000000;

/* when map_hugetlb is set, bits [26:31] encode the log2 of the huge page size.
the following definitions are associated with this huge page size encoding.
it is responsibility of the application to know which sizes are supported on
the running system.  see mmap(2) man page for details.  */

constexpr static const i32 map_huge_shift = 26;
constexpr static const i32 map_huge_mask = 0x3f;

constexpr static const i32 map_huge_16kb = (14 << map_huge_shift);
constexpr static const i32 map_huge_64kb = (16 << map_huge_shift);
constexpr static const i32 map_huge_512kb = (19 << map_huge_shift);
constexpr static const i32 map_huge_1mb = (20 << map_huge_shift);
constexpr static const i32 map_huge_2mb = (21 << map_huge_shift);
constexpr static const i32 map_huge_8mb = (23 << map_huge_shift);
constexpr static const i32 map_huge_16mb = (24 << map_huge_shift);
constexpr static const i32 map_huge_32mb = (25 << map_huge_shift);
constexpr static const i32 map_huge_256mb = (28 << map_huge_shift);
constexpr static const i32 map_huge_512mb = (29 << map_huge_shift);
constexpr static const i32 map_huge_1gb = (30 << map_huge_shift);
constexpr static const i32 map_huge_2gb = (31 << map_huge_shift);
constexpr static const i32 map_huge_16gb = (34u << map_huge_shift);

/* flags to `msync'.  */
constexpr static const i32 ms_async = 1;      /* sync memory asynchronously.  */
constexpr static const i32 ms_sync = 4;       /* synchronous memory sync.  */
constexpr static const i32 ms_invalidate = 2; /* invalidate the caches.  */

constexpr static const i32 madv_normal = 0;           /* no further special treatment.  */
constexpr static const i32 madv_random = 1;           /* expect random page references.  */
constexpr static const i32 madv_sequential = 2;       /* expect sequential page references.  */
constexpr static const i32 madv_willneed = 3;         /* will need these pages.  */
constexpr static const i32 madv_dontneed = 4;         /* don't need these pages.  */
constexpr static const i32 madv_free = 8;             /* free pages only if memory pressure.  */
constexpr static const i32 madv_remove = 9;           /* remove these pages and resources.  */
constexpr static const i32 madv_dontfork = 10;        /* do not inherit across fork.  */
constexpr static const i32 madv_dofork = 11;          /* do inherit across fork.  */
constexpr static const i32 madv_mergeable = 12;       /* ksm may merge identical pages.  */
constexpr static const i32 madv_unmergeable = 13;     /* ksm may not merge identical pages.  */
constexpr static const i32 madv_hugepage = 14;        /* worth backing with hugepages.  */
constexpr static const i32 madv_nohugepage = 15;      /* not worth backing with hugepages.  */
constexpr static const i32 madv_dontdump = 16;        /* explicitly exclude from the core dump,
                                   overrides the coredump filter bits.  */
constexpr static const i32 madv_dodump = 17;          /* clear the madv_dontdump flag.  */
constexpr static const i32 madv_wipeonfork = 18;      /* zero memory on fork, child only.  */
constexpr static const i32 madv_keeponfork = 19;      /* undo madv_wipeonfork.  */
constexpr static const i32 madv_cold = 20;            /* deactivate these pages.  */
constexpr static const i32 madv_pageout = 21;         /* reclaim these pages.  */
constexpr static const i32 madv_populate_read = 22;   /* populate (prefault) page tables
                                    readable.  */
constexpr static const i32 madv_populate_write = 23;  /* populate (prefault) page tables
                                    writable.  */
constexpr static const i32 madv_dontneed_locked = 24; /* like madv_dontneed, but drop
                                    locked pages too.  */
constexpr static const i32 madv_collapse = 25;        /* synchronous hugepage collapse.  */
constexpr static const i32 madv_hwpoison = 100;       /* poison a page for testing.  */

constexpr static const i32 posix_madv_normal = 0;     /* no further special treatment.  */
constexpr static const i32 posix_madv_random = 1;     /* expect random page references.  */
constexpr static const i32 posix_madv_sequential = 2; /* expect sequential page references.  */
constexpr static const i32 posix_madv_willneed = 3;   /* will need these pages.  */
constexpr static const i32 posix_madv_dontneed = 4;   /* don't need these pages.  */

constexpr static const i32 mcl_current = 1; /* lock all currently mapped pages.  */
constexpr static const i32 mcl_future = 2;  /* lock all additions to address
                          space.  */
constexpr static const i32 mcl_onfault = 4;

constexpr static const i32 mfd_cloexec = 0x0001;       /* set FD_CLOEXEC on the new FD */
constexpr static const i32 mfd_allow_sealing = 0x0002; /* allow sealing operations */
constexpr static const i32 mfd_hugetlb = 0x0004;       /* create in hugetlbfs */
constexpr static const i32 mfd_huge_2mb = (21 << map_huge_shift);
constexpr static const i32 mfd_huge_1gb = (30 << map_huge_shift);
constexpr static const i32 seal_shrink = 0x0002;
constexpr static const i32 seal_grow = 0x0004;
constexpr static const i32 seal_write = 0x0008;
constexpr static const i32 add_seals = 1033;

/* helper for unknown/unused bits */
constexpr static const i32 mfd_unused_mask = ~(mfd_cloexec | mfd_allow_sealing | mfd_hugetlb);

enum class mem_prots : i32 { read = 0x1, write = 0x2, exec = 0x4, none = 0x0, growsdown = 0x01000000, growsup = 0x02000000, __end };

enum class map_types : i32 {
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

};     // namespace micron
