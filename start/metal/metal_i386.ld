/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 *  i386 BARE-METAL IMAGE LAYOUT
 *
 *  Sized for a PC-class target booted by `qemu-system-x86_64 -kernel`
 *
 *  Four symbol groups, and each has exactly one consumer:
 *    __stack_top                   reset_i386.s, before anything else exists
 *    __data_load/_start/_end       reset_i386.s, the LMA -> VMA copy
 *    __bss_start/__bss_end         reset_i386.s, the zero fill
 *    __heap_start/__heap_end       the board's RAM.
 */

ENTRY(_start)

SECTIONS
{
  /* 1 MiB: below this is BIOS/legacy space on a PC. Multiboot loads at the link address. */
  . = 1M;

  .text : ALIGN(4K)
  {
    KEEP(*(.mbhdr))
    *(.text .text.*)
  }

  .rodata : ALIGN(4K) { *(.rodata .rodata.*) }

  .note.gnu.build-id : { *(.note.gnu.build-id) }

  .preinit_array : ALIGN(4)
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  }

  .init_array : ALIGN(4)
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  }

  .fini_array : ALIGN(4)
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

  . = ALIGN(4K);
  __stack_bottom = .;
  . = . + 64K;
  __stack_top = .;

  . = ALIGN(4K);
  __heap_start = .;
  . = 16M;
  __heap_end = .;

  /DISCARD/ :
  {
    *(.eh_frame)
    *(.eh_frame_hdr)
    *(.comment)
    *(.note.GNU-stack)
  }
}
