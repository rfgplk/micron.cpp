@  Copyright (c) 2026- David Lucius Severus
@
@  Distributed under the Boost Software License, Version 1.0.
@  See accompanying file LICENSE_1_0.txt or copy at
@  http://www.boost.org/LICENSE_1_0.txt

@ armv7-a RESET VECTOR

@ .text.reset, not .text: on a real board the CPU begins executing at the image base, so the
@ reset vector has to BE at the image base. Left in plain .text it lands wherever link order
@ puts it -- measured at 0x40009ccc once, entirely at the mercy of which object duck passed
@ first. qemu -kernel reads e_entry and does not care, which is exactly what makes it the kind
@ of thing that works until it is on hardware. metal_arm{32,64}.ld pins this section first.
	.section .text.reset, "ax", %progbits
	.global	_start
	.type	_start, %function

_start:
	@ there is no stack until this pair
	ldr	r0, =__stack_top
	mov	sp, r0
	mov	fp, #0                     @ outermost frame marker

	@ .data from LMA to VMA
	ldr	r0, =__data_load
	ldr	r1, =__data_start
	ldr	r2, =__data_end
	cmp	r0, r1
	beq	2f
1:	cmp	r1, r2
	bhs	2f
	ldrb	r3, [r0], #1
	strb	r3, [r1], #1
	b	1b
2:
	@ zero .bss
	ldr	r1, =__bss_start
	ldr	r2, =__bss_end
	mov	r3, #0
3:	cmp	r1, r2
	bhs	4f
	strb	r3, [r1], #1
	b	3b
4:
	bic	sp, sp, #7                 @ AAPCS requires 8-byte aligned sp
	bl	__micron_metalc

5:	wfi                                @ metalc ends in port::halt; park if a board's returned
	b	5b

	.size	_start, . - _start

	.section .note.GNU-stack,"",%progbits
