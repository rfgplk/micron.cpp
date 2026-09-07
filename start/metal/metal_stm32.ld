/*  Copyright (c) 2026 David Lucius Severus
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt
 *
 *  ARMv7-M BARE-METAL IMAGE LAYOUT; STM32F4-class
 *
 *  Sized for `qemu-system-arm -M netduinoplus2` (STM32F405: Cortex-M4, 1 MiB flash at 0x08000000).
 *  `-M netduino2` is an STM32F205 and takes this file unchanged.
 *
 *  THE F405 HAS 192 KiB OF SRAM AND IT IS NOT ONE REGION. 112 KiB of SRAM1 and 16 KiB of SRAM2 are
 *  contiguous at 0x20000000, and the other 64 KiB is CCM -- a separate block at 0x10000000, on a
 *  different bus, not reachable by DMA. Declaring LENGTH = 192K here is arithmetic that matches the
 *  datasheet's headline number and describes memory that is not there.
 *    __stack_top                   reset_cortexm.s
 *    __data_load/_start/_end       reset_cortexm.s, the flash -> SRAM copy
 *    __bss_start/__bss_end         reset_cortexm.s, the zero fill
 *    __{pre,}init_array_*          metal_start.cpp, via port::run_init_array
 *    __heap_start/__heap_end       the board's RAM, owned by port::pages
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K      /* SRAM1 112K + SRAM2 16K. NOT the CCM. */
}

ENTRY(Reset_Handler)

__stack_size = 8K;

SECTIONS
{
  .isr_vector : ALIGN(4)
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text : ALIGN(4)
  {
    *(.text .text.*)
  } > FLASH

  .rodata : ALIGN(4) { *(.rodata .rodata.*) } > FLASH

  .note.gnu.build-id : { *(.note.gnu.build-id) } > FLASH

  .preinit_array : ALIGN(4)
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  .init_array : ALIGN(4)
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array : ALIGN(4)
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP(*(.fini_array .dtors))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  __data_load = LOADADDR(.data);
  .data : ALIGN(4)
  {
    __data_start = .;
    *(.data .data.*)
    . = ALIGN(4);
    __data_end = .;
  } > SRAM AT> FLASH

  .bss (NOLOAD) : ALIGN(4)
  {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > SRAM

  . = ALIGN(8);
  __stack_bottom = .;
  . = . + __stack_size;
  __stack_top = .;

  . = ALIGN(8);
  __heap_start = .;
  __heap_end = ORIGIN(SRAM) + LENGTH(SRAM);

  ASSERT(__heap_end > __heap_start + 32K,
         "metal_stm32.ld: less than 32K of SRAM left for the heap. Shrink __stack_size, or the image.")
  ASSERT(__stack_top <= ORIGIN(SRAM) + LENGTH(SRAM),
         "metal_stm32.ld: the stack does not fit in SRAM.")

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
