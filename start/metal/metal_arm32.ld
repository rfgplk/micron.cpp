/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 *  armv7-a BARE-METAL IMAGE LAYOUT
 *
 *  Origin 0x40000000 is where qemu-system-arm -M virt places RAM, and is a common board default
 *
 *  A real board replaces this file.
 *    __stack_top                   reset stub, before anything else exists
 *    __data_load/_start/_end       reset stub, the LMA -> VMA copy
 *    __bss_start/__bss_end         reset stub, the zero fill
 *    __{pre,}init_array_*          metal_start.cpp, via port::run_init_array.
 *    __heap_start/__heap_end       the board's RAM.
 */

ENTRY(_start)

SECTIONS
{
  . = 0x40000000;

  .text : ALIGN(4K)
  {
    KEEP(*(.text.reset))
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

  /* the stack grows DOWN from __stack_top */
  . = ALIGN(4K);
  __stack_bottom = .;
  . = . + 64K;
  __stack_top = .;

  . = ALIGN(4K);
  __heap_start = .;
  . = 0x41000000;
  __heap_end = .;

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
