//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_start
	.type	_start, %function

_start:
	mov	x29, #0                    // outermost frame marker (fp)
	mov	x30, #0                    // and lr
	ldr	x0, [sp]                   // argc
	add	x1, sp, #8                 // argv = sp + 8

	add	x2, x1, x0, lsl #3         // &argv[argc] = argv + argc*8
	add	x2, x2, #8                 // envp = past argv NULL

	mov	x3, x2                     // walk envp searching for the trailing NULL
.Lscan:
	ldr	x4, [x3]
	cmp	x4, #0
	b.eq	.Lfound
	add	x3, x3, #8
	b	.Lscan
.Lfound:
	add	x3, x3, #8                 // x3 = auxv

	mov	x4, sp                     // AArch64 ABI requires 16-byte aligned sp
	and	x4, x4, #-16
	mov	sp, x4

	bl	__micron_startc            // (x0=argc, x1=argv, x2=envp, x3=auxv) -> w0

	mov	x8, #93                    // SYS_exit (status already in x0)
	svc	#0
	brk	#0                         // unreachable

	.size	_start, . - _start

	.section .note.GNU-stack,"",%progbits
