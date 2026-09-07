/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 *  amd64 BARE-METAL IMAGE LAYOUT
 *
 *  PVH, so `qemu-system-x86_64 -kernel` can load it with no bootloader. NOT multiboot: qemu's
 *  multiboot loader takes ELF32 only and commits to that path as soon as it sees the magic, so a
 *  multiboot header on a 64-bit image suppresses the protocol that works. See reset.s. The 32-bit trampoline
 *  reset.s enters through gets its OWN output section, .mb32, and that is not cosmetic: it is
 *  32-bit code in a 64-bit image, so `objdump -d -j .text` would decode it as nonsense and
 *  scripts/check_metal_image.sh would report instructions that are not there.
 *
 *  `duck --metal --metal-entry lm` uses reset_amd64_lm.s, which needs none of the above; the
 *  layout is otherwise identical, so one script serves both.
 *
 *  A real board replaces this file
 *    __stack_top                   reset stub, before anything else exists
 *    __data_load/_start/_end       reset stub, the LMA -> VMA copy
 *    __bss_start/__bss_end         reset stub, the zero fill
 *    __{pre,}init_array_*          metal_start.cpp, via port::run_init_array.
 *    __heap_start/__heap_end       the board's RAM
 */

ENTRY(_start)

SECTIONS
{
  . = 1M;

  /* the multiboot header must sit in the first 8 KiB of the FILE, so it leads the first section.
   * .mb32 is 32-bit code -- see the banner. */
  .mb32 : ALIGN(4K)
  {
    *(.text.mb32)
  }

  .text : ALIGN(4K)
  {
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) { *(.rodata .rodata.*) }

  /* placed explicitly: the toolchain injects --build-id on every link, and an orphan .note left to
   * the placement heuristic can land ahead of .text */
  .note.gnu.build-id : { *(.note.gnu.build-id) }

  /* STANDALONE ON PURPOSE. Folded into any other output section it comes out PROGBITS, ld builds no
   * PT_NOTE for it, and the loader reports "without PVH ELF Note" on an image that carries the
   * bytes -- measured. Its address does not matter; its section type does. */
  .note.Xen : ALIGN(4) { KEEP(*(.note.Xen)) }

  .preinit_array : ALIGN(8)
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  }

  .init_array : ALIGN(8)
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  }

  .fini_array : ALIGN(8)
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP(*(.fini_array .dtors))
    PROVIDE_HIDDEN(__fini_array_end = .);
  }

  __data_load = LOADADDR(.data);
  .data : ALIGN(4K)
  {
    __data_start = .;
    *(.data .data.*)
    __data_end = .;
  }

  .bss (NOLOAD) : ALIGN(4K)
  {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    __bss_end = .;
  }

  /* the stack grows DOWN from __stack_top */
  . = ALIGN(4K);
  __stack_bottom = .;
  . = . + 64K;
  __stack_top = .;

  . = ALIGN(4K);
  __heap_start = .;
  . = 16M;
  __heap_end = .;

  /* `. = 16M` is an ABSOLUTE assignment, not `. = . + 16M`. Grow the image past this and ld either
   * refuses to move the counter backwards or -- worse, when it lands just under -- hands the board
   * a heap of a few KiB with nothing naming the cause. bb's TINY profile wants 256 KiB; assert
   * comfortably above it so the failure is this message and not an allocation that returns null on
   * the fourth push_back. */
  ASSERT(__heap_end > __heap_start + 1M,
         "metal.ld: the image has grown into the heap. Raise the 16M ceiling on __heap_end.")

  /DISCARD/ :
  {
    *(.eh_frame)
    *(.eh_frame_hdr)
    *(.comment)
    *(.note.GNU-stack)
    *(.ARM.exidx*)
    *(.ARM.extab*)
  }
}
