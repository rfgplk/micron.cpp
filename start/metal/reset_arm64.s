//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// aarch64 RESET VECTOR
//
// Entered with the MMU off, caches off, at whatever exception level the board hands over -- EL1 for
// `qemu-system-aarch64 -M virt` with virtualization off, which is its default.
//
// TWO THINGS HERE ARE NOT ABOUT GETTING TO C, and both convert a silent lockup into a message:
//
//   CPACR_EL1.FPEN  is 0b00 at reset, which TRAPS every FP and SIMD instruction. --metal passes
//                   -mgeneral-regs-only so none should be emitted, and check_metal_image.sh
//                   asserts that -- but "should" is not "does", and the failure mode of being
//                   wrong is a trap taken through a vector table that does not exist.
//
//   VBAR_EL1        is UNKNOWN at reset. Until it is set, ANY exception -- an alignment fault from
//                   the MMU being off, a stray FP instruction, a null dereference -- branches into
//                   whatever the register happened to hold. The table below is 2 KiB of .text that
//                   writes one line and stops. It is not a fault handler; it is the difference
//                   between debugging a hang and reading a word.
//
// MEMORY IS DEVICE-nGnRnE WHILE THE MMU IS OFF, which is a real constraint and not a footnote: on
// Device memory every unaligned access faults, unconditionally, regardless of SCTLR_EL1.A. micron's
// scalar tier goes through __builtin_memcpy and byte loops so it does not generate any, and the
// image boots -- but a board that turns the MMU on and maps its RAM Normal-Cacheable is doing so
// for correctness as much as for speed. That mapping is a board's job, not a library's.

// .text.reset, not .text: on a real board the CPU begins executing at the image base, so the
// reset vector has to BE at the image base. Left in plain .text it lands wherever link order
// puts it -- measured at 0x40009ccc once, entirely at the mercy of which object duck passed
// first. qemu -kernel reads e_entry and does not care, which is exactly what makes it the kind
// of thing that works until it is on hardware. metal_arm{32,64}.ld pins this section first.
	.section .text.reset, "ax", %progbits
	.global	_start
	.type	_start, %function

_start:
	// there is no stack until this pair of instructions
	ldr	x0, =__stack_top
	mov	sp, x0
	mov	x29, #0                    // outermost frame marker
	mov	x30, #0

	// somewhere to land before anything can fault
	ldr	x0, =__mc_vectors
	msr	vbar_el1, x0

	// FPEN = 0b11: do not trap FP/SIMD at EL0 or EL1. Nothing should emit any; this is so that
	// being wrong is a fault we can report rather than one we cannot take.
	mrs	x0, cpacr_el1
	orr	x0, x0, #(3 << 20)
	msr	cpacr_el1, x0
	isb

	// .data from LMA to VMA
	ldr	x0, =__data_load
	ldr	x1, =__data_start
	ldr	x2, =__data_end
	cmp	x0, x1
	b.eq	2f
1:	cmp	x1, x2
	b.hs	2f
	ldrb	w3, [x0], #1
	strb	w3, [x1], #1
	b	1b
2:
	// zero .bss
	ldr	x1, =__bss_start
	ldr	x2, =__bss_end
3:	cmp	x1, x2
	b.hs	4f
	strb	wzr, [x1], #1
	b	3b
4:
	// SP must be 16-byte aligned before the call
	mov	x0, sp
	and	x0, x0, #~15
	mov	sp, x0

	bl	__micron_metalc

5:	wfi                                // metalc ends in port::halt; park if a board's returned
	b	5b

	.size	_start, . - _start

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// EXCEPTION VECTOR TABLE

	.align	11                         // 2 KiB, architectural
__mc_vectors:
	.rept	16
	b	__mc_fault
	.align	7                          // 0x80 per entry
	.endr

__mc_fault:
	// mc_mport_write_diag is weak with a no-op default, so this is safe before a board port exists
	adrp	x0, __mc_fault_msg
	add	x0, x0, :lo12:__mc_fault_msg
	mov	x1, #(__mc_fault_msg_end - __mc_fault_msg)
	// the fault may well have arrived on a broken stack; give the call a known-good one
	ldr	x2, =__stack_top
	mov	sp, x2
	bl	mc_mport_write_diag
	mov	w0, #132                   // SIGILL's shell code, and not micron's PASS sentinel
	bl	mc_mport_halt
1:	wfi
	b	1b

	.section .rodata
__mc_fault_msg:
	.ascii	"micron-metal: unhandled exception, VBAR_EL1 trap\n"
__mc_fault_msg_end:

	.section .note.GNU-stack,"",%progbits
