@  Copyright (c) 2026- David Lucius Severus
@
@  Distributed under the Boost Software License, Version 1.0.
@  See accompanying file LICENSE_1_0.txt or copy at
@  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_start
	.type	_start, %function

_start:
	mov	fp, #0                     @ outermost frame marker
	ldr	r0, [sp]                   @ argc
	add	r1, sp, #4                 @ argv = sp + 4

	add	r2, r1, r0, lsl #2         @ &argv[argc] = argv + argc*4
	add	r2, r2, #4                 @ envp = past argv NULL

	mov	r3, r2                     @ walk envp searching for the trailing NULL
.Lscan:
	ldr	ip, [r3]
	cmp	ip, #0
	beq	.Lfound
	add	r3, r3, #4
	b	.Lscan
.Lfound:
	add	r3, r3, #4                 @ r3 = auxv

	bic	sp, sp, #7                 @ AAPCS requires 8-byte aligned sp

	bl	__micron_startc            @ returns int in r0

	mov	r7, #1                     @ SYS_exit
	svc	#0
	udf	#0                         @ unreachable

	.size	_start, . - _start

	.section .note.GNU-stack,"",%progbits
