@  Copyright (c) 2026- David Lucius Severus
@
@  Distributed under the Boost Software License, Version 1.0.
@  See accompanying file LICENSE_1_0.txt or copy at
@  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_continue
	.type	_continue, %function

_continue:
	push	{fp, lr}                  @ AAPCS keeps sp 8-byte aligned across the pair
	mov	fp, #0                     @ outermost frame marker
	bl	__micron_continuec          @ (r0 = args) -> i64 in r0:r1
	pop	{fp, pc}

	.size	_continue, . - _continue

	.section .note.GNU-stack,"",%progbits
