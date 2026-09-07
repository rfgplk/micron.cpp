@  Copyright (c) 2026- David Lucius Severus
@
@  Distributed under the Boost Software License, Version 1.0.
@  See accompanying file LICENSE_1_0.txt or copy at
@  http://www.boost.org/LICENSE_1_0.txt

@ ARMv7-M RESET VECTOR (Cortex-M3/M4/M7; STM32)

	.syntax unified
	.cpu cortex-m4
	.thumb

@ %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@ vector table

	.section .isr_vector, "a", %progbits
	.align	2
	.global	__mc_vector_table
	.type	__mc_vector_table, %object
__mc_vector_table:
	.word	__stack_top                @  0  initial MSP -- loaded by hardware, not by code
	.word	Reset_Handler              @  1  initial PC, bit 0 set by .thumb_func
	.word	Default_Handler            @  2  NMI
	.word	HardFault_Handler          @  3
	.word	Default_Handler            @  4  MemManage
	.word	Default_Handler            @  5  BusFault
	.word	Default_Handler            @  6  UsageFault
	.word	0                          @  7  reserved
	.word	0                          @  8  reserved
	.word	0                          @  9  reserved
	.word	0                          @ 10  reserved
	.word	Default_Handler            @ 11  SVCall
	.word	Default_Handler            @ 12  DebugMonitor
	.word	0                          @ 13  reserved
	.word	Default_Handler            @ 14  PendSV
	.word	Default_Handler            @ 15  SysTick
	.size	__mc_vector_table, . - __mc_vector_table

@ %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@ reset

	.text
	.align	2
	.global	Reset_Handler
	.type	Reset_Handler, %function
	.thumb_func
Reset_Handler:
	ldr	r0, =__stack_top
	mov	sp, r0
	movs	r7, #0                     @ outermost frame marker -- r7 is the Thumb frame pointer

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
	movs	r3, #0
3:	cmp	r1, r2
	bhs	4f
	strb	r3, [r1], #1
	b	3b
4:
	mov	r0, sp
	bic	r0, r0, #7
	mov	sp, r0

	bl	__micron_metalc

	@ __micron_metalc ends in port::halt and does not return
5:	wfi
	b	5b
	.size	Reset_Handler, . - Reset_Handler

@ %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
@ faults

	.align	2
	.global	HardFault_Handler
	.type	HardFault_Handler, %function
	.thumb_func
HardFault_Handler:
	tst	lr, #4
	ite	eq
	mrseq	r4, msp
	mrsne	r4, psp

	ldr	r2, =__stack_top
	mov	sp, r2

	ldr	r0, =__mc_fault_msg
	ldr	r1, =__mc_fault_len
	bl	mc_mport_write_diag

	ldr	r0, =0xE000ED28
	ldr	r1, [r0]                   @ v0 = CFSR
	ldr	r0, =0xE000ED38
	ldr	r3, [r0]                   @ v1 = BFAR
	ldr	r0, [r4, #24]              @ v2 = the stacked PC
	push	{r0}
	ldr	r0, =__mc_n_pc             @ n2
	push	{r0}
	ldr	r0, =__mc_n_cfsr           @ n0
	ldr	r2, =__mc_n_bfar           @ n1
	bl	__mc_metal_fault_report
	add	sp, sp, #8

	movs	r0, #132                   @ SIGILL's shell code -- deliberately not micron's PASS sentinel
	bl	mc_mport_halt
6:	wfi
	b	6b
	.size	HardFault_Handler, . - HardFault_Handler

	.align	2
	.global	Default_Handler
	.type	Default_Handler, %function
	.thumb_func
Default_Handler:
	b	HardFault_Handler
	.size	Default_Handler, . - Default_Handler

	.section .rodata
	.align	2
__mc_fault_msg:
	.ascii	"micron-metal: unhandled exception\n"
__mc_fault_msg_end:
	.set	__mc_fault_len, __mc_fault_msg_end - __mc_fault_msg
__mc_n_cfsr:
	.asciz	"CFSR"
__mc_n_bfar:
	.asciz	"BFAR"
__mc_n_pc:
	.asciz	"PC"

	.section .note.GNU-stack,"",%progbits
